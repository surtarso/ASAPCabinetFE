#ifndef VIDEO_PLAYER_FACTORY_H
#define VIDEO_PLAYER_FACTORY_H

#include "render/ivideo_player.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string>

class IConfigService; // Forward declaration

class VideoPlayerFactory {
public:
    static std::unique_ptr<IVideoPlayer> createVideoPlayer(SDL_Renderer* renderer,
                                                          const std::string& path,
                                                          int width, int height,
                                                          IConfigService* configService);
};

#endif // VIDEO_PLAYER_FACTORY_H