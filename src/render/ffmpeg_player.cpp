#include "render/ffmpeg_player.h"
#include "utils/logging.h"
#include <SDL.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

FFmpegPlayer::FFmpegPlayer()
    : renderer_(nullptr),
      width_(0),
      height_(0),
      isPlaying_(false),
      texture_(nullptr),
      formatContext_(nullptr),
      codecContext_(nullptr),
      frame_(nullptr),
      rgbFrame_(nullptr),
      packet_(nullptr),
      swsContext_(nullptr),
      videoStreamIndex_(-1),
      rgbBuffer_(nullptr) {
    LOG_DEBUG("FFmpegPlayer: Constructor called");
}

FFmpegPlayer::~FFmpegPlayer() {
    cleanup();
    LOG_DEBUG("FFmpegPlayer: Destructor called");
}

bool FFmpegPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    LOG_DEBUG("FFmpegPlayer: Setting up video playback for path=" << path << ", width=" << width << ", height=" << height);
    cleanup(); // Ensure everything is cleaned up from previous usage

    renderer_ = renderer;
    path_ = path;
    width_ = width;
    height_ = height;

    if (!renderer_ || path_.empty() || width_ <= 0 || height_ <= 0) {
        LOG_ERROR("FFmpegPlayer: Invalid setup parameters");
        cleanup(); // Still safe to call cleanup here as pointers will be null
        return false;
    }

    LOG_DEBUG("Allocating formatContext");
    formatContext_ = avformat_alloc_context();
    if (!formatContext_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate format context");
        cleanup(); // Pointers will be null, cleanup is safe
        return false;
    }

    // Initialize network if not already (good for RTSP/HTTP streams)
    static bool networkInitialized = false;
    if (!networkInitialized) {
        avformat_network_init();
        networkInitialized = true;
    }
    //av_log_set_level(AV_LOG_ERROR); // Suppress verbose FFmpeg logs

    // Open input and find stream info using the *same* formatContext_
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
    for (unsigned int i = 0; i < formatContext_->nb_streams; ++i) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
        } else {
            // Explicitly discard non-video streams (like audio)
            formatContext_->streams[i]->discard = AVDISCARD_ALL;
        }
    }
    if (videoStreamIndex_ == -1) {
        LOG_ERROR("FFmpegPlayer: No video stream found");
        cleanup();
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(formatContext_->streams[videoStreamIndex_]->codecpar->codec_id);
    if (!codec) {
        LOG_ERROR("FFmpegPlayer: Codec not found");
        cleanup();
        return false;
    }

    codecContext_ = avcodec_alloc_context3(codec);
    if (!codecContext_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate codec context");
        cleanup();
        return false;
    }

    if (avcodec_parameters_to_context(codecContext_, formatContext_->streams[videoStreamIndex_]->codecpar) < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to copy codec parameters");
        cleanup();
        return false;
    }

    if (avcodec_open2(codecContext_, codec, nullptr) < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to open codec");
        cleanup();
        return false;
    }

    frame_ = av_frame_alloc();
    rgbFrame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    if (!frame_ || !rgbFrame_ || !packet_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate frame or packet");
        cleanup();
        return false;
    }

    swsContext_ = sws_getContext(
        codecContext_->width, codecContext_->height, codecContext_->pix_fmt,
        width_, height_, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext_) {
        LOG_ERROR("FFmpegPlayer: Failed to initialize swscale context");
        cleanup();
        return false;
    }

    int ret = av_image_alloc(rgbFrame_->data, rgbFrame_->linesize, width_, height_, AV_PIX_FMT_RGB24, 1);
    if (ret < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate RGB frame buffer: " << ret);
        cleanup();
        return false;
    }
    rgbFrame_->width = width_;
    rgbFrame_->height = height_;
    rgbFrame_->format = AV_PIX_FMT_RGB24;
    rgbBuffer_ = rgbFrame_->data[0];

    if (texture_) { // Destroy any existing texture before creating a new one
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    LOG_DEBUG("Creating texture: w=" << width << ", h=" << height);
    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture_) {
        LOG_ERROR("Failed to create texture: " << SDL_GetError());
        cleanup();
        return false;
    }

    LOG_DEBUG("Setup complete");
    return true;
}

