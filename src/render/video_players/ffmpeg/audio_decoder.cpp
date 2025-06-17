#include "audio_decoder.h"
#include "ffmpeg_player.h"
#include "log/logging.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}
#include <cmath>

AudioDecoder::AudioDecoder(FFmpegPlayer* player)
    : player_(player), audioCodecContext_(nullptr), audioFrame_(nullptr), audioPacket_(nullptr),
      swrContext_(nullptr), audioFifo_(nullptr), audioStreamIndex_(-1), audioDevice_(0),
      currentVolume_(1.0f), isMuted_(false) {}

AudioDecoder::~AudioDecoder() {
    cleanup();
}

bool AudioDecoder::setup(AVFormatContext* formatContext) {
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex_ = i;
            break;
        }
    }
    if (audioStreamIndex_ == -1) {
        LOG_DEBUG("AudioDecoder: No audio stream found.");
        return false;
    }

    const AVCodec* audioCodec = avcodec_find_decoder(formatContext->streams[audioStreamIndex_]->codecpar->codec_id);
    if (!audioCodec) {
        LOG_ERROR("AudioDecoder: Audio codec not found.");
        cleanup();
        return false;
    }

    audioCodecContext_ = avcodec_alloc_context3(audioCodec);
    if (!audioCodecContext_) {
        LOG_ERROR("AudioDecoder: Failed to allocate audio codec context.");
        cleanup();
        return false;
    }

    if (avcodec_parameters_to_context(audioCodecContext_, formatContext->streams[audioStreamIndex_]->codecpar) < 0) {
        LOG_ERROR("AudioDecoder: Failed to copy audio codec parameters.");
        cleanup();
        return false;
    }

    if (avcodec_open2(audioCodecContext_, audioCodec, nullptr) < 0) {
        LOG_ERROR("AudioDecoder: Failed to open audio codec.");
        cleanup();
        return false;
    }

    audioFrame_ = av_frame_alloc();
    audioPacket_ = av_packet_alloc();
    if (!audioFrame_ || !audioPacket_) {
        LOG_ERROR("AudioDecoder: Failed to allocate audio frame or packet.");
        cleanup();
        return false;
    }

    SDL_AudioSpec wantedSpec;
    wantedSpec.freq = 44100;
    wantedSpec.format = AUDIO_S16SYS;
    wantedSpec.channels = 2;
    wantedSpec.samples = 1024;
    wantedSpec.callback = SDLAudioCallback;
    wantedSpec.userdata = this;

    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &wantedSpec, &audioSpec_, 0);
    if (audioDevice_ == 0) {
        LOG_ERROR("AudioDecoder: Failed to open audio device: " << SDL_GetError() << ".");
        cleanup();
        return false;
    }

    swrContext_ = swr_alloc();
    if (!swrContext_) {
        LOG_ERROR("AudioDecoder: Could not allocate resampler context.");
        cleanup();
        return false;
    }

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
        LOG_ERROR("AudioDecoder: Could not initialize resampler.");
        swr_free(&swrContext_);
        cleanup();
        return false;
    }

    audioFifo_ = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, wantedSpec.channels, 1);
    if (!audioFifo_) {
        LOG_ERROR("AudioDecoder: Could not allocate audio FIFO.");
        cleanup();
        return false;
    }

    SDL_PauseAudioDevice(audioDevice_, 0);
    return true;
}

void AudioDecoder::play() {
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 0);
    }
}

void AudioDecoder::stop() {
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 1);
        if (audioFifo_) {
            av_audio_fifo_drain(audioFifo_, av_audio_fifo_size(audioFifo_));
        }
    }
    flush();
}

void AudioDecoder::update() {
    if (!player_->isPlaying() || !audioFifo_) return;

    const int targetAudioBufferSamples = audioSpec_.freq * audioSpec_.channels;
    const int maxAudioDecodeAttempts = 5;
    int attempts = 0;
    while (av_audio_fifo_size(audioFifo_) < targetAudioBufferSamples && player_->isPlaying() && attempts < maxAudioDecodeAttempts) {
        if (!decodeAudioFrame()) {
            player_->seekToBeginning(audioStreamIndex_);
            flush();
        }
        attempts++;
    }
}

