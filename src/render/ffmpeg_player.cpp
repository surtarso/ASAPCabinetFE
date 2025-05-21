#include "render/ffmpeg_player.h"
#include "utils/logging.h"
#include <SDL.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h> // For av_malloc
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
      rgbBuffer_(nullptr) { // Initialize rgbBuffer_ to nullptr
    //LOG_DEBUG("FFmpegPlayer: Constructor called");
}

FFmpegPlayer::~FFmpegPlayer() {
    cleanup();
    //LOG_DEBUG("FFmpegPlayer: Destructor called");
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

    //LOG_DEBUG("FFmpegPlayer: Allocating formatContext");
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

    //LOG_DEBUG("FFmpegPlayer: Codec Context dimensions - width=" << codecContext_->width << ", height=" << codecContext_->height << ", pix_fmt=" << av_get_pix_fmt_name(codecContext_->pix_fmt));

    frame_ = av_frame_alloc();
    rgbFrame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    if (!frame_ || !rgbFrame_ || !packet_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate frame or packet");
        cleanup();
        return false;
    }

    // LOG_DEBUG("FFmpegPlayer: sws_getContext params: src_w=" << codecContext_->width << ", src_h=" << codecContext_->height << ", src_pix_fmt=" << av_get_pix_fmt_name(codecContext_->pix_fmt)
    //       << ", dst_w=" << width_ << ", dst_h=" << height_ << ", dst_pix_fmt=" << av_get_pix_fmt_name(AV_PIX_FMT_RGB24));

    swsContext_ = sws_getContext(
        codecContext_->width, codecContext_->height, codecContext_->pix_fmt,
        width_, height_, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext_) {
        LOG_ERROR("FFmpegPlayer: Failed to initialize swscale context");
        cleanup();
        return false;
    }

    // --- START PROPOSED CHANGE FOR RGB BUFFER ALLOCATION ---
    // Calculate the minimum required buffer size for the RGB frame
    int min_rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width_, height_, 1);
    if (min_rgb_buffer_size < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to get required RGB buffer size: " << min_rgb_buffer_size);
        cleanup();
        return false;
    }

    // Add extra padding to account for potential overruns by sws_scale's optimized routines.
    // A padding of 64 bytes is a common safe margin, aligning to cache lines/SIMD registers.
    size_t extra_padding = 64; 
    size_t allocated_rgb_buffer_size = min_rgb_buffer_size + extra_padding;

    // Allocate the buffer using av_malloc to ensure FFmpeg's preferred alignment
    rgbBuffer_ = (uint8_t*)av_malloc(allocated_rgb_buffer_size);
    if (!rgbBuffer_) {
        LOG_ERROR("FFmpegPlayer: Failed to allocate RGB frame buffer with extra padding!");
        cleanup();
        return false;
    }

    // Fill the rgbFrame_->data and rgbFrame_->linesize arrays to point to the new buffer.
    // Use the *original* width/height for this, not the padded size, as this describes the image.
    int ret = av_image_fill_arrays(rgbFrame_->data, rgbFrame_->linesize, rgbBuffer_, AV_PIX_FMT_RGB24, width_, height_, 1);
    if (ret < 0) {
        LOG_ERROR("FFmpegPlayer: Failed to fill RGB frame arrays: " << ret);
        cleanup();
        return false;
    }

    // Log the actual linesize FFmpeg determined for the RGB frame
    // LOG_DEBUG("FFmpegPlayer: Manually allocated RGB buffer with size " << allocated_rgb_buffer_size
    //           << ". rgbFrame_ linesize[0]=" << rgbFrame_->linesize[0] << ", Expected pixel data size=" << min_rgb_buffer_size);

    // Set other rgbFrame_ properties for consistency
    rgbFrame_->width = width_;
    rgbFrame_->height = height_;
    rgbFrame_->format = AV_PIX_FMT_RGB24;
    // rgbFrame_->data[0] now points to rgbBuffer_, no need for separate assignment for usage by sws_scale
    // --- END PROPOSED CHANGE ---

    if (texture_) { // Destroy any existing texture before creating a new one
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    //LOG_DEBUG("FFmpegPlayer: Creating texture: w=" << width << ", h=" << height);
    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    
    int texWidth, texHeight;
    Uint32 texFormat;
    SDL_QueryTexture(texture_, &texFormat, nullptr, &texWidth, &texHeight);
    //LOG_DEBUG("FFmpegPlayer: SDL Texture created: width=" << texWidth << ", height=" << texHeight << ", format=" << SDL_GetPixelFormatName(texFormat));

    if (!texture_) {
        LOG_ERROR("FFmpegPlayer: Failed to create texture: " << SDL_GetError());
        cleanup();
        return false;
    }

    //LOG_DEBUG("FFmpegPlayer: Setup complete");
    return true;
}

void FFmpegPlayer::play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    //LOG_DEBUG("FFmpegPlayer: Play started");
}

void FFmpegPlayer::stop() {
    if (!isPlaying_) return;
    isPlaying_ = false;
    if (formatContext_ && videoStreamIndex_ >= 0) {
        // Seek to the beginning of the video
        // AVSEEK_FLAG_BACKWARD is often used to ensure keyframe is found before target
        av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
        // Flush codec buffers to ensure no old frames are returned after seek
        avcodec_flush_buffers(codecContext_);
    }
    //LOG_DEBUG("FFmpegPlayer: Stopped and reset to start");
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
        int ret = av_read_frame(formatContext_, packet_);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                // End of file reached, loop the video
                //LOG_DEBUG("FFmpegPlayer: End of video stream, looping.");
                if (formatContext_ && videoStreamIndex_ >= 0) {
                    av_seek_frame(formatContext_, videoStreamIndex_, 0, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(codecContext_);
                }
                return false; // No new frame decoded in this pass, so return false
            } else {
                //LOG_ERROR("FFmpegPlayer: Error reading frame: " << av_err2str(ret));
                return false;
            }
        }

        if (packet_->stream_index == videoStreamIndex_) {
            ret = avcodec_send_packet(codecContext_, packet_);
            if (ret < 0) {
                //LOG_ERROR("FFmpegPlayer: Error sending packet to decoder: " << av_err2str(ret));
                av_packet_unref(packet_);
                return false;
            }
            
            ret = avcodec_receive_frame(codecContext_, frame_);
            if (ret >= 0) { // Successfully received a frame
                sws_scale(swsContext_, frame_->data, frame_->linesize, 0, codecContext_->height,
                          rgbFrame_->data, rgbFrame_->linesize);
                av_packet_unref(packet_); // Packet is consumed, unref it
                return true; // Successfully decoded and converted a frame
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                // Decoder needs more packets or end of stream reached, continue loop to read more
                // Don't unref packet yet if EAGAIN, it means it wasn't consumed.
                // However, the typical pattern is to unref after avcodec_send_packet regardless of EAGAIN,
                // as the packet is assumed 'sent'.
                av_packet_unref(packet_);
                continue; // Try reading another packet
            } else {
                //LOG_ERROR("FFmpegPlayer: Error receiving frame from decoder: " << av_err2str(ret));
                av_packet_unref(packet_);
                return false;
            }
        }
        av_packet_unref(packet_); // Unref packet if it's not the video stream or not consumed by send_packet
    }
    return false;
}

void FFmpegPlayer::updateTexture() {
    if (!texture_ || !rgbFrame_ || !rgbBuffer_) return; // Check rgbBuffer_ instead of rgbFrame_->data[0] for clarity

    void* pixels;
    int pitch; // This is the pitch (stride) of the SDL_Texture
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        const uint8_t* src = rgbBuffer_; // Use rgbBuffer_ directly

        // The actual byte width of one line of RGB24 data
        int bytes_per_pixel_row = width_ * 3;

        // Iterate through each row and copy.
        // Use the smaller of 'bytes_per_pixel_row' and 'pitch' for the memcpy amount
        // to prevent writing past the SDL texture's row buffer.
        // Use FFmpeg's linesize to advance the source pointer.
        // Use SDL's pitch to advance the destination pointer.
        for (int y = 0; y < height_; ++y) {
            memcpy(dst, src, bytes_per_pixel_row);
            dst += pitch; // Advance destination pointer by SDL's texture pitch
            src += rgbFrame_->linesize[0]; // Advance source pointer by FFmpeg's calculated linesize
        }
        SDL_UnlockTexture(texture_);
    } else {
        LOG_ERROR("FFmpegPlayer: Failed to lock texture: " << SDL_GetError());
    }
}

void FFmpegPlayer::cleanup() {
    // Add a check to prevent logging a potentially invalid path_ if cleanup is called very early
    // or after path_ has been cleared.
    if (!path_.empty()) {
        //LOG_DEBUG("FFmpegPlayer::cleanup() started for path: " << path_);
    }
    // else {
    //     LOG_DEBUG("FFmpegPlayer::cleanup() started (path empty).");
    // }

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Destroyed SDL Texture.");
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Freed SwsContext.");
    }

    // Free the rgbBuffer_ memory, which was allocated by av_malloc.
    // This MUST happen before av_frame_free(&rgbFrame_) if you used av_image_fill_arrays.
    if (rgbBuffer_) {
        av_freep(&rgbBuffer_); // av_freep takes a pointer to the pointer, and nullifies it.
        // After this, rgbFrame_->data[0] will also be null if it pointed to rgbBuffer_.
        // LOG_DEBUG("FFmpegPlayer:   - Explicitly freed rgbBuffer_ (allocated by av_malloc).");
    }

    if (rgbFrame_) {
        // No need to free rgbFrame_->data here if rgbBuffer_ was freed.
        // av_frame_free will free the AVFrame structure itself,
        // and any internal buffers it allocated if av_frame_get_buffer was used.
        // Since we used av_image_fill_arrays, the data pointer was external and freed above.
        av_frame_free(&rgbFrame_); 
        rgbFrame_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Freed RGB AVFrame (structure).");
    }

    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Freed AVFrame.");
    }

    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Freed AVPacket.");
    }

    if (codecContext_) {
        avcodec_free_context(&codecContext_);
        codecContext_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Freed AVCodecContext.");
    }

    if (formatContext_) {
        avformat_close_input(&formatContext_); // This also calls avformat_free_context internally
        formatContext_ = nullptr;
        // LOG_DEBUG("FFmpegPlayer:   - Closed AVFormatContext.");
    }

    renderer_ = nullptr;
    path_.clear(); // Clear path_ string at the end of cleanup
    width_ = 0;
    height_ = 0;
    videoStreamIndex_ = -1;

    //LOG_DEBUG("FFmpegPlayer::cleanup() complete.");
}