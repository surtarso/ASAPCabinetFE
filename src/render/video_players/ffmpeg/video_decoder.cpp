#include "video_decoder.h"
#include "ffmpeg_player.h"
#include "utils/logging.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

VideoDecoder::VideoDecoder(FFmpegPlayer* player)
    : player_(player), videoCodecContext_(nullptr), renderer_(nullptr), width_(0), height_(0),
      videoFrame_(nullptr), rgbFrame_(nullptr), videoPacket_(nullptr), swsContext_(nullptr),
      videoStreamIndex_(-1), rgbBuffer_(nullptr), texture_(nullptr), videoClock_(0.0),
      needsReset_(false) {}

VideoDecoder::~VideoDecoder() {
    cleanup();
}

bool VideoDecoder::setup(AVFormatContext* formatContext, SDL_Renderer* renderer, int width, int height) {
    renderer_ = renderer;
    width_ = width;
    height_ = height;

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
            break;
        }
    }
    if (videoStreamIndex_ == -1) {
        LOG_INFO("VideoDecoder: No video stream found.");
        return false;
    }

    const AVCodec* videoCodec = avcodec_find_decoder(formatContext->streams[videoStreamIndex_]->codecpar->codec_id);
    if (!videoCodec) {
        LOG_ERROR("VideoDecoder: Video codec not found.");
        cleanup();
        return false;
    }

    videoCodecContext_ = avcodec_alloc_context3(videoCodec);
    if (!videoCodecContext_) {
        LOG_ERROR("VideoDecoder: Failed to allocate video codec context.");
        cleanup();
        return false;
    }

    if (avcodec_parameters_to_context(videoCodecContext_, formatContext->streams[videoStreamIndex_]->codecpar) < 0) {
        LOG_ERROR("VideoDecoder: Failed to copy video codec parameters.");
        cleanup();
        return false;
    }

    if (avcodec_open2(videoCodecContext_, videoCodec, nullptr) < 0) {
        LOG_ERROR("VideoDecoder: Failed to open video codec.");
        cleanup();
        return false;
    }

    videoFrame_ = av_frame_alloc();
    rgbFrame_ = av_frame_alloc();
    videoPacket_ = av_packet_alloc();
    if (!videoFrame_ || !rgbFrame_ || !videoPacket_) {
        LOG_ERROR("VideoDecoder: Failed to allocate video frame or packet.");
        cleanup();
        return false;
    }

    swsContext_ = sws_getContext(
        videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt,
        width_, height_, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext_) {
        LOG_ERROR("VideoDecoder: Failed to initialize swscale context.");
        cleanup();
        return false;
    }

    int min_rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width_, height_, 1);
    if (min_rgb_buffer_size < 0) {
        LOG_ERROR("VideoDecoder: Failed to get required RGB buffer size: " << min_rgb_buffer_size << ".");
        cleanup();
        return false;
    }

    size_t extra_padding = 64;
    size_t allocated_rgb_buffer_size = min_rgb_buffer_size + extra_padding;
    rgbBuffer_ = (uint8_t*)av_malloc(allocated_rgb_buffer_size);
    if (!rgbBuffer_) {
        LOG_ERROR("VideoDecoder: Failed to allocate RGB frame buffer with extra padding!");
        cleanup();
        return false;
    }

    int ret = av_image_fill_arrays(rgbFrame_->data, rgbFrame_->linesize, rgbBuffer_, AV_PIX_FMT_RGB24, width_, height_, 1);
    if (ret < 0) {
        LOG_ERROR("VideoDecoder: Failed to fill RGB frame arrays: " << ret << ".");
        cleanup();
        return false;
    }

    rgbFrame_->width = width_;
    rgbFrame_->height = height_;
    rgbFrame_->format = AV_PIX_FMT_RGB24;

    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (!texture_) {
        LOG_ERROR("VideoDecoder: Failed to create texture: " << SDL_GetError() << ".");
        cleanup();
        return false;
    }

    needsReset_ = false;
    return true;
}

void VideoDecoder::play() {
    resetPlaybackTimes();
}

void VideoDecoder::stop() {
    flush();
}

void VideoDecoder::update() {
    if (!player_->isPlaying()) return;

    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedPlaybackTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - playbackStartTime_).count();

    if (videoClock_ <= elapsedPlaybackTime || needsReset_) {
        if (decodeVideoFrame()) {
            videoClock_ = videoFrame_->pts * av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->time_base);
            if (videoClock_ < 0) videoClock_ = 0;
            updateTexture();
        } else if (needsReset_) {
            player_->seekToBeginning(videoStreamIndex_);
            resetPlaybackTimes();
            needsReset_ = false;
        }
    }
}

SDL_Texture* VideoDecoder::getTexture() const {
    return texture_;
}

