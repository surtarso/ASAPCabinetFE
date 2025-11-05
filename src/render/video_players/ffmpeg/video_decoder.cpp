/**
 * @file video_decoder.cpp
 * @brief Implementation of the VideoDecoder class for decoding video streams in ASAPCabinetFE.
 */

#include "video_decoder.h"
#include "ffmpeg_player.h"
#include "log/logging.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/hwcontext.h>
#include <libavutil/error.h>
}

VideoDecoder::VideoDecoder(FFmpegPlayer* player)
    : player_(player), videoCodecContext_(nullptr), renderer_(nullptr), width_(0), height_(0),
      videoFrame_(nullptr), rgbFrame_(nullptr), videoPacket_(nullptr), swsContext_(nullptr),
      videoStreamIndex_(-1), rgbBuffer_(nullptr), texture_(nullptr), videoClock_(0.0),
      needsReset_(false), expectedSwFormat_(AV_PIX_FMT_NONE) {
    //LOG_DEBUG("VideoDecoder constructed for player: " + std::to_string(reinterpret_cast<uintptr_t>(player)));
}

VideoDecoder::~VideoDecoder() {
    cleanup();
    //LOG_DEBUG("VideoDecoder destroyed.");
}

bool VideoDecoder::setup(AVFormatContext* formatContext, SDL_Renderer* renderer, int width, int height) {
    if (!renderer) {
        LOG_ERROR("VideoDecoder::setup: renderer is null");
        return false;
    }
    renderer_ = renderer;
    width_ = width;
    height_ = height;

    LOG_DEBUG("Setting up VideoDecoder: width=" + std::to_string(width) + ", height=" + std::to_string(height));

    // Find video stream
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
            break;
        }
    }
    if (videoStreamIndex_ == -1) {
        LOG_ERROR("No video stream found.");
        return false;
    }

    // Find decoder
    const AVCodec* videoCodec = avcodec_find_decoder(formatContext->streams[videoStreamIndex_]->codecpar->codec_id);
    if (!videoCodec) {
        LOG_ERROR("Video codec not found for ID: " + std::to_string(formatContext->streams[videoStreamIndex_]->codecpar->codec_id));
        cleanup();
        return false;
    }

    // Allocate codec context
    videoCodecContext_ = avcodec_alloc_context3(videoCodec);
    if (!videoCodecContext_) {
        LOG_ERROR("Failed to allocate video codec context.");
        cleanup();
        return false;
    }

    // Copy codec parameters
    if (avcodec_parameters_to_context(videoCodecContext_, formatContext->streams[videoStreamIndex_]->codecpar) < 0) {
        LOG_ERROR("Failed to copy video codec parameters.");
        cleanup();
        return false;
    }

    // Hardware acceleration attempt
    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_VAAPI; // Configurable in future
    AVBufferRef* local_hwDeviceCtx = nullptr;
    bool hw_accel_enabled = false;

    int ret = av_hwdevice_ctx_create(&local_hwDeviceCtx, hwType, nullptr, nullptr, 0);
    if (ret >= 0 && local_hwDeviceCtx) {
        videoCodecContext_->hw_device_ctx = av_buffer_ref(local_hwDeviceCtx);
        if (videoCodecContext_->hw_device_ctx) {
            if (hwType == AV_HWDEVICE_TYPE_VAAPI) {
                expectedSwFormat_ = AV_PIX_FMT_NV12;
            } else if (hwType == AV_HWDEVICE_TYPE_DXVA2) {
                expectedSwFormat_ = AV_PIX_FMT_NV12;
            }
            LOG_DEBUG("Hardware acceleration enabled: " + std::string(av_hwdevice_get_type_name(hwType)));
            hw_accel_enabled = true;
        } else {
            LOG_WARN("Failed to reference hardware device context, falling back to software decoding.");
            expectedSwFormat_ = AV_PIX_FMT_NONE;
        }
    } else {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
        LOG_INFO("Hardware acceleration unavailable (" + std::string(err_buf) + "), using software decoding.");
        expectedSwFormat_ = AV_PIX_FMT_NONE;
    }

    if (local_hwDeviceCtx) {
        av_buffer_unref(&local_hwDeviceCtx);
    }

    // Configure codec context
    videoCodecContext_->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT;
    videoCodecContext_->err_recognition = AV_EF_EXPLODE | AV_EF_COMPLIANT | AV_EF_CRCCHECK;

    // Open codec
    ret = avcodec_open2(videoCodecContext_, videoCodec, nullptr);
    if (ret < 0 && hw_accel_enabled) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
        LOG_WARN("Failed to open codec with hardware acceleration (" + std::string(err_buf) + "), attempting software fallback.");

        av_buffer_unref(&videoCodecContext_->hw_device_ctx);
        videoCodecContext_->hw_device_ctx = nullptr;
        expectedSwFormat_ = AV_PIX_FMT_NONE;

        ret = avcodec_open2(videoCodecContext_, videoCodec, nullptr);
    }

    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
        LOG_ERROR("Failed to open video codec (" + std::string(err_buf) + ").");
        cleanup();
        return false;
    }

    // Allocate frames and packet
    videoFrame_ = av_frame_alloc();
    rgbFrame_ = av_frame_alloc();
    videoPacket_ = av_packet_alloc();
    if (!videoFrame_ || !rgbFrame_ || !videoPacket_) {
        LOG_ERROR("Failed to allocate video frame or packet.");
        cleanup();
        return false;
    }

    // Setup scaling context
    AVPixelFormat sws_input_pix_fmt = videoCodecContext_->hw_device_ctx && expectedSwFormat_ != AV_PIX_FMT_NONE
        ? expectedSwFormat_ : videoCodecContext_->pix_fmt;

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }

    swsContext_ = sws_getContext(
        videoCodecContext_->width, videoCodecContext_->height, sws_input_pix_fmt,
        width_, height_, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext_) {
        LOG_ERROR("Failed to initialize swscale context for format: " + std::string(av_get_pix_fmt_name(sws_input_pix_fmt)));
        cleanup();
        return false;
    }

    // Allocate RGB buffer
    int min_rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width_, height_, 1);
    if (min_rgb_buffer_size < 0) {
        LOG_ERROR("Failed to get RGB buffer size: " + std::to_string(min_rgb_buffer_size));
        cleanup();
        return false;
    }

    size_t extra_padding = 64;
    rgbBuffer_ = (uint8_t*)av_malloc(min_rgb_buffer_size + extra_padding);
    if (!rgbBuffer_) {
        LOG_ERROR("Failed to allocate RGB frame buffer.");
        cleanup();
        return false;
    }

    ret = av_image_fill_arrays(rgbFrame_->data, rgbFrame_->linesize, rgbBuffer_, AV_PIX_FMT_RGB24, width_, height_, 1);
    if (ret < 0) {
        LOG_ERROR("Failed to fill RGB frame arrays: " + std::to_string(ret));
        cleanup();
        return false;
    }

    rgbFrame_->width = width_;
    rgbFrame_->height = height_;
    rgbFrame_->format = AV_PIX_FMT_RGB24;

    // Create texture
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    // Log renderer info to help diagnose platform-specific renderer/texture issues
    SDL_RendererInfo rinfo;
    if (SDL_GetRendererInfo(renderer_, &rinfo) == 0) {
        LOG_DEBUG(std::string("Renderer info: ") + (rinfo.name ? rinfo.name : "<unknown>") + ", flags=" + std::to_string(rinfo.flags));
    } else {
        LOG_DEBUG(std::string("SDL_GetRendererInfo failed: ") + SDL_GetError());
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (!texture_) {
        LOG_ERROR(std::string("Failed to create texture: ") + SDL_GetError());
        cleanup();
        return false;
    }

    needsReset_ = false;
    LOG_DEBUG("VideoDecoder setup complete: stream index=" + std::to_string(videoStreamIndex_) +
                      ", codec=" + std::string(videoCodec->name));
    return true;
}