bool AudioDecoder::decodeAudioFrame() {
    while (player_->isPlaying()) {
        int ret = av_read_frame(player_->getFormatContext(), audioPacket_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                av_packet_unref(audioPacket_);
                return false;
            }
            char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
            LOG_ERROR("AudioDecoder: Error reading audio packet: " << err_buf << ".");
            av_packet_unref(audioPacket_);
            return false;
        }

        if (audioPacket_->stream_index == audioStreamIndex_) {
            ret = avcodec_send_packet(audioCodecContext_, audioPacket_);
            av_packet_unref(audioPacket_);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("AudioDecoder: Error sending audio packet to decoder: " << err_buf << ".");
                return false;
            }

            ret = avcodec_receive_frame(audioCodecContext_, audioFrame_);
            if (ret >= 0) {
                int out_samples = swr_get_out_samples(swrContext_, audioFrame_->nb_samples);
                if (out_samples < 0) {
                    LOG_ERROR("AudioDecoder: Failed to calculate output samples for resampling.");
                    return false;
                }

                uint8_t* out_buffer = nullptr;
                av_samples_alloc(&out_buffer, nullptr, audioSpec_.channels, out_samples, AV_SAMPLE_FMT_S16, 0);
                if (!out_buffer) {
                    LOG_ERROR("AudioDecoder: Failed to allocate output buffer for resampling.");
                    return false;
                }

                int converted_samples = swr_convert(swrContext_, &out_buffer, out_samples,
                                                    (const uint8_t**)audioFrame_->data, audioFrame_->nb_samples);
                if (converted_samples < 0) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, converted_samples);
                    LOG_ERROR("AudioDecoder: Audio resampling failed: " << err_buf << ".");
                    av_freep(&out_buffer);
                    return false;
                }

                ret = av_audio_fifo_write(audioFifo_, (void**)&out_buffer, converted_samples);
                av_freep(&out_buffer);
                if (ret < 0) {
                    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                    LOG_ERROR("AudioDecoder: Failed to write to audio FIFO: " << err_buf << ".");
                    return false;
                }
                return true;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                continue;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("AudioDecoder: Error receiving audio frame from decoder: " << err_buf << ".");
                return false;
            }
        }
        av_packet_unref(audioPacket_);
    }
    return false;
}

void AudioDecoder::fillAudioStream(Uint8* stream, int len) {
    if (!audioFifo_ || !player_->isPlaying()) {
        SDL_memset(stream, 0, len);
        return;
    }

    SDL_memset(stream, 0, len);
    if (isMuted_) {
        LOG_DEBUG("AudioDecoder: Muted, returning silence.");
        return;
    }

    int audio_len_bytes = len;
    int bytes_per_sample = SDL_AUDIO_BITSIZE(audioSpec_.format) / 8;
    int audio_len_samples = audio_len_bytes / (audioSpec_.channels * bytes_per_sample);

    int read_samples = av_audio_fifo_read(audioFifo_, (void**)&stream, audio_len_samples);
    if (read_samples < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, read_samples);
        LOG_ERROR("AudioDecoder: Error reading from audio FIFO: " << err_buf << ".");
        return;
    }

    if (currentVolume_ <= 0.001f) {
        SDL_memset(stream, 0, audio_len_bytes);
        LOG_DEBUG("AudioDecoder: Forcing silence due to very low currentVolume_ (" << currentVolume_ << ").");
        return;
    }

    int volume_for_sdl_mixer = static_cast<int>(currentVolume_ * SDL_MIX_MAXVOLUME);
    SDL_MixAudioFormat(stream, stream, audioSpec_.format, audio_len_bytes, volume_for_sdl_mixer);
}

void AudioDecoder::SDLAudioCallback(void* userdata, Uint8* stream, int len) {
    AudioDecoder* decoder = static_cast<AudioDecoder*>(userdata);
    decoder->fillAudioStream(stream, len);
}

void AudioDecoder::setVolume(float volume) {
    float normalizedVolume = std::min(std::max(volume / 100.0f, 0.0f), 1.0f);
    float logScaledVolume = 0.0f;
    if (normalizedVolume > 0.0f) {
        logScaledVolume = (std::log10(normalizedVolume * 9.0f + 1.0f) / std::log10(10.0f));
    }
    currentVolume_ = std::min(std::max(logScaledVolume, 0.0f), 1.0f);
    LOG_DEBUG("AudioDecoder: setVolume: Input=" << volume << ", Normalized (linear)=" << normalizedVolume << ", LogScaled=" << currentVolume_);
}

void AudioDecoder::setMute(bool mute) {
    isMuted_ = mute;
}

void AudioDecoder::flush() {
    if (audioCodecContext_) {
        avcodec_flush_buffers(audioCodecContext_);
    }
}

void AudioDecoder::cleanup() {
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 1);
        SDL_CloseAudioDevice(audioDevice_);
        audioDevice_ = 0;
    }
    if (swrContext_) {
        swr_free(&swrContext_);
        swrContext_ = nullptr;
    }
    if (audioFifo_) {
        av_audio_fifo_free(audioFifo_);
        audioFifo_ = nullptr;
    }
    if (audioFrame_) {
        av_frame_free(&audioFrame_);
    }
    if (audioPacket_) {
        av_packet_free(&audioPacket_);
    }
    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
    }
    audioStreamIndex_ = -1;
}