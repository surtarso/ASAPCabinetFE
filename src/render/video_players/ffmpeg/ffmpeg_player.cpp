#include "ffmpeg_player.h"
#include "utils/logging.h"
#include <SDL.h>
#include <chrono>
#include <algorithm>
#include <thread> // Keep thread for sleep if needed, but we're removing blocking sleep.
#include <cmath> // For std::log10

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
}

// Global variable to store the FFmpegPlayer instance for the SDL audio callback
static FFmpegPlayer* globalFFmpegPlayerInstance = nullptr;

FFmpegPlayer::FFmpegPlayer()
    : renderer_(nullptr),
      width_(0),
      height_(0),
      isPlaying_(false),
      texture_(nullptr),
      formatContext_(nullptr),
      videoCodecContext_(nullptr),
      videoFrame_(nullptr),
      rgbFrame_(nullptr),
      videoPacket_(nullptr),
      swsContext_(nullptr),
      videoStreamIndex_(-1),
      rgbBuffer_(nullptr),
      videoClock_(0.0),
      lastFrameTime_(),
      playbackStartTime_(),
      audioCodecContext_(nullptr),
      audioFrame_(nullptr),
      audioPacket_(nullptr),
      swrContext_(nullptr),
      audioFifo_(nullptr),
      audioStreamIndex_(-1),
      audioDevice_(0),
      needsVideoDecoderReset_(false),
      currentVolume_(1.0f),
      isMuted_(false)
{
    globalFFmpegPlayerInstance = this;
}

FFmpegPlayer::~FFmpegPlayer() {
    cleanup();
    if (globalFFmpegPlayerInstance == this) {
        globalFFmpegPlayerInstance = nullptr;
    }
}

void FFmpegPlayer::cleanup() {
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 1);
        SDL_CloseAudioDevice(audioDevice_);
        audioDevice_ = 0;
    }

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }

    if (swrContext_) {
        swr_free(&swrContext_);
        swrContext_ = nullptr;
    }

    if (audioFifo_) {
        av_audio_fifo_free(audioFifo_);
        audioFifo_ = nullptr;
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

    if (audioFrame_) {
        av_frame_free(&audioFrame_);
    }

    if (videoPacket_) {
        av_packet_free(&videoPacket_);
    }

    if (audioPacket_) {
        av_packet_free(&audioPacket_);
    }

    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
    }

    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
    }

    if (formatContext_) {
        avformat_close_input(&formatContext_);
        formatContext_ = nullptr;
    }

    needsVideoDecoderReset_ = false;
    renderer_ = nullptr;
    path_.clear();
    width_ = 0;
    height_ = 0;
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
    isPlaying_ = false;
    videoClock_ = 0.0;
    lastFrameTime_ = std::chrono::high_resolution_clock::time_point();
    playbackStartTime_ = std::chrono::high_resolution_clock::time_point();
}

