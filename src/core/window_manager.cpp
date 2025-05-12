#include "core/window_manager.h"
#include "utils/logging.h"
#include <iostream>

WindowManager::WindowManager(const Settings& settings)
    : playfieldWindow_(nullptr, SDL_DestroyWindow),
      backglassWindow_(nullptr, SDL_DestroyWindow),
      playfieldRenderer_(nullptr, SDL_DestroyRenderer),
      backglassRenderer_(nullptr, SDL_DestroyRenderer) {
    // Create primary window (Playfield)
    int scaledPlayfieldWidth = settings.enableDpiScaling ? 
        static_cast<int>(settings.playfieldWindowWidth * settings.dpiScale) : settings.playfieldWindowWidth;
    int scaledPlayfieldHeight = settings.enableDpiScaling ? 
        static_cast<int>(settings.playfieldWindowHeight * settings.dpiScale) : settings.playfieldWindowHeight;

    playfieldWindow_.reset(SDL_CreateWindow("Playfield",
                                          SDL_WINDOWPOS_CENTERED_DISPLAY(settings.playfieldWindowMonitor),
                                          SDL_WINDOWPOS_CENTERED,
                                          scaledPlayfieldWidth,
                                          scaledPlayfieldHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!playfieldWindow_) {
        LOG_ERROR("Failed to create primary window: " << SDL_GetError());
        exit(1);
    }
    playfieldRenderer_.reset(SDL_CreateRenderer(playfieldWindow_.get(), -1, 
                                              SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!playfieldRenderer_) {
        LOG_ERROR("Failed to create primary renderer: " << SDL_GetError());
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(playfieldRenderer_.get(), SDL_BLENDMODE_BLEND);

    // Create secondary window (Backglass)
    int scaledBackglassWidth = settings.enableDpiScaling ? 
        static_cast<int>(settings.backglassWindowWidth * settings.dpiScale) : settings.backglassWindowWidth;
    int scaledBackglassHeight = settings.enableDpiScaling ? 
        static_cast<int>(settings.backglassWindowHeight * settings.dpiScale) : settings.backglassWindowHeight;

    backglassWindow_.reset(SDL_CreateWindow("Backglass",
                                            SDL_WINDOWPOS_CENTERED_DISPLAY(settings.backglassWindowMonitor),
                                            SDL_WINDOWPOS_CENTERED,
                                            scaledBackglassWidth,
                                            scaledBackglassHeight,
                                            SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!backglassWindow_) {
        LOG_ERROR("Failed to create secondary window: " << SDL_GetError());
        exit(1);
    }
    backglassRenderer_.reset(SDL_CreateRenderer(backglassWindow_.get(), -1, 
                                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!backglassRenderer_) {
        LOG_ERROR("Failed to create secondary renderer: " << SDL_GetError());
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(backglassRenderer_.get(), SDL_BLENDMODE_BLEND);
}