void VideoDecoder::play() {
    resetPlaybackTimes();
    LOG_DEBUG("Video playback started.");
}

void VideoDecoder::stop() {
    flush();
    //LOG_DEBUG("Video playback stopped.");
}

void VideoDecoder::update() {
    static bool firstValidFrame = false;

    if (!player_->isPlaying()) return;

    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedPlaybackTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - playbackStartTime_).count();

    double frameDelay = 1.0 / av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->r_frame_rate);
    if (videoClock_ <= elapsedPlaybackTime) {
        if (decodeVideoFrame()) {
            double nextVideoClock = videoFrame_->pts * av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->time_base);
            if (nextVideoClock < videoClock_ || nextVideoClock < 0) {
                videoClock_ += frameDelay;
            } else {
                videoClock_ = nextVideoClock;
            }
            firstValidFrame = true;
            updateTexture();
        } else if (needsReset_) {
            player_->seekToBeginning(videoStreamIndex_);
            flush();
            resetPlaybackTimes();
            needsReset_ = false;
            firstValidFrame = false;
            if (decodeVideoFrame()) {
                videoClock_ = videoFrame_->pts * av_q2d(player_->getFormatContext()->streams[videoStreamIndex_]->time_base);
                if (videoClock_ < 0) videoClock_ = frameDelay;
                firstValidFrame = true;
                updateTexture();
            }
        } else if (!firstValidFrame) {
            videoClock_ += frameDelay;
            LOG_DEBUG("Waiting for first valid frame, advancing clock to " + std::to_string(videoClock_));
        }
    }
}

