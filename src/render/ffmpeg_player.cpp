#include "render/ffmpeg_player.h"
#include "utils/logging.h"
#include <SDL.h>
#include <chrono> // For timing video frames
#include <algorithm> // Required for std::min and std::max

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h> // For av_opt_set_int
#include <libswresample/swresample.h> // For audio resampling
#include <libavutil/audio_fifo.h> // For audio buffering
#include <libavutil/channel_layout.h> // For modern channel layout APIs
#include <libavutil/error.h> // For av_make_error_string
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
      audioCodecContext_(nullptr),
      audioFrame_(nullptr),
      audioPacket_(nullptr),
      swrContext_(nullptr),
      audioFifo_(nullptr),
      audioStreamIndex_(-1),
      audioDevice_(0),
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
    LOG_DEBUG("FFmpegPlayer::cleanup() started for path: " << path_);

    if (audioDevice_ != 0) {
        SDL_CloseAudioDevice(audioDevice_);
        audioDevice_ = 0;
        LOG_DEBUG("FFmpegPlayer:   - Closed SDL audio device.");
    }

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
        LOG_DEBUG("FFmpegPlayer:   - Destroyed SDL Texture.");
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
        LOG_DEBUG("FFmpegPlayer:   - Freed SwsContext.");
    }

    if (swrContext_) {
        swr_free(&swrContext_);
        swrContext_ = nullptr;
        LOG_DEBUG("FFmpegPlayer:   - Freed SwrContext.");
    }

    if (audioFifo_) {
        av_audio_fifo_free(audioFifo_);
        audioFifo_ = nullptr;
        LOG_DEBUG("FFmpegPlayer:   - Freed AVAudioFifo.");
    }

    if (rgbBuffer_) {
        av_freep(&rgbBuffer_);
        LOG_DEBUG("FFmpegPlayer:   - Explicitly freed rgbBuffer_.");
    }

    if (rgbFrame_) {
        av_frame_free(&rgbFrame_);
        LOG_DEBUG("FFmpegPlayer:   - Freed RGB AVFrame (structure).");
    }

    if (videoFrame_) {
        av_frame_free(&videoFrame_);
        LOG_DEBUG("FFmpegPlayer:   - Freed Video AVFrame.");
    }

    if (audioFrame_) {
        av_frame_free(&audioFrame_);
        LOG_DEBUG("FFmpegPlayer:   - Freed Audio AVFrame.");
    }

    if (videoPacket_) {
        av_packet_free(&videoPacket_);
        LOG_DEBUG("FFmpegPlayer:   - Freed Video AVPacket.");
    }

    if (audioPacket_) {
        av_packet_free(&audioPacket_);
        LOG_DEBUG("FFmpegPlayer:   - Freed Audio AVPacket.");
    }

    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
        LOG_DEBUG("FFmpegPlayer:   - Freed Video AVCodecContext.");
    }

    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
        LOG_DEBUG("FFmpegPlayer:   - Freed Audio AVCodecContext.");
    }

    if (formatContext_) {
        avformat_close_input(&formatContext_);
        formatContext_ = nullptr;
        LOG_DEBUG("FFmpegPlayer:   - Closed AVFormatContext.");
    }

    renderer_ = nullptr;
    path_.clear();
    width_ = 0;
    height_ = 0;
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
    isPlaying_ = false;

    LOG_DEBUG("FFmpegPlayer::cleanup() complete.");
}