void FFmpegPlayer::play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    LOG_DEBUG("FFmpegPlayer: Play started");
}

void FFmpegPlayer::stop() {
    if (!isPlaying_) return;
    isPlaying_ = false;
    if (formatContext_ && videoStreamIndex_ >= 0) {
        av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
    }
    LOG_DEBUG("FFmpegPlayer: Stopped and reset to start");
}

void FFmpegPlayer::update() {
    if (!isPlaying_ || !texture_) return;
    if (decodeFrame()) {
        updateTexture();
    }
}

SDL_Texture* FFmpegPlayer::getTexture() const {
    return texture_;
}

bool FFmpegPlayer::isPlaying() const {
    return isPlaying_;
}

bool FFmpegPlayer::decodeFrame() {
    while (isPlaying_) {
        if (av_read_frame(formatContext_, packet_) < 0) {
            LOG_DEBUG("FFmpegPlayer: End of video stream");
            if (formatContext_ && videoStreamIndex_ >= 0) {
                av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(codecContext_);
            }
            return false;
        }

        if (packet_->stream_index == videoStreamIndex_) {
            if (avcodec_send_packet(codecContext_, packet_) >= 0) {
                if (avcodec_receive_frame(codecContext_, frame_) >= 0) {
                    sws_scale(swsContext_, frame_->data, frame_->linesize, 0, codecContext_->height,
                              rgbFrame_->data, rgbFrame_->linesize);
                    av_packet_unref(packet_);
                    return true;
                }
            }
        }
        av_packet_unref(packet_);
    }
    return false;
}

void FFmpegPlayer::updateTexture() {
    if (!texture_ || !rgbFrame_ || !rgbFrame_->data[0]) return;
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        const uint8_t* src = rgbFrame_->data[0];
        int row_size = width_ * 3;
        for (int y = 0; y < height_; ++y) {
            memcpy(dst, src, row_size);
            dst += pitch;
            src += rgbFrame_->linesize[0];
        }
        SDL_UnlockTexture(texture_);
    } else {
        LOG_ERROR("Failed to lock texture: " << SDL_GetError());
    }
}

void FFmpegPlayer::cleanup() {
    LOG_DEBUG("FFmpegPlayer::cleanup() started for path: " << path_);

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
        LOG_DEBUG("  - Destroyed SDL Texture.");
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
        LOG_DEBUG("  - Freed SwsContext.");
    }

    // THIS IS THE CRUCIAL CHANGE:
    // Explicitly free the buffer allocated by av_image_alloc
    // It's important to do this BEFORE av_frame_free(&rgbFrame_)
    // because av_frame_free would nullify rgbFrame_->data, preventing access.
    if (rgbFrame_ && rgbFrame_->data[0]) {
        av_freep(&rgbFrame_->data[0]); // This frees the buffer pointed to by rgbFrame_->data[0]
                                       // and nullifies the pointer within the frame
        rgbBuffer_ = nullptr; // Ensure your own member pointer is also nullified
        LOG_DEBUG("  - Explicitly freed rgbFrame_->data[0] buffer (allocated by av_image_alloc).");
    }

    if (rgbFrame_) {
        av_frame_free(&rgbFrame_); // This frees the AVFrame structure itself
        rgbFrame_ = nullptr;
        LOG_DEBUG("  - Freed RGB AVFrame (structure).");
    }

    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
        LOG_DEBUG("  - Freed AVFrame.");
    }

    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
        LOG_DEBUG("  - Freed AVPacket.");
    }

    if (codecContext_) {
        avcodec_free_context(&codecContext_);
        codecContext_ = nullptr;
        LOG_DEBUG("  - Freed AVCodecContext.");
    }

    if (formatContext_) {
        avformat_close_input(&formatContext_); // This also calls avformat_free_context internally
        formatContext_ = nullptr;
        LOG_DEBUG("  - Closed AVFormatContext.");
    }

    renderer_ = nullptr;
    path_.clear();
    width_ = 0;
    height_ = 0;
    videoStreamIndex_ = -1;

    LOG_DEBUG("FFmpegPlayer::cleanup() complete.");
}