bool FFmpegPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    cleanup();

    renderer_ = renderer;
    path_ = path;
    width_ = width;
    height_ = height;

    if (!renderer_ || path_.empty() || width_ <= 0 || height_ <= 0) {
        LOG_ERROR("FFmpegPlayer: Invalid setup parameters.");
        cleanup();
        return false;
    }

    formatContext_ = avformat_alloc_context();
    if (!formatContext_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate format context.");
        cleanup();
        return false;
    }

    static bool networkInitialized = false;
    if (!networkInitialized) {
        avformat_network_init();
        networkInitialized = true;
    }

    if (avformat_open_input(&formatContext_, path_.c_str(), nullptr, nullptr) < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to open video file: " << path_ << ".");
        cleanup();
        return false;
    }

    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to find stream info.");
        cleanup();
        return false;
    }

    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
    for (unsigned int i = 0; i < formatContext_->nb_streams; ++i) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
        } else if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex_ = i;
        }
    }

    // --- Video Stream Setup ---
    if (videoStreamIndex_ != -1) {
        const AVCodec* videoCodec = avcodec_find_decoder(formatContext_->streams[videoStreamIndex_]->codecpar->codec_id);
        if (!videoCodec) {
            LOG_ERROR("FFmpegPlayer: Video codec not found.");
            cleanup();
            return false;
        }

        videoCodecContext_ = avcodec_alloc_context3(videoCodec);
        if (!videoCodecContext_) {
            LOG_ERROR("FFmpegPlayer: Failed to allocate video codec context.");
            cleanup();
            return false;
        }

        if (avcodec_parameters_to_context(videoCodecContext_, formatContext_->streams[videoStreamIndex_]->codecpar) < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to copy video codec parameters.");
            cleanup();
            return false;
        }

        if (avcodec_open2(videoCodecContext_, videoCodec, nullptr) < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to open video codec.");
            cleanup();
            return false;
        }

        videoFrame_ = av_frame_alloc();
        rgbFrame_ = av_frame_alloc();
        videoPacket_ = av_packet_alloc();
        if (!videoFrame_ || !rgbFrame_ || !videoPacket_) {
            LOG_ERROR("FFmpegPlayer: Failed to allocate video frame or packet.");
            cleanup();
            return false;
        }

        swsContext_ = sws_getContext(
            videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt,
            width_, height_, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext_) {
            LOG_ERROR("FFmpegPlayer: Failed to initialize swscale context.");
            cleanup();
            return false;
        }

        int min_rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width_, height_, 1);
        if (min_rgb_buffer_size < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to get required RGB buffer size: " << min_rgb_buffer_size << ".");
            cleanup();
            return false;
        }

        size_t extra_padding = 64;
        size_t allocated_rgb_buffer_size = min_rgb_buffer_size + extra_padding;

        rgbBuffer_ = (uint8_t*)av_malloc(allocated_rgb_buffer_size);
        if (!rgbBuffer_) {
            LOG_ERROR("FFmpegPlayer: Failed to allocate RGB frame buffer with extra padding!");
            cleanup();
            return false;
        }

        int ret = av_image_fill_arrays(rgbFrame_->data, rgbFrame_->linesize, rgbBuffer_, AV_PIX_FMT_RGB24, width_, height_, 1);
        if (ret < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to fill RGB frame arrays: " << ret << ".");
            cleanup();
            return false;
        }

        rgbFrame_->width = width_;
        rgbFrame_->height = height_;
        rgbFrame_->format = AV_PIX_FMT_RGB24;

        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
        texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!texture_) {
            LOG_ERROR("FFmpegPlayer: Failed to create texture: " << SDL_GetError() << ".");
            cleanup();
            return false;
        }
    } else {
        LOG_INFO("FFmpegPlayer: No video stream found in " << path_ << ". Video will not be displayed.");
    }

    // --- Audio Stream Setup ---
    if (audioStreamIndex_ != -1) {
        const AVCodec* audioCodec = avcodec_find_decoder(formatContext_->streams[audioStreamIndex_]->codecpar->codec_id);
        if (!audioCodec) {
            LOG_ERROR("FFmpegPlayer: Audio codec not found.");
            audioStreamIndex_ = -1;
        } else {
            audioCodecContext_ = avcodec_alloc_context3(audioCodec);
            if (!audioCodecContext_) {
                LOG_ERROR("FFmpegPlayer: Failed to allocate audio codec context.");
                audioStreamIndex_ = -1;
            } else {
                if (avcodec_parameters_to_context(audioCodecContext_, formatContext_->streams[audioStreamIndex_]->codecpar) < 0) {
                    LOG_ERROR("FFmpegPlayer: Failed to copy audio codec parameters.");
                    audioStreamIndex_ = -1;
                } else {
                    if (avcodec_open2(audioCodecContext_, audioCodec, nullptr) < 0) {
                        LOG_ERROR("FFmpegPlayer: Failed to open audio codec.");
                        audioStreamIndex_ = -1;
                    } else {
                        audioFrame_ = av_frame_alloc();
                        audioPacket_ = av_packet_alloc();
                        if (!audioFrame_ || !audioPacket_) {
                            LOG_ERROR("FFmpegPlayer: Failed to allocate audio frame or packet.");
                            audioStreamIndex_ = -1;
                        } else {
                            SDL_AudioSpec wantedSpec;
                            wantedSpec.freq = 44100;
                            wantedSpec.format = AUDIO_S16SYS;
                            wantedSpec.channels = 2;
                            wantedSpec.samples = 1024;
                            wantedSpec.callback = SDLAudioCallback;
                            wantedSpec.userdata = this;

                            audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &wantedSpec, &audioSpec_, 0);
                            if (audioDevice_ == 0) {
                                LOG_ERROR("FFmpegPlayer: Failed to open audio device: " << SDL_GetError() << ".");
                                audioStreamIndex_ = -1;
                            } else {
                                swrContext_ = swr_alloc();
                                if (!swrContext_) {
                                    LOG_ERROR("FFmpegPlayer: Could not allocate resampler context.");
                                    audioStreamIndex_ = -1;
                                } else {
                                    AVChannelLayout in_ch_layout;
                                    if (audioCodecContext_->ch_layout.nb_channels > 0) {
                                        in_ch_layout = audioCodecContext_->ch_layout;
                                    } else {
                                        AVChannelLayout default_layout = AV_CHANNEL_LAYOUT_STEREO;
                                        av_channel_layout_copy(&in_ch_layout, &default_layout);
                                    }

                                    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;

                                    av_opt_set_chlayout(swrContext_, "in_chlayout", &in_ch_layout, 0);
                                    av_opt_set_int(swrContext_, "in_sample_rate", audioCodecContext_->sample_rate, 0);
                                    av_opt_set_sample_fmt(swrContext_, "in_sample_fmt", audioCodecContext_->sample_fmt, 0);

                                    av_opt_set_chlayout(swrContext_, "out_chlayout", &out_ch_layout, 0);
                                    av_opt_set_int(swrContext_, "out_sample_rate", wantedSpec.freq, 0);
                                    av_opt_set_sample_fmt(swrContext_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

                                    if (swr_init(swrContext_) < 0) {
                                        LOG_ERROR("FFmpegPlayer: Could not initialize resampler.");
                                        swr_free(&swrContext_);
                                        audioStreamIndex_ = -1;
                                    } else {
                                        audioFifo_ = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, wantedSpec.channels, 1);
                                        if (!audioFifo_) {
                                            LOG_ERROR("FFmpegPlayer: Could not allocate audio FIFO.");
                                            audioStreamIndex_ = -1;
                                        } else {
                                            SDL_PauseAudioDevice(audioDevice_, 0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        LOG_INFO("FFmpegPlayer: No audio stream found in " << path_ << ". Video will play silently.");
    }

    // Set needsVideoDecoderReset_ to false after successful setup of video decoder
    if (videoStreamIndex_ != -1) {
        needsVideoDecoderReset_ = false;
    }

    if (videoStreamIndex_ == -1 && audioStreamIndex_ == -1) {
        LOG_ERROR("FFmpegPlayer: No video or audio streams found. Cannot play.");
        cleanup();
        return false;
    }

    return true;
}

void FFmpegPlayer::play() {
    if (isPlaying_) return;
    isPlaying_ = true;

    playbackStartTime_ = std::chrono::high_resolution_clock::now();
    videoClock_ = 0.0;

    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 0);
    }
}

void FFmpegPlayer::stop() {
    if (!isPlaying_) return;
    isPlaying_ = false;

    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 1);
        if (audioFifo_) {
            av_audio_fifo_drain(audioFifo_, av_audio_fifo_size(audioFifo_));
        }
    }

    if (formatContext_) {
        if (videoStreamIndex_ != -1) {
            // Instead of just flushing, mark for full reset on next play/update
            needsVideoDecoderReset_ = true;
            // Still seek to 0 and flush for current state, but the flag will ensure deeper reset
            av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(videoCodecContext_);
        }
        if (audioStreamIndex_ != -1) {
            av_seek_frame(formatContext_, audioStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(audioCodecContext_);
        }
    }

    videoClock_ = 0.0;
    lastFrameTime_ = std::chrono::high_resolution_clock::time_point();
    playbackStartTime_ = std::chrono::high_resolution_clock::time_point();
}

void FFmpegPlayer::update() {
    if (!isPlaying_) return;

    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedPlaybackTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - playbackStartTime_).count();

    // --- Video Decoding and Synchronization ---
    if (videoStreamIndex_ != -1 && texture_) {
        // Check if it's time for a new frame OR if the decoder needs to be reset (e.g., after looping)
        if (videoClock_ <= elapsedPlaybackTime || needsVideoDecoderReset_) {
            if (decodeVideoFrame()) {
                videoClock_ = videoFrame_->pts * av_q2d(formatContext_->streams[videoStreamIndex_]->time_base);
                if (videoClock_ < 0) videoClock_ = 0;
                updateTexture();
            } else {
                // If decodeVideoFrame returned false due to AVERROR_EOF (and set needsVideoDecoderReset_)
                // or other non-recoverable error.
                if (needsVideoDecoderReset_) {
                    // This means EOF was reached in decodeVideoFrame, so we loop.
                    av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
                    // No need to call avcodec_flush_buffers here because decodeVideoFrame()
                    // will handle the full codec context reset.
                    playbackStartTime_ = std::chrono::high_resolution_clock::now();
                    videoClock_ = 0.0;
                    needsVideoDecoderReset_ = false; // Reset the flag after handling loop
                } else {
                    // Some other error in decodeVideoFrame, stop playback.
                    isPlaying_ = false;
                }
            }
        }
    }

    // --- Audio Decoding (Fill FIFO buffer) ---
    // ... (this section remains the same, as it seems to be working) ...
    if (audioStreamIndex_ != -1 && audioFifo_) {
        const int targetAudioBufferSamples = audioSpec_.freq * audioSpec_.channels;

        const int maxAudioDecodeAttempts = 5;
        int attempts = 0;
        while (av_audio_fifo_size(audioFifo_) < targetAudioBufferSamples && isPlaying_ && attempts < maxAudioDecodeAttempts) {
            if (!decodeAudioFrame()) {
                if (formatContext_ && audioStreamIndex_ != -1) {
                    av_seek_frame(formatContext_, audioStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(audioCodecContext_);
                } else {
                    break;
                }
            }
            attempts++;
        }
    }
}

SDL_Texture* FFmpegPlayer::getTexture() const {
    return texture_;
}

bool FFmpegPlayer::isPlaying() const {
    return isPlaying_;
}

void FFmpegPlayer::setVolume(float volume) {
    // Input `volume` is 0-100 (from Settings).
    // Normalize to 0.0-1.0. Clamp to prevent issues.
    float normalizedVolume = std::min(std::max(volume / 100.0f, 0.0f), 1.0f);

    // Apply logarithmic scaling for better perceived volume control
    // A common formula: 0 for silent, 1.0 for max.
    // When volume is 0.0, log10(1) = 0.
    // When volume is 1.0, log10(10) = 1.
    // Small offset for near-zero values to avoid log(0) which is undefined.
    float logScaledVolume = 0.0f;
    if (normalizedVolume > 0.0f) {
        // Use a small epsilon to avoid log of 0, or just ensure >0.
        // A simple log scale: log10(val * 9 + 1) / log10(10) maps 0-1 to 0-1 logarithmically.
        logScaledVolume = (std::log10(normalizedVolume * 9.0f + 1.0f) / std::log10(10.0f));
    }
    // Clamp again to be sure it's within 0.0-1.0 after scaling
    currentVolume_ = std::min(std::max(logScaledVolume, 0.0f), 1.0f);


    LOG_DEBUG("FFmpegPlayer: setVolume: Input=" << volume << ", Normalized (linear)=" << normalizedVolume << ", LogScaled=" << currentVolume_);
}

void FFmpegPlayer::setMute(bool mute) {
    isMuted_ = mute;
}

bool FFmpegPlayer::decodeVideoFrame() {
    if (needsVideoDecoderReset_) {
        // ... (existing full decoder reset logic as you have it) ...
        if (videoCodecContext_) { // Ensure it's not null before freeing
            avcodec_free_context(&videoCodecContext_);
            videoCodecContext_ = nullptr;
        }
        const AVCodec* videoCodec = avcodec_find_decoder(formatContext_->streams[videoStreamIndex_]->codecpar->codec_id);
        if (!videoCodec) {
            LOG_ERROR("FFmpegPlayer: Video codec not found during reset.");
            return false;
        }
        videoCodecContext_ = avcodec_alloc_context3(videoCodec);
        if (!videoCodecContext_) {
            LOG_ERROR("FFmpegPlayer: Failed to re-allocate video codec context during reset.");
            return false;
        }
        if (avcodec_parameters_to_context(videoCodecContext_, formatContext_->streams[videoStreamIndex_]->codecpar) < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to re-copy video codec parameters during reset.");
            return false;
        }
        if (avcodec_open2(videoCodecContext_, videoCodec, nullptr) < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to re-open video codec during reset.");
            return false;
        }
        sws_freeContext(swsContext_);
        swsContext_ = sws_getContext(
            videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt,
            width_, height_, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext_) {
            LOG_ERROR("FFmpegPlayer: Failed to re-initialize swscale context during reset.");
            return false;
        }
        needsVideoDecoderReset_ = false;
        LOG_DEBUG("FFmpegPlayer: Video decoder fully reset.");
    }


    while (isPlaying_) {
        int ret = av_read_frame(formatContext_, videoPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_packet_unref(videoPacket_);
                needsVideoDecoderReset_ = true;
                return false;
            }
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
            LOG_ERROR("FFmpegPlayer: Error reading video packet: " << err_buf << ".");
            av_packet_unref(videoPacket_);
            return false;
        }

        if (videoPacket_->stream_index == videoStreamIndex_) {
            ret = avcodec_send_packet(videoCodecContext_, videoPacket_);
            av_packet_unref(videoPacket_); // Unref packet after sending it
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN)) {
                    // Decoder is busy, try to receive frames before sending more packets.
                    // This loop tries to drain the decoder.
                    int receive_ret;
                    while ((receive_ret = avcodec_receive_frame(videoCodecContext_, videoFrame_)) >= 0) {
                        sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height,
                                  rgbFrame_->data, rgbFrame_->linesize);
                        return true; // Successfully decoded a frame by forcing receive
                    }
                    if (receive_ret == AVERROR(EAGAIN)) {
                        return false; // Still no frame, decoder still needs more packets or flushing.
                    } else if (receive_ret == AVERROR_EOF) {
                        needsVideoDecoderReset_ = true; // Signal EOF to loop
                        return false;
                    } else {
                         char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                         av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, receive_ret);
                         LOG_ERROR("FFmpegPlayer: Error receiving frame after send_packet EAGAIN: " << err_buf << ".");
                         return false;
                    }
                } else { // Other critical errors sending packet
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                    LOG_ERROR("FFmpegPlayer: Error sending video packet to decoder: " << err_buf << ".");
                    return false;
                }
            }

            ret = avcodec_receive_frame(videoCodecContext_, videoFrame_);
            if (ret >= 0) {
                sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height,
                          rgbFrame_->data, rgbFrame_->linesize);
                return true;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                continue; // Need more packets or decoder needs flushing, keep trying to read packets
            } else { // Other critical errors receiving frame
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("FFmpegPlayer: Error receiving video frame from decoder: " << err_buf << ".");
                return false;
            }
        }
        av_packet_unref(videoPacket_); // Make sure this is always unref'd if not processed by video stream
    }
    return false;
}