bool FFmpegPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    LOG_DEBUG("FFmpegPlayer: Setting up video playback for path=" << path << ", width=" << width << ", height=" << height);
    cleanup();

    renderer_ = renderer;
    path_ = path;
    width_ = width;
    height_ = height;

    if (!renderer_ || path_.empty() || width_ <= 0 || height_ <= 0) {
        LOG_ERROR("FFmpegPlayer: Invalid setup parameters");
        cleanup();
        return false;
    }

    formatContext_ = avformat_alloc_context();
    if (!formatContext_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate format context");
        cleanup();
        return false;
    }

    static bool networkInitialized = false;
    if (!networkInitialized) {
        avformat_network_init();
        networkInitialized = true;
    }

    if (avformat_open_input(&formatContext_, path_.c_str(), nullptr, nullptr) < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to open video file: " << path_);
        cleanup();
        return false;
    }

    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to find stream info");
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
            LOG_ERROR("FFmpegPlayer: Video codec not found");
            cleanup();
            return false;
        }

        videoCodecContext_ = avcodec_alloc_context3(videoCodec);
        if (!videoCodecContext_) {
            LOG_ERROR("FFmpegPlayer: Failed to allocate video codec context");
            cleanup();
            return false;
        }

        if (avcodec_parameters_to_context(videoCodecContext_, formatContext_->streams[videoStreamIndex_]->codecpar) < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to copy video codec parameters");
            cleanup();
            return false;
        }

        if (avcodec_open2(videoCodecContext_, videoCodec, nullptr) < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to open video codec");
            cleanup();
            return false;
        }

        videoFrame_ = av_frame_alloc();
        rgbFrame_ = av_frame_alloc();
        videoPacket_ = av_packet_alloc();
        if (!videoFrame_ || !rgbFrame_ || !videoPacket_) {
            LOG_ERROR("FFmpegPlayer: Failed to allocate video frame or packet");
            cleanup();
            return false;
        }

        swsContext_ = sws_getContext(
            videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt,
            width_, height_, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext_) {
            LOG_ERROR("FFmpegPlayer: Failed to initialize swscale context");
            cleanup();
            return false;
        }

        int min_rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width_, height_, 1);
        if (min_rgb_buffer_size < 0) {
            LOG_ERROR("FFmpegPlayer: Failed to get required RGB buffer size: " << min_rgb_buffer_size);
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
            LOG_ERROR("FFmpegPlayer: Failed to fill RGB frame arrays: " << ret);
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
            LOG_ERROR("FFmpegPlayer: Failed to create texture: " << SDL_GetError());
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
            LOG_ERROR("FFmpegPlayer: Audio codec not found");
            audioStreamIndex_ = -1;
        } else {
            audioCodecContext_ = avcodec_alloc_context3(audioCodec);
            if (!audioCodecContext_) {
                LOG_ERROR("FFmpegPlayer: Failed to allocate audio codec context");
                audioStreamIndex_ = -1;
            } else {
                if (avcodec_parameters_to_context(audioCodecContext_, formatContext_->streams[audioStreamIndex_]->codecpar) < 0) {
                    LOG_ERROR("FFmpegPlayer: Failed to copy audio codec parameters");
                    audioStreamIndex_ = -1;
                } else {
                    if (avcodec_open2(audioCodecContext_, audioCodec, nullptr) < 0) {
                        LOG_ERROR("FFmpegPlayer: Failed to open audio codec");
                        audioStreamIndex_ = -1;
                    } else {
                        audioFrame_ = av_frame_alloc();
                        audioPacket_ = av_packet_alloc();
                        if (!audioFrame_ || !audioPacket_) {
                            LOG_ERROR("FFmpegPlayer: Failed to allocate audio frame or packet");
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
                                LOG_ERROR("FFmpegPlayer: Failed to open audio device: " << SDL_GetError());
                                audioStreamIndex_ = -1;
                            } else {
                                swrContext_ = swr_alloc();
                                if (!swrContext_) {
                                    LOG_ERROR("FFmpegPlayer: Could not allocate resampler context");
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
                                        LOG_ERROR("FFmpegPlayer: Could not initialize resampler");
                                        swr_free(&swrContext_);
                                        audioStreamIndex_ = -1;
                                    } else {
                                        audioFifo_ = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, wantedSpec.channels, 1);
                                        if (!audioFifo_) {
                                            LOG_ERROR("FFmpegPlayer: Could not allocate audio FIFO");
                                            audioStreamIndex_ = -1;
                                        } else {
                                            SDL_PauseAudioDevice(audioDevice_, 0);
                                            LOG_DEBUG("FFmpegPlayer: Audio stream setup complete.");
                                        }
                                    }
                                    if (audioCodecContext_->ch_layout.nb_channels <= 0) {
                                        av_channel_layout_uninit(&in_ch_layout);
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

    if (videoStreamIndex_ == -1 && audioStreamIndex_ == -1) {
        LOG_ERROR("FFmpegPlayer: No video or audio streams found. Cannot play.");
        cleanup();
        return false;
    }

    LOG_DEBUG("FFmpegPlayer: Setup complete");
    return true;
}

void FFmpegPlayer::play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 0);
    }
    LOG_DEBUG("FFmpegPlayer: Play started");
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

    if (formatContext_ && videoStreamIndex_ >= 0) {
        av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(videoCodecContext_);
    }
    if (formatContext_ && audioStreamIndex_ >= 0) {
        av_seek_frame(formatContext_, audioStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(audioCodecContext_);
    }
    LOG_DEBUG("FFmpegPlayer: Stopped and reset to start");
}

void FFmpegPlayer::update() {
    if (!isPlaying_) return;

    if (videoStreamIndex_ != -1 && texture_) {
        decodeVideoFrame();
        updateTexture();
    }

    if (audioStreamIndex_ != -1 && audioFifo_) {
        // Decode audio until FIFO has at least 4x the requested samples to prevent underruns
        while (av_audio_fifo_size(audioFifo_) < audioSpec_.samples * audioSpec_.channels * 4) {
            if (!decodeAudioFrame()) {
                break; // No more audio frames or error
            }
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
    currentVolume_ = std::min(std::max(volume, 0.0f), 1.0f);
    LOG_DEBUG("FFmpegPlayer: Volume set to " << currentVolume_);
}

void FFmpegPlayer::setMute(bool mute) {
    isMuted_ = mute;
    LOG_DEBUG("FFmpegPlayer: Mute set to " << (isMuted_ ? "true" : "false"));
}

bool FFmpegPlayer::decodeVideoFrame() {
    while (isPlaying_) {
        int ret = av_read_frame(formatContext_, videoPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if (formatContext_ && videoStreamIndex_ >= 0) {
                    av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(videoCodecContext_);
                }
                return false;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                std::string err_str = err_buf;
                LOG_ERROR("FFmpegPlayer: Error reading video frame: " << err_str);
                return false;
            }
        }

        if (videoPacket_->stream_index == videoStreamIndex_) {
            ret = avcodec_send_packet(videoCodecContext_, videoPacket_);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                std::string err_str = err_buf;
                LOG_ERROR("FFmpegPlayer: Error sending video packet to decoder: " << err_str);
                av_packet_unref(videoPacket_);
                return false;
            }

            ret = avcodec_receive_frame(videoCodecContext_, videoFrame_);
            if (ret >= 0) {
                sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height,
                          rgbFrame_->data, rgbFrame_->linesize);
                av_packet_unref(videoPacket_);
                return true;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(videoPacket_);
                continue;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                std::string err_str = err_buf;
                LOG_ERROR("FFmpegPlayer: Error receiving video frame from decoder: " << err_str);
                av_packet_unref(videoPacket_);
                return false;
            }
        }
        av_packet_unref(videoPacket_);
    }
    return false;
}

bool FFmpegPlayer::decodeAudioFrame() {
    while (isPlaying_) {
        int ret = av_read_frame(formatContext_, audioPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if (formatContext_ && audioStreamIndex_ != -1) {
                    av_seek_frame(formatContext_, audioStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(audioCodecContext_);
                }
                return false;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                std::string err_str = err_buf;
                LOG_ERROR("FFmpegPlayer: Error reading audio frame: " << err_str);
                return false;
            }
        }

        if (audioPacket_->stream_index == audioStreamIndex_) {
            ret = avcodec_send_packet(audioCodecContext_, audioPacket_);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                std::string err_str = err_buf;
                LOG_ERROR("FFmpegPlayer: Error sending audio packet to decoder: " << err_str);
                av_packet_unref(audioPacket_);
                return false;
            }

            ret = avcodec_receive_frame(audioCodecContext_, audioFrame_);
            if (ret >= 0) {
                // Calculate required output buffer size
                int out_samples = swr_get_out_samples(swrContext_, audioFrame_->nb_samples);
                if (out_samples < 0) {
                    LOG_ERROR("FFmpegPlayer: Failed to calculate output samples for resampling.");
                    av_packet_unref(audioPacket_);
                    return false;
                }

                // Allocate output buffer for resampled audio
                int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                int out_buffer_size = out_samples * audioSpec_.channels * bytes_per_sample;
                uint8_t* out_buffer = (uint8_t*)av_malloc(out_buffer_size);
                if (!out_buffer) {
                    LOG_ERROR("FFmpegPlayer: Failed to allocate output buffer for resampling.");
                    av_packet_unref(audioPacket_);
                    return false;
                }

                // Resample audio
                int converted_samples = swr_convert(swrContext_, &out_buffer, out_samples,
                                                    (const uint8_t**)audioFrame_->data, audioFrame_->nb_samples);
                if (converted_samples < 0) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, converted_samples);
                    std::string err_str = err_buf;
                    LOG_ERROR("FFmpegPlayer: Audio resampling failed: " << err_str);
                    av_freep(&out_buffer);
                    av_packet_unref(audioPacket_);
                    return false;
                }

                // Write resampled audio to FIFO
                ret = av_audio_fifo_write(audioFifo_, (void**)&out_buffer, converted_samples);
                if (ret < 0) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                    std::string err_str = err_buf;
                    LOG_ERROR("FFmpegPlayer: Failed to write to audio FIFO: " << err_str);
                    av_freep(&out_buffer);
                    av_packet_unref(audioPacket_);
                    return false;
                }

                av_freep(&out_buffer);
                av_packet_unref(audioPacket_);
                return true;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(audioPacket_);
                continue;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                std::string err_str = err_buf;
                LOG_ERROR("FFmpegPlayer: Error receiving audio frame from decoder: " << err_str);
                av_packet_unref(audioPacket_);
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
        LOG_ERROR("FFmpegPlayer: Failed to lock texture: " << SDL_GetError());
    }
}