bool VideoDecoder::decodeVideoFrame() {
    if (needsReset_) {
        if (videoCodecContext_) {
            avcodec_free_context(&videoCodecContext_);
        }
        const AVCodec* videoCodec = avcodec_find_decoder(player_->getFormatContext()->streams[videoStreamIndex_]->codecpar->codec_id);
        if (!videoCodec) {
            LOG_ERROR("VideoDecoder: Video codec not found during reset.");
            return false;
        }
        videoCodecContext_ = avcodec_alloc_context3(videoCodec);
        if (!videoCodecContext_) {
            LOG_ERROR("VideoDecoder: Failed to re-allocate video codec context during reset.");
            return false;
        }
        if (avcodec_parameters_to_context(videoCodecContext_, player_->getFormatContext()->streams[videoStreamIndex_]->codecpar) < 0) {
            LOG_ERROR("VideoDecoder: Failed to re-copy video codec parameters during reset.");
            return false;
        }
        if (avcodec_open2(videoCodecContext_, videoCodec, nullptr) < 0) {
            LOG_ERROR("VideoDecoder: Failed to re-open video codec during reset.");
            return false;
        }
        sws_freeContext(swsContext_);
        swsContext_ = sws_getContext(
            videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt,
            width_, height_, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext_) {
            LOG_ERROR("VideoDecoder: Failed to re-initialize swscale context during reset.");
            return false;
        }
        needsReset_ = false;
        LOG_DEBUG("VideoDecoder: Video decoder fully reset.");
    }

    while (player_->isPlaying()) {
        int ret = av_read_frame(player_->getFormatContext(), videoPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_packet_unref(videoPacket_);
                needsReset_ = true;
                return false;
            }
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
            LOG_ERROR("VideoDecoder: Error reading video packet: " << err_buf << ".");
            av_packet_unref(videoPacket_);
            return false;
        }

        if (videoPacket_->stream_index == videoStreamIndex_) {
            ret = avcodec_send_packet(videoCodecContext_, videoPacket_);
            av_packet_unref(videoPacket_);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN)) {
                    int receive_ret;
                    while ((receive_ret = avcodec_receive_frame(videoCodecContext_, videoFrame_)) >= 0) {
                        sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height,
                                  rgbFrame_->data, rgbFrame_->linesize);
                        return true;
                    }
                    if (receive_ret == AVERROR(EAGAIN)) {
                        return false;
                    } else if (receive_ret == AVERROR_EOF) {
                        needsReset_ = true;
                        return false;
                    } else {
                        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, receive_ret);
                        LOG_ERROR("VideoDecoder: Error receiving frame after send_packet EAGAIN: " << err_buf << ".");
                        return false;
                    }
                } else {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                    LOG_ERROR("VideoDecoder: Error sending video packet to decoder: " << err_buf << ".");
                    return false;
                }
            }

            ret = avcodec_receive_frame(videoCodecContext_, videoFrame_);
            if (ret >= 0) {
                sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height,
                          rgbFrame_->data, rgbFrame_->linesize);
                return true;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                continue;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("VideoDecoder: Error receiving video frame from decoder: " << err_buf << ".");
                return false;
            }
        }
        av_packet_unref(videoPacket_);
    }
    return false;
}

void VideoDecoder::updateTexture() {
    if (!texture_ || !rgbFrame_ || !rgbBuffer_) return;

    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        const uint8_t* src = rgbBuffer_;
        int bytes_per_pixel_row = width_ * 3;
        for (int y = 0; y < height_; ++y) {
            memcpy(dst, src, bytes_per_pixel_row);
            dst += pitch;
            src += rgbFrame_->linesize[0];
        }
        SDL_UnlockTexture(texture_);
    } else {
        LOG_ERROR("VideoDecoder: Failed to lock texture: " << SDL_GetError() << ".");
    }
}

void VideoDecoder::flush() {
    if (videoCodecContext_) {
        avcodec_flush_buffers(videoCodecContext_);
    }
}

void VideoDecoder::resetPlaybackTimes() {
    videoClock_ = 0.0;
    lastFrameTime_ = std::chrono::high_resolution_clock::time_point();
    playbackStartTime_ = std::chrono::high_resolution_clock::now();
}

void VideoDecoder::cleanup() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }
    if (rgbBuffer_) {
        av_freep(&rgbBuffer_);
        rgbBuffer_ = nullptr;
    }
    if (rgbFrame_) {
        av_frame_free(&rgbFrame_);
    }
    if (videoFrame_) {
        av_frame_free(&videoFrame_);
    }
    if (videoPacket_) {
        av_packet_free(&videoPacket_);
    }
    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
    }
    videoStreamIndex_ = -1;
    needsReset_ = false;
}