SDL_Texture* VideoDecoder::getTexture() const {
    return texture_;
}

// Must run on the thread that owns renderer_ and texture_
void VideoDecoder::applyPendingTextureUpdate() {
    // quick check
    if (!hasPendingFrame_.load(std::memory_order_acquire))
        return;

    if (!texture_) {
        LOG_ERROR("applyPendingTextureUpdate: texture_ is null.");
        hasPendingFrame_.store(false, std::memory_order_release);
        return;
    }

    std::vector<uint8_t> localCopy;
    int localPitch = 0;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        if (pendingBuffer_.empty()) {
            hasPendingFrame_.store(false, std::memory_order_release);
            return;
        }
        localCopy.swap(pendingBuffer_); // fast move, pendingBuffer_ becomes empty
        localPitch = pendingPitch_;
        pendingPitch_ = 0;
    }

    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) != 0) {
        LOG_ERROR("applyPendingTextureUpdate: Failed to lock texture: " + std::string(SDL_GetError()) +
                  ", texture=" + std::to_string(reinterpret_cast<uintptr_t>(texture_)) +
                  ", renderer=" + std::to_string(reinterpret_cast<uintptr_t>(renderer_)));
        hasPendingFrame_.store(false, std::memory_order_release);
        return;
    }

    // pitch may be >= localPitch. Copy each row into texture pitch.
    const uint8_t* src = localCopy.data();
    uint8_t* dst = static_cast<uint8_t*>(pixels);
    const int copy_per_row = std::min(pitch, localPitch);

    for (int y = 0; y < height_; ++y) {
        memcpy(dst, src, copy_per_row);
        dst += pitch;
        src += localPitch;
    }

    SDL_UnlockTexture(texture_);
    hasPendingFrame_.store(false, std::memory_order_release);
}

bool VideoDecoder::decodeVideoFrame() {
    static int invalidFrameSkipCount = 0;
    static int badPacketCount = 0;
    const int maxSkipFrames = 30;
    const int maxBadPackets = 5;

    while (player_->isPlaying()) {
        int ret = av_read_frame(player_->getFormatContext(), videoPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_packet_unref(videoPacket_);
                needsReset_ = true;
                invalidFrameSkipCount = 0;
                badPacketCount = 0;
                //LOG_DEBUG("Reached EOF, setting needsReset_.");
                return false;
            }
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
            LOG_ERROR("Error reading video packet: " + std::string(err_buf));
            av_packet_unref(videoPacket_);
            continue;
        }

        if (videoPacket_->stream_index == videoStreamIndex_) {
            ret = avcodec_send_packet(videoCodecContext_, videoPacket_);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_WARN("Error sending video packet: " + std::string(err_buf));
                av_packet_unref(videoPacket_);
                if (videoClock_ < 1.0 && invalidFrameSkipCount < maxSkipFrames) {
                    invalidFrameSkipCount++;
                    badPacketCount++;
                    LOG_DEBUG("Skipping bad packet (count=" + std::to_string(invalidFrameSkipCount) + ")");
                    if (badPacketCount >= maxBadPackets) {
                        LOG_DEBUG("Flushing codec after " + std::to_string(badPacketCount) + " bad packets.");
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
                AVFrame* frameToScale = videoFrame_;
                AVFrame* swFrame = nullptr;

                if (videoFrame_->hw_frames_ctx) {
                    swFrame = av_frame_alloc();
                    if (!swFrame) {
                        LOG_ERROR("Failed to allocate software frame for HW transfer.");
                        continue;
                    }
                    ret = av_hwframe_transfer_data(swFrame, videoFrame_, 0);
                    if (ret < 0) {
                        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                        LOG_WARN("Failed to transfer hardware frame: " + std::string(err_buf));
                        av_frame_free(&swFrame);
                        continue;
                    }
                    frameToScale = swFrame;
                }

                if (!frameToScale->data[0] || frameToScale->width <= 0 || frameToScale->height <= 0 ||
                    frameToScale->format == AV_PIX_FMT_NONE) {
                    LOG_WARN("Invalid frame data: width=" + std::to_string(frameToScale->width) +
                             ", height=" + std::to_string(frameToScale->height) +
                             ", format=" + std::to_string(frameToScale->format));
                    if (swFrame) av_frame_free(&swFrame);
                    invalidFrameSkipCount++;
                    if (videoClock_ < 1.0 && invalidFrameSkipCount < maxSkipFrames) {
                        continue;
                    }
                    LOG_ERROR("Too many invalid frames.");
                    if (swFrame) av_frame_free(&swFrame);
                    return false;
                }

                sws_scale(swsContext_, frameToScale->data, frameToScale->linesize, 0, frameToScale->height,
                          rgbFrame_->data, rgbFrame_->linesize);

                if (swFrame) av_frame_free(&swFrame);
                invalidFrameSkipCount = 0;

                // instead of updateTexture();
                queueFrameForTextureUpdate();
                return true;
            } else if (ret == AVERROR(EAGAIN)) {
                continue;
            } else if (ret == AVERROR_EOF) {
                needsReset_ = true;
                invalidFrameSkipCount = 0;
                badPacketCount = 0;
                LOG_DEBUG("Decoder reached EOF.");
                return false;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_WARN("Error receiving video frame: " + std::string(err_buf));
                invalidFrameSkipCount++;
                if (invalidFrameSkipCount >= maxSkipFrames) {
                    LOG_DEBUG("Too many frame errors, seeking forward.");
                    double target_time = videoClock_ + 2.0;
                    player_->seek(target_time, videoStreamIndex_);
                    flush();
                    resetPlaybackTimes();
                    needsReset_ = false;
                    invalidFrameSkipCount = 0;
                    badPacketCount = 0;
                    return false;
                }
                continue;
            }
        } else {
            av_packet_unref(videoPacket_);
        }
    }
    invalidFrameSkipCount = 0;
    badPacketCount = 0;
    return false;
}

// Called from decoder thread after sws_scale() produced rgbFrame_ (rgbBuffer_)
void VideoDecoder::queueFrameForTextureUpdate() {
    if (!rgbFrame_ || !rgbBuffer_) {
        LOG_ERROR("queueFrameForTextureUpdate: invalid rgb buffer/frame.");
        return;
    }

    const int bytes_per_row = width_ * 3;
    if (rgbFrame_->linesize[0] < bytes_per_row) {
        LOG_ERROR("queueFrameForTextureUpdate: linesize too small: " + std::to_string(rgbFrame_->linesize[0]));
        return;
    }

    size_t requiredSize = static_cast<size_t>(height_) * static_cast<size_t>(bytes_per_row);

    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingBuffer_.resize(requiredSize);
        pendingPitch_ = bytes_per_row;
        uint8_t* dst = pendingBuffer_.data();
        const uint8_t* src = rgbBuffer_;
        for (int y = 0; y < height_; ++y) {
            memcpy(dst, src, bytes_per_row);
            dst += bytes_per_row;
            src += rgbFrame_->linesize[0];
        }
    }

    // publish to main thread
    hasPendingFrame_.store(true, std::memory_order_release);
}

void VideoDecoder::updateTexture() {
    if (!texture_ || !rgbFrame_ || !rgbBuffer_ || !rgbFrame_->data[0] || rgbFrame_->linesize[0] <= 0) {
        LOG_ERROR("Invalid RGB frame data for texture update.");
        return;
    }

    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        SDL_memset(pixels, 0, pitch * height_);
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        const uint8_t* src = rgbBuffer_;
        int bytes_per_row = width_ * 3;
        if (rgbFrame_->linesize[0] < bytes_per_row) {
            LOG_ERROR("Invalid RGB frame linesize: " + std::to_string(rgbFrame_->linesize[0]));
            SDL_UnlockTexture(texture_);
            return;
        }
        for (int y = 0; y < height_; ++y) {
            memcpy(dst, src, bytes_per_row);
            dst += pitch;
            src += rgbFrame_->linesize[0];
        }
        SDL_UnlockTexture(texture_);
        //LOG_DEBUG("Texture updated successfully.");
    } else {
        LOG_ERROR("Failed to lock texture: " + std::string(SDL_GetError()));
    }
}

void VideoDecoder::flush() {
    if (videoCodecContext_) {
        avcodec_flush_buffers(videoCodecContext_);
        //LOG_DEBUG("Codec buffers flushed.");
    }
}

void VideoDecoder::resetPlaybackTimes() {
    videoClock_ = 0.0;
    lastFrameTime_ = std::chrono::high_resolution_clock::time_point();
    playbackStartTime_ = std::chrono::high_resolution_clock::now();
    //LOG_DEBUG("Playback times reset.");
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
        rgbFrame_ = nullptr;
    }
    if (videoFrame_) {
        av_frame_free(&videoFrame_);
        videoFrame_ = nullptr;
    }
    if (videoPacket_) {
        av_packet_free(&videoPacket_);
        videoPacket_ = nullptr;
    }
    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
        videoCodecContext_ = nullptr;
    }
    videoStreamIndex_ = -1;
    needsReset_ = false;
    //LOG_DEBUG("VideoDecoder resources cleaned up.");
}