bool FFmpegPlayer::decodeAudioFrame() {
    while (isPlaying_) {
        int ret = av_read_frame(formatContext_, audioPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_packet_unref(audioPacket_);
                return false; // Signify end of stream
            }
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
            LOG_ERROR("FFmpegPlayer: Error reading audio packet: " << err_buf << ".");
            av_packet_unref(audioPacket_);
            return false;
        }

        if (audioPacket_->stream_index == audioStreamIndex_) {
            ret = avcodec_send_packet(audioCodecContext_, audioPacket_);
            av_packet_unref(audioPacket_);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("FFmpegPlayer: Error sending audio packet to decoder: " << err_buf << ".");
                return false;
            }

            ret = avcodec_receive_frame(audioCodecContext_, audioFrame_);
            if (ret >= 0) {
                int out_samples = swr_get_out_samples(swrContext_, audioFrame_->nb_samples);
                if (out_samples < 0) {
                    LOG_ERROR("FFmpegPlayer: Failed to calculate output samples for resampling.");
                    return false;
                }

                uint8_t* out_buffer = nullptr;
                av_samples_alloc(&out_buffer, nullptr, audioSpec_.channels, out_samples, AV_SAMPLE_FMT_S16, 0);

                if (!out_buffer) {
                    LOG_ERROR("FFmpegPlayer: Failed to allocate output buffer for resampling.");
                    return false;
                }

                int converted_samples = swr_convert(swrContext_, &out_buffer, out_samples,
                                                    (const uint8_t**)audioFrame_->data, audioFrame_->nb_samples);
                if (converted_samples < 0) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, converted_samples);
                    LOG_ERROR("FFmpegPlayer: Audio resampling failed: " << err_buf << ".");
                    av_freep(&out_buffer);
                    return false;
                }

                ret = av_audio_fifo_write(audioFifo_, (void**)&out_buffer, converted_samples);
                av_freep(&out_buffer);

                if (ret < 0) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                    LOG_ERROR("FFmpegPlayer: Failed to write to audio FIFO: " << err_buf << ".");
                    return false;
                }
                return true;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                continue;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("FFmpegPlayer: Error receiving audio frame from decoder: " << err_buf << ".");
                return false;
            }
        }
        av_packet_unref(audioPacket_);
    }
    return false;
}

void FFmpegPlayer::updateTexture() {
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
        LOG_ERROR("FFmpegPlayer: Failed to lock texture: " << SDL_GetError() << ".");
    }
}

void FFmpegPlayer::SDLAudioCallback(void* userdata, Uint8* stream, int len) {
    FFmpegPlayer* player = static_cast<FFmpegPlayer*>(userdata);
    if (!player || !player->audioFifo_ || !player->isPlaying_) {
        SDL_memset(stream, 0, len); // Fill buffer with silence
        return;
    }

    SDL_memset(stream, 0, len); // This should zero out the entire buffer first

    if (player->isMuted_) {
        // If muted, return here. The stream is already zeroed by memset.
        LOG_DEBUG("FFmpegPlayer: SDLAudioCallback: Muted, returning silence.");
        return;
    }

    int audio_len_bytes = len;
    int bytes_per_sample = SDL_AUDIO_BITSIZE(player->audioSpec_.format) / 8;
    int audio_len_samples = audio_len_bytes / (player->audioSpec_.channels * bytes_per_sample);

    int read_samples = av_audio_fifo_read(player->audioFifo_, (void**)&stream, audio_len_samples);
    if (read_samples < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, read_samples);
        LOG_ERROR("FFmpegPlayer: SDLAudioCallback: Error reading from audio FIFO: " << err_buf << ".");
        return;
    }

    // Diagnostic: If currentVolume_ is very low, force silence and log.
    if (player->currentVolume_ <= 0.001f) { // Use a small epsilon for float comparison
        SDL_memset(stream, 0, audio_len_bytes); // Force silence if volume is near zero
        LOG_DEBUG("FFmpegPlayer: SDLAudioCallback: Forcing silence due to very low currentVolume_ (" << player->currentVolume_ << ").");
        return; // Return immediately, no mixing needed
    }


    int volume_for_sdl_mixer = static_cast<int>(player->currentVolume_ * SDL_MIX_MAXVOLUME);
    //LOG_DEBUG("FFmpegPlayer: SDLAudioCallback: currentVolume_=" << player->currentVolume_ << ", SDL_MIX_MAXVOLUME=" << SDL_MIX_MAXVOLUME << ", volume_for_sdl_mixer=" << volume_for_sdl_mixer);

    SDL_MixAudioFormat(stream, stream, player->audioSpec_.format, audio_len_bytes, volume_for_sdl_mixer);

    // Diagnostic: Check if stream content is mostly zero after mixing if volume_for_sdl_mixer is 0
    if (volume_for_sdl_mixer == 0) {
        bool all_zero = true;
        for (int i = 0; i < audio_len_bytes; ++i) {
            if (stream[i] != 0) {
                all_zero = false;
                break;
            }
        }
        if (!all_zero) {
            LOG_ERROR("FFmpegPlayer: SDLAudioCallback: volume_for_sdl_mixer is 0, but stream is NOT all zeros after MixAudioFormat!");
        } else {
            LOG_DEBUG("FFmpegPlayer: SDLAudioCallback: volume_for_sdl_mixer is 0, and stream IS all zeros after MixAudioFormat. Expected behavior.");
        }
    }
}