/**
 * @file ffmpeg_player.cpp
 * @brief Implementation of the FFmpegPlayer class for managing video and audio playback in ASAPCabinetFE.
 */

#include "ffmpeg_player.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "log/logging.h"
extern "C" {
#include <libavformat/avformat.h>
}

FFmpegPlayer::FFmpegPlayer()
    : renderer_(nullptr), width_(0), height_(0), isPlaying_(false), formatContext_(nullptr),
      videoDecoder_(new VideoDecoder(this)), audioDecoder_(new AudioDecoder(this)) {}

FFmpegPlayer::~FFmpegPlayer() {
    cleanup();
    delete videoDecoder_;
    delete audioDecoder_;
}

bool FFmpegPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    cleanup();
    renderer_ = renderer;
    path_ = path;
    width_ = width;
    height_ = height;

    if (!renderer_ || path_.empty() || width_ <= 0 || height_ <= 0) {
        LOG_ERROR("Invalid setup parameters: renderer=" + std::to_string(reinterpret_cast<uintptr_t>(renderer)) +
                  ", path=" + path_ + ", width=" + std::to_string(width) + ", height=" + std::to_string(height));
        cleanup();
        return false;
    }

    formatContext_ = avformat_alloc_context();
    if (!formatContext_) {
        LOG_ERROR("Failed to allocate format context.");
        cleanup();
        return false;
    }

    static bool networkInitialized = false;
    if (!networkInitialized) {
        avformat_network_init();
        networkInitialized = true;
    }

    if (avformat_open_input(&formatContext_, path_.c_str(), nullptr, nullptr) < 0) {
        LOG_ERROR("Failed to open video file: " + path_);
        cleanup();
        return false;
    }

    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        LOG_ERROR("Failed to find stream info for: " + path_);
        cleanup();
        return false;
    }

    bool videoSetup = videoDecoder_->setup(formatContext_, renderer_, width_, height_);
    bool audioSetup = audioDecoder_->setup(formatContext_);

    if (!videoSetup && !audioSetup) {
        LOG_ERROR("No video or audio streams found in: " + path_);
        cleanup();
        return false;
    }

    LOG_DEBUG("FFmpegPlayer setup complete for: " + path_);
    return true;
}

void FFmpegPlayer::play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    videoDecoder_->play();
    audioDecoder_->play();
    LOG_DEBUG("Playback started for: " + path_);
}

void FFmpegPlayer::stop() {
    if (!isPlaying_) return;
    isPlaying_ = false;
    videoDecoder_->stop();
    audioDecoder_->stop();

    if (formatContext_) {
        if (videoDecoder_->getTexture()) {
            seekToBeginning(-1);
            videoDecoder_->flush();
        }
        if (audioDecoder_) {
            seekToBeginning(-1);
            audioDecoder_->flush();
        }
    }
    LOG_DEBUG("Playback stopped for: " + path_);
}

void FFmpegPlayer::update() {
    if (!isPlaying_) return;

    // Ensure pending decoded frames are uploaded to texture on the render thread.
    if (videoDecoder_) {
        videoDecoder_->applyPendingTextureUpdate(); // main-thread upload
    }

    // Drive decoders: keep decode logic on whichever thread you run decoders in.
    // If decoder runs in worker thread, this call might be a no-op or control tick.
    videoDecoder_->update();   // may run decoding work (keep it thread-safe)
    audioDecoder_->update();
}

SDL_Texture* FFmpegPlayer::getTexture() const {
    return videoDecoder_->getTexture();
}

bool FFmpegPlayer::isPlaying() const {
    return isPlaying_;
}

void FFmpegPlayer::setVolume(float volume) {
    audioDecoder_->setVolume(volume);
    //LOG_DEBUG("Volume set to: " + std::to_string(volume));
}

void FFmpegPlayer::setMute(bool mute) {
    audioDecoder_->setMute(mute);
    //LOG_DEBUG("Mute set to: " + std::to_string(mute));
}

void FFmpegPlayer::seekToBeginning(int streamIndex) {
    if (formatContext_) {
        int targetStream = streamIndex;
        if (targetStream == -1) {
            for (unsigned int i = 0; i < formatContext_->nb_streams; ++i) {
                if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||
                    formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    int ret = avformat_seek_file(formatContext_, i, INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BACKWARD);
                    if (ret < 0) {
                        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                        LOG_ERROR("Seek failed for stream " + std::to_string(i) + ": " + err_buf);
                    }
                }
            }
        } else {
            int ret = avformat_seek_file(formatContext_, targetStream, INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BACKWARD);
            if (ret < 0) {
                char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
                LOG_ERROR("Seek failed for stream " + std::to_string(targetStream) + ": " + err_buf);
            }
        }
        avformat_flush(formatContext_);
        if (videoDecoder_) {
            videoDecoder_->flush();
        }
        if (audioDecoder_) {
            audioDecoder_->flush();
        }
        //LOG_DEBUG("Seek to beginning completed for stream: " + std::to_string(streamIndex));
    }
}

void FFmpegPlayer::cleanup() {
    videoDecoder_->cleanup();
    audioDecoder_->cleanup();
    if (formatContext_) {
        avformat_close_input(&formatContext_);
        formatContext_ = nullptr;
    }
    //std::string oldPath_ = path_;
    renderer_ = nullptr;
    path_.clear();
    width_ = 0;
    height_ = 0;
    isPlaying_ = false;
    //LOG_DEBUG("FFmpegPlayer resources cleaned up for: " + oldPath_);
}

void FFmpegPlayer::seek(double time_seconds, int stream_index) {
    if (!formatContext_) return;

    // Explicit cast to avoid -Wfloat-conversion warning
    int64_t timestamp = static_cast<int64_t>(time_seconds * static_cast<double>(AV_TIME_BASE));

    int ret = av_seek_frame(formatContext_, stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
        LOG_ERROR("Failed to seek to " + std::to_string(time_seconds) + "s: " + err_buf);
    } else {
        LOG_DEBUG("Successfully sought to " + std::to_string(time_seconds) + "s for stream: " + std::to_string(stream_index));
    }
}
