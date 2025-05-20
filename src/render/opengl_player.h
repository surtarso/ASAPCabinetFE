#ifndef OPENGL_PLAYER_H
#define OPENGL_PLAYER_H

#include "render/ivideo_player.h"
#include <string>

class OpenGLPlayer : public IVideoPlayer {
public:
    OpenGLPlayer() = default;
    ~OpenGLPlayer() override = default;

    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;
    void play() override;
    void stop() override;
    void update() override;
    SDL_Texture* getTexture() const override;
    bool isPlaying() const override;

private:
    bool isPlaying_ = false;
};

#endif // OPENGL_PLAYER_H