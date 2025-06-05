#include "ffmpeg_player.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "utils/logging.h"
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

    bool videoSetup = videoDecoder_->setup(formatContext_, renderer_, width_, height_);
    bool audioSetup = audioDecoder_->setup(formatContext_);

    if (!videoSetup && !audioSetup) {
        LOG_ERROR("FFmpegPlayer: No video or audio streams found. Cannot play.");
        cleanup();
        return false;
    }

    return true;
}

void FFmpegPlayer::play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    videoDecoder_->play();
    audioDecoder_->play();
}

void FFmpegPlayer::stop() {
    if (!isPlaying_) return;
    isPlaying_ = false;
    videoDecoder_->stop();
    audioDecoder_->stop();

    if (formatContext_) {
        if (videoDecoder_->getTexture()) {
            seekToBeginning(-1); // Seek video stream
            videoDecoder_->flush();
        }
        if (audioDecoder_) {
            seekToBeginning(-1); // Seek audio stream
            audioDecoder_->flush();
        }
    }
}

void FFmpegPlayer::update() {
    if (!isPlaying_) return;
    videoDecoder_->update();
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
}

void FFmpegPlayer::setMute(bool mute) {
    audioDecoder_->setMute(mute);
}

void FFmpegPlayer::seekToBeginning(int streamIndex) {
    if (formatContext_) {
        int targetStream = streamIndex;
        if (targetStream == -1) {
            for (unsigned int i = 0; i < formatContext_->nb_streams; ++i) {
                if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||
                    formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    av_seek_frame(formatContext_, i, 0, AVSEEK_FLAG_BACKWARD);
                }
            }
        } else {
            av_seek_frame(formatContext_, targetStream, 0, AVSEEK_FLAG_BACKWARD);
        }
    }
}

void FFmpegPlayer::cleanup() {
    videoDecoder_->cleanup();
    audioDecoder_->cleanup();
    if (formatContext_) {
        avformat_close_input(&formatContext_);
        formatContext_ = nullptr;
    }
    renderer_ = nullptr;
    path_.clear();
    width_ = 0;
    height_ = 0;
    isPlaying_ = false;
}