void FFmpegPlayer::SDLAudioCallback(void* userdata, Uint8* stream, int len) {
    FFmpegPlayer* player = static_cast<FFmpegPlayer*>(userdata);
    if (!player || !player->audioFifo_ || !player->isPlaying_) {
        memset(stream, 0, len);
        return;
    }

    SDL_memset(stream, 0, len);

    int audio_len_bytes = len;
    int audio_len_samples = audio_len_bytes / (player->audioSpec_.channels * (SDL_AUDIO_BITSIZE(player->audioSpec_.format) / 8));

    if (player->isMuted_) {
        return;
    }

    int fifo_size = av_audio_fifo_size(player->audioFifo_);
    if (fifo_size < audio_len_samples) {
        LOG_DEBUG("FFmpegPlayer: Audio FIFO underrun. Requested " << audio_len_samples << " samples, got " << fifo_size);
    }

    int read_samples = av_audio_fifo_read(player->audioFifo_, (void**)&stream, audio_len_samples);
    if (read_samples < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, read_samples);
        std::string err_str = err_buf;
        LOG_ERROR("FFmpegPlayer: Error reading from audio FIFO: " << err_str);
        return;
    }

    if (player->currentVolume_ < 1.0f) {
        SDL_MixAudioFormat(stream, stream, player->audioSpec_.format, read_samples * player->audioSpec_.channels * (SDL_AUDIO_BITSIZE(player->audioSpec_.format) / 8), static_cast<int>(player->currentVolume_ * SDL_MIX_MAXVOLUME));
    }
}