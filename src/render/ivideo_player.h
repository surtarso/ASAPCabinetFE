#ifndef IVIDEO_PLAYER_H
#define IVIDEO_PLAYER_H

#include <SDL2/SDL.h>
#include <string>

class IVideoPlayer {
public:
    virtual ~IVideoPlayer() = default;
    virtual bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void update() = 0;
    virtual SDL_Texture* getTexture() const = 0;
    virtual bool isPlaying() const = 0;
};

#endif // IVIDEO_PLAYER_H