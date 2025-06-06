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
        LOG_ERROR("VideoDecoder: No video stream found.");
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

    // Enable error concealment for robust decoding
    videoCodecContext_->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT;
    videoCodecContext_->err_recognition = AV_EF_EXPLODE | AV_EF_COMPLIANT | AV_EF_CRCCHECK;

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
    static bool firstValidFrame = false; // Track first valid frame

    if (!player_->isPlaying()) return;

    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedPlaybackTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - playbackStartTime_).count();

    // Use PTS difference for timing, fallback to frame rate
    double frameDelay = 1.0 / av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->r_frame_rate);
    if (videoClock_ <= elapsedPlaybackTime) {
        if (decodeVideoFrame()) {
            double nextVideoClock = videoFrame_->pts * av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->time_base);
            if (nextVideoClock < videoClock_ || nextVideoClock < 0) {
                // Invalid PTS, use frameDelay
                videoClock_ += frameDelay;
            } else {
                videoClock_ = nextVideoClock;
            }
            firstValidFrame = true;
            updateTexture();
        } else if (needsReset_) {
            // Seek to beginning and reset timing
            player_->seekToBeginning(videoStreamIndex_);
            flush(); // Clear codec buffers
            resetPlaybackTimes();
            needsReset_ = false;
            firstValidFrame = false;
            // Try decoding the first frame
            if (decodeVideoFrame()) {
                videoClock_ = videoFrame_->pts * av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->time_base);
                if (videoClock_ < 0) videoClock_ = frameDelay;
                firstValidFrame = true;
                updateTexture();
            }
        } else if (!firstValidFrame) {
            // Advance clock to avoid stalling on bad frames
            videoClock_ += frameDelay;
            LOG_DEBUG("VideoDecoder: Waiting for first valid frame, advancing clock to " << videoClock_ << ".");
        }
    }
}

SDL_Texture* VideoDecoder::getTexture() const {
    return texture_;
}

bool VideoDecoder::decodeVideoFrame() {
    static int invalidFrameSkipCount = 0;
    static int badPacketCount = 0;
    const int maxSkipFrames = 5;
    const int maxBadPackets = 2; // Flush more frequently for H.264

    while (player_->isPlaying()) {
        int ret = av_read_frame(player_->getFormatContext(), videoPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_packet_unref(videoPacket_);
                needsReset_ = true;
                invalidFrameSkipCount = 0;
                badPacketCount = 0;
                LOG_DEBUG("VideoDecoder: Reached EOF, setting needsReset_.");
                return false;
            }
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
            LOG_ERROR("VideoDecoder: Error reading video packet: " << err_buf << ".");
            av_packet_unref(videoPacket_);
            continue;
        }

        if (videoPacket_->stream_index == videoStreamIndex_) {
            ret = avcodec_send_packet(videoCodecContext_, videoPacket_);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("VideoDecoder: Error sending video packet to decoder: " << err_buf << ".");
                av_packet_unref(videoPacket_);
                if (videoClock_ < 1.0 && invalidFrameSkipCount < maxSkipFrames) {
                    invalidFrameSkipCount++;
                    badPacketCount++;
                    LOG_DEBUG("VideoDecoder: Skipping bad packet at start (count=" << invalidFrameSkipCount << ").");
                    if (badPacketCount >= maxBadPackets) {
                        LOG_DEBUG("VideoDecoder: Flushing codec after " << badPacketCount << " bad packets.");
                        avcodec_flush_buffers(videoCodecContext_);
                        badPacketCount = 0;
                    }
                    continue;
                }
                continue;
            }
            badPacketCount = 0;

            ret = avcodec_receive_frame(videoCodecContext_, videoFrame_);
            av_packet_unref(videoPacket_);
            if (ret >= 0) {
                // Validate frame more strictly
                if (videoFrame_->data[0] && videoFrame_->width == videoCodecContext_->width &&
                    videoFrame_->height == videoCodecContext_->height && videoFrame_->format == videoCodecContext_->pix_fmt &&
                    videoFrame_->pict_type != AV_PICTURE_TYPE_NONE) {
                    sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height,
                              rgbFrame_->data, rgbFrame_->linesize);
                    invalidFrameSkipCount = 0;
                    LOG_DEBUG("VideoDecoder: Valid frame decoded (width=" << videoFrame_->width << ", height=" << videoFrame_->height << ").");
                    return true;
                } else {
                    if (videoClock_ < 1.0 && invalidFrameSkipCount < maxSkipFrames) {
                        invalidFrameSkipCount++;
                        LOG_DEBUG("VideoDecoder: Skipping invalid frame at start (count=" << invalidFrameSkipCount
                                  << ", width=" << videoFrame_->width << ", height=" << videoFrame_->height
                                  << ", format=" << videoFrame_->format << ", pict_type=" << videoFrame_->pict_type << ").");
                        continue;
                    }
                    LOG_ERROR("VideoDecoder: Invalid frame data (width=" << videoFrame_->width << ", height=" << videoFrame_->height
                              << ", format=" << videoFrame_->format << ", pict_type=" << videoFrame_->pict_type << "). Giving up.");
                    return false;
                }
            } else if (ret == AVERROR(EAGAIN)) {
                continue;
            } else if (ret == AVERROR_EOF) {
                needsReset_ = true;
                invalidFrameSkipCount = 0;
                badPacketCount = 0;
                LOG_DEBUG("VideoDecoder: Decoder reached EOF.");
                return false;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("VideoDecoder: Error receiving video frame: " << err_buf << ".");
                if (videoClock_ < 1.0 && invalidFrameSkipCount < maxSkipFrames) {
                    invalidFrameSkipCount++;
                    LOG_DEBUG("VideoDecoder: Skipping error frame at start (count=" << invalidFrameSkipCount << ").");
                    continue;
                }
                return false;
            }
        } else {
            av_packet_unref(videoPacket_);
        }
    }
    invalidFrameSkipCount = 0;
    badPacketCount = 0;
    return false;
}

void VideoDecoder::updateTexture() {
    if (!texture_ || !rgbFrame_ || !rgbBuffer_ || !rgbFrame_->data[0] || rgbFrame_->linesize[0] <= 0) {
        LOG_ERROR("VideoDecoder: Invalid RGB frame data for texture update.");
        return;
    }

    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        // Clear texture to black to prevent ghosting
        SDL_memset(pixels, 0, pitch * height_);
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        const uint8_t* src = rgbBuffer_;
        int bytes_per_pixel_row = width_ * 3;
        if (rgbFrame_->linesize[0] < bytes_per_pixel_row) {
            LOG_ERROR("VideoDecoder: Invalid RGB frame linesize (" << rgbFrame_->linesize[0] << ") for texture update.");
            SDL_UnlockTexture(texture_);
            return;
        }
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