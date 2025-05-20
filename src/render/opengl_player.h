#ifndef OPENGL_PLAYER_H
#define OPENGL_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL.h>
#include <string>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class OpenGLPlayer : public IVideoPlayer {
public:
    OpenGLPlayer();
    ~OpenGLPlayer() override;

    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;
    void play() override;
    void stop() override;
    void update() override;
    SDL_Texture* getTexture() const override;
    bool isPlaying() const override;

private:
    SDL_Renderer* renderer_;
    std::string path_;
    int width_;
    int height_;
    bool isPlaying_;
    SDL_Texture* texture_;

    // FFmpeg context
    AVFormatContext* formatContext_;
    AVCodecContext* codecContext_;
    AVFrame* frame_;
    AVFrame* rgbFrame_;
    AVPacket* packet_;
    SwsContext* swsContext_;
    int videoStreamIndex_;
    uint8_t* rgbBuffer_;

    void cleanup();
    bool decodeFrame();
    void updateTexture();
};

#endif // OPENGL_PLAYER_H