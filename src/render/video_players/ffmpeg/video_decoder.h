#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <SDL2/SDL.h>
#include <chrono>
#include <string>

// Forward declarations for FFmpeg structs
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class FFmpegPlayer; // Forward declaration

class VideoDecoder {
public:
    VideoDecoder(FFmpegPlayer* player);
    ~VideoDecoder();

    bool setup(AVFormatContext* formatContext, SDL_Renderer* renderer, int width, int height);
    void play();
    void stop();
    void update();
    SDL_Texture* getTexture() const;
    bool decodeVideoFrame();
    void updateTexture();
    void flush();
    void resetPlaybackTimes();

private:
    friend class FFmpegPlayer; // Allow FFmpegPlayer to access private members
    FFmpegPlayer* player_;
    AVCodecContext* videoCodecContext_;
    SDL_Renderer* renderer_;
    int width_;
    int height_;
    AVFrame* videoFrame_;
    AVFrame* rgbFrame_;
    AVPacket* videoPacket_;
    SwsContext* swsContext_;
    int videoStreamIndex_;
    uint8_t* rgbBuffer_;
    SDL_Texture* texture_;
    double videoClock_;
    std::chrono::high_resolution_clock::time_point lastFrameTime_;
    std::chrono::high_resolution_clock::time_point playbackStartTime_;
    bool needsReset_;

    void cleanup();
};

#endif // VIDEO_DECODER_H