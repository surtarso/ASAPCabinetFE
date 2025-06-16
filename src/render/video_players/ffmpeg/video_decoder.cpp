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
      needsReset_(false) {
    // hwDeviceCtx_ is removed as a member, so no need to initialize it here.
}

VideoDecoder::~VideoDecoder() {
    cleanup();
}

bool VideoDecoder::setup(AVFormatContext* formatContext, SDL_Renderer* renderer, int width, int height) {
    renderer_ = renderer;
    width_ = width;
    height_ = height;
    int ret;

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

    // --- Hardware Acceleration Attempt and Fallback Logic ---
    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_VAAPI; // Example, make configurable
    AVBufferRef* local_hwDeviceCtx_temp = nullptr; // Use a local temporary pointer
    bool hw_accel_attempted = false; // Renamed for clarity: we *attempted* HW accel

    ret = av_hwdevice_ctx_create(&local_hwDeviceCtx_temp, hwType, NULL, NULL, 0);
    if (ret >= 0) {
        // Attempt to assign the hardware device context to the codec context.
        // The av_buffer_ref() increases the reference count.
        videoCodecContext_->hw_device_ctx = av_buffer_ref(local_hwDeviceCtx_temp);

        // Only mark hw_accel_attempted as true if the hw_device_ctx was successfully referenced and assigned
        if (videoCodecContext_->hw_device_ctx) {
            if (hwType == AV_HWDEVICE_TYPE_VAAPI) {
                expectedSwFormat_ = AV_PIX_FMT_NV12;
            } else if (hwType == AV_HWDEVICE_TYPE_DXVA2) {
                expectedSwFormat_ = AV_PIX_FMT_NV12;
            }
            LOG_INFO("Hardware acceleration device context created: " << av_hwdevice_get_type_name(hwType));
            hw_accel_attempted = true; // Mark that we tried HW accel and it was successfully set
        } else {
            // Failed to reference the buffer, treat as if HW creation failed.
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
            LOG_DEBUG("Failed to reference hardware device context buffer (" << err_buf << "), will proceed with software decoding.");
            expectedSwFormat_ = AV_PIX_FMT_NONE; // Ensure software path uses native format
        }
    } else {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
        LOG_DEBUG("Failed to create hardware device context (" << err_buf << "), will proceed with software decoding.");
        expectedSwFormat_ = AV_PIX_FMT_NONE; // Ensure software path uses native format
    }

    // Always unref the local temporary context. The codec context holds its own reference now.
    if (local_hwDeviceCtx_temp) {
        av_buffer_unref(&local_hwDeviceCtx_temp);
    }

    // Enable error concealment for robust decoding (apply regardless of HW or SW)
    videoCodecContext_->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT;
    videoCodecContext_->err_recognition = AV_EF_EXPLODE | AV_EF_COMPLIANT | AV_EF_CRCCHECK;
    // videoCodecContext_->thread_count = 1; // Explicitly set to 1 to disable multi-threading
    // videoCodecContext_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    // videoCodecContext_->skip_frame = AVDISCARD_NONREF;

    // Try to open the codec with potential hardware acceleration (if hw_accel_attempted is true)
    int open_ret = avcodec_open2(videoCodecContext_, videoCodec, nullptr);

    // If opening with HW failed, and we actually attempted HW, then clean up and retry in SW.
    if (open_ret < 0 && hw_accel_attempted) { // No need to check videoCodecContext_->hw_device_ctx here directly, hw_accel_attempted implies it was set.
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, open_ret);
        LOG_ERROR("VideoDecoder: Failed to open video codec with hardware acceleration (" << err_buf << "). Attempting software fallback.");

        // Clean up hardware context on the videoCodecContext_
        // This drops the reference held by the codec context.
        if (videoCodecContext_->hw_device_ctx) { // Check before unref'ing to be safe
            av_buffer_unref(&videoCodecContext_->hw_device_ctx);
            videoCodecContext_->hw_device_ctx = nullptr; // Crucially, also nullify the context's HW reference
        }
        expectedSwFormat_ = AV_PIX_FMT_NONE; // Ensure software format is used for sws_getContext

        // Try to open the codec again, this time *without* hardware acceleration
        open_ret = avcodec_open2(videoCodecContext_, videoCodec, nullptr);
    }

    if (open_ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, open_ret);
        LOG_ERROR("VideoDecoder: Failed to open video codec even with software decoding (" << err_buf << ").");
        cleanup();
        return false;
    }
    // --- End Hardware Acceleration Attempt and Fallback Logic ---


    videoFrame_ = av_frame_alloc();
    rgbFrame_ = av_frame_alloc();
    videoPacket_ = av_packet_alloc();
    if (!videoFrame_ || !rgbFrame_ || !videoPacket_) {
        LOG_ERROR("VideoDecoder: Failed to allocate video frame or packet.");
        cleanup();
        return false;
    }

    // Determine the pixel format that will be input to sws_scale
    AVPixelFormat sws_input_pix_fmt;
    // Check videoCodecContext_->hw_device_ctx directly now that hwDeviceCtx_ is gone
    if (videoCodecContext_->hw_device_ctx != nullptr && expectedSwFormat_ != AV_PIX_FMT_NONE) {
        // If HW acceleration is active, frames will be transferred to this software format (e.g., NV12 for VAAPI)
        sws_input_pix_fmt = expectedSwFormat_;
    } else {
        // Otherwise, use the native pixel format of the video stream (software decoding)
        sws_input_pix_fmt = videoCodecContext_->pix_fmt;
    }

    // Free any existing swsContext_ before creating a new one (important if setup is called multiple times)
    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }

    swsContext_ = sws_getContext(
        videoCodecContext_->width, videoCodecContext_->height, sws_input_pix_fmt, // Use the determined input format
        width_, height_, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext_) {
        LOG_ERROR("VideoDecoder: Failed to initialize swscale context for format " << av_get_pix_fmt_name(sws_input_pix_fmt) << ".");
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

    ret = av_image_fill_arrays(rgbFrame_->data, rgbFrame_->linesize, rgbBuffer_, AV_PIX_FMT_RGB24, width_, height_, 1);
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
    const int maxSkipFrames = 30; // Increased tolerance for bad frames
    const int maxBadPackets = 5;  // Also increase this for more flexibility with bad packets

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
                AVFrame* frameToScale = videoFrame_; // Assume videoFrame_ is what we'll scale initially

                // If it's a hardware frame, transfer it to a software frame
                if (videoFrame_->hw_frames_ctx) {
                    AVFrame* swFrame = av_frame_alloc();
                    if (!swFrame) {
                        LOG_ERROR("Failed to allocate software frame for HW transfer.");
                        // DO NOT av_frame_free(&videoFrame_) here, as it's a member and managed by avcodec_receive_frame.
                        // Instead, just continue to skip this problematic frame.
                        continue;
                    }
                    ret = av_hwframe_transfer_data(swFrame, videoFrame_, 0);
                    if (ret < 0) {
                        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                        LOG_ERROR("Failed to transfer hardware frame to software: " << err_buf);
                        av_frame_free(&swFrame); // Free the newly allocated swFrame on failure
                        continue; // Skip this bad frame
                    }
                    frameToScale = swFrame; // Now scale the software copy
                }

                // Validate the frame *before* scaling
                // Use frameToScale's properties for validation, as it's the actual source for sws_scale
                if (!frameToScale->data[0] || frameToScale->width <= 0 || frameToScale->height <= 0 ||
                    frameToScale->format == AV_PIX_FMT_NONE) {
                    LOG_ERROR("VideoDecoder: Invalid frame data after potential HW transfer (width=" << frameToScale->width
                              << ", height=" << frameToScale->height << ", format=" << frameToScale->format << "). Skipping.");
                    if (frameToScale != videoFrame_) { // Free swFrame if it was allocated
                        av_frame_free(&frameToScale);
                    }
                    invalidFrameSkipCount++; // Increment skip count for truly invalid frames
                    if (videoClock_ < 1.0 && invalidFrameSkipCount < maxSkipFrames) {
                        continue; // Skip this and try next frame
                    }
                    LOG_ERROR("VideoDecoder: Too many invalid frames, giving up.");
                    return false; // Give up after too many skips
                }

                // Now, scale the (potentially transferred) software frame
                sws_scale(swsContext_, frameToScale->data, frameToScale->linesize, 0, frameToScale->height,
                          rgbFrame_->data, rgbFrame_->linesize);

                if (frameToScale != videoFrame_) {
                    av_frame_free(&frameToScale); // Free the temporary software frame if it was created
                }

                invalidFrameSkipCount = 0; // Reset skip count on a successful frame
                return true; // Successfully decoded and scaled a frame
            } else if (ret == AVERROR(EAGAIN)) {
                // Not enough data to decode a frame, need more packets. Continue loop.
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

                invalidFrameSkipCount++;
                LOG_DEBUG("VideoDecoder: Skipping error frame (count=" << invalidFrameSkipCount << ").");

                if (invalidFrameSkipCount >= maxSkipFrames) {
                    LOG_DEBUG("VideoDecoder: Too many consecutive frame errors. Attempting to seek forward.");

                    // Calculate a target time slightly ahead of current clock, e.g., +2 seconds
                    double target_time = videoClock_ + 2.0; // Seek 2 seconds ahead
                    // Assuming FFmpegPlayer::seek is implemented as discussed previously
                    player_->seek(target_time, videoStreamIndex_);
                    flush(); // Flush the decoder after seeking
                    resetPlaybackTimes(); // Reset clock after seek
                    needsReset_ = false; // We just sought, no need for the update() reset
                    invalidFrameSkipCount = 0; // Reset skip count after recovery attempt
                    badPacketCount = 0; // Reset bad packet count
                    return false; // Exit to let update() process the new state
                }
                continue; // Keep trying to receive frames from the current packet queue
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
        // This implicitly unrefs videoCodecContext_->hw_device_ctx if it was set
        avcodec_free_context(&videoCodecContext_);
    }
    // hwDeviceCtx_ is no longer a member variable, so no need to unref it here.
    videoStreamIndex_ = -1;
    needsReset_ = false;
}