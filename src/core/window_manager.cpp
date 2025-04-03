#include "core/window_manager.h"
#include "utils/logging.h"
#include <iostream>

WindowManager::WindowManager(const Settings& settings)
    : primaryWindow_(nullptr, SDL_DestroyWindow),
      secondaryWindow_(nullptr, SDL_DestroyWindow),
      primaryRenderer_(nullptr, SDL_DestroyRenderer),
      secondaryRenderer_(nullptr, SDL_DestroyRenderer) {
    // Create primary window (Playfield)
    int scaledMainWidth = settings.enableDpiScaling ? 
        static_cast<int>(settings.mainWindowWidth * settings.dpiScale) : settings.mainWindowWidth;
    int scaledMainHeight = settings.enableDpiScaling ? 
        static_cast<int>(settings.mainWindowHeight * settings.dpiScale) : settings.mainWindowHeight;

    primaryWindow_.reset(SDL_CreateWindow("Playfield",
                                          SDL_WINDOWPOS_CENTERED_DISPLAY(settings.mainWindowMonitor),
                                          SDL_WINDOWPOS_CENTERED,
                                          scaledMainWidth,
                                          scaledMainHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!primaryWindow_) {
        LOG_ERROR("Failed to create primary window: " << SDL_GetError());
        exit(1);
    }
    primaryRenderer_.reset(SDL_CreateRenderer(primaryWindow_.get(), -1, 
                                              SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!primaryRenderer_) {
        LOG_ERROR("Failed to create primary renderer: " << SDL_GetError());
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(primaryRenderer_.get(), SDL_BLENDMODE_BLEND);

    // Create secondary window (Backglass)
    int scaledSecondWidth = settings.enableDpiScaling ? 
        static_cast<int>(settings.secondWindowWidth * settings.dpiScale) : settings.secondWindowWidth;
    int scaledSecondHeight = settings.enableDpiScaling ? 
        static_cast<int>(settings.secondWindowHeight * settings.dpiScale) : settings.secondWindowHeight;

    secondaryWindow_.reset(SDL_CreateWindow("Backglass",
                                            SDL_WINDOWPOS_CENTERED_DISPLAY(settings.secondWindowMonitor),
                                            SDL_WINDOWPOS_CENTERED,
                                            scaledSecondWidth,
                                            scaledSecondHeight,
                                            SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!secondaryWindow_) {
        LOG_ERROR("Failed to create secondary window: " << SDL_GetError());
        exit(1);
    }
    secondaryRenderer_.reset(SDL_CreateRenderer(secondaryWindow_.get(), -1, 
                                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!secondaryRenderer_) {
        LOG_ERROR("Failed to create secondary renderer: " << SDL_GetError());
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(secondaryRenderer_.get(), SDL_BLENDMODE_BLEND);
}
