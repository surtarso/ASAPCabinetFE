#include "core/window_manager.h"
#include "utils/logging.h"
#include <iostream>

WindowManager::WindowManager(const Settings& settings)
    : playfieldWindow_(nullptr, SDL_DestroyWindow),
      backglassWindow_(nullptr, SDL_DestroyWindow),
      dmdWindow_(nullptr, SDL_DestroyWindow),
      playfieldRenderer_(nullptr, SDL_DestroyRenderer),
      backglassRenderer_(nullptr, SDL_DestroyRenderer),
      dmdRenderer_(nullptr, SDL_DestroyRenderer) {
    updateWindows(settings);
}

void WindowManager::updateWindows(const Settings& settings) {
    createOrUpdateWindow(playfieldWindow_, playfieldRenderer_, "Playfield",
                         settings.playfieldWindowMonitor,
                         settings.playfieldWindowWidth,
                         settings.playfieldWindowHeight,
                         settings.dpiScale,
                         settings.enableDpiScaling);

    createOrUpdateWindow(backglassWindow_, backglassRenderer_, "Backglass",
                         settings.backglassWindowMonitor,
                         settings.backglassWindowWidth,
                         settings.backglassWindowHeight,
                         settings.dpiScale,
                         settings.enableDpiScaling);

    createOrUpdateWindow(dmdWindow_, dmdRenderer_, "DMD",
                         settings.dmdWindowMonitor,
                         settings.dmdWindowWidth,
                         settings.dmdWindowHeight,
                         settings.dpiScale,
                         settings.enableDpiScaling);
}

void WindowManager::createOrUpdateWindow(std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>& window,
                                        std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>& renderer,
                                        const char* title, int monitor, int width, int height,
                                        float dpiScale, bool enableDpiScaling) {
    int scaledWidth = enableDpiScaling ? static_cast<int>(width * dpiScale) : width;
    int scaledHeight = enableDpiScaling ? static_cast<int>(height * dpiScale) : height;

    if (window) {
        int currentWidth, currentHeight;
        SDL_GetWindowSize(window.get(), &currentWidth, &currentHeight);
        int currentDisplay = SDL_GetWindowDisplayIndex(window.get());

        if (currentWidth != scaledWidth || currentHeight != scaledHeight || currentDisplay != monitor) {
            SDL_SetWindowSize(window.get(), scaledWidth, scaledHeight);
            SDL_SetWindowPosition(window.get(), SDL_WINDOWPOS_CENTERED_DISPLAY(monitor),
                                 SDL_WINDOWPOS_CENTERED);

            SDL_GetWindowSize(window.get(), &currentWidth, &currentHeight);
            currentDisplay = SDL_GetWindowDisplayIndex(window.get());
            if (currentWidth != scaledWidth || currentHeight != scaledHeight || currentDisplay != monitor) {
                renderer.reset();
                window.reset();
            }
        }
    }

    if (!window) {
        window.reset(SDL_CreateWindow(title,
                                      SDL_WINDOWPOS_CENTERED_DISPLAY(monitor),
                                      SDL_WINDOWPOS_CENTERED,
                                      scaledWidth,
                                      scaledHeight,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
        if (!window) {
            LOG_ERROR("Failed to create " << title << " window: " << SDL_GetError());
            exit(1);
        }

        renderer.reset(SDL_CreateRenderer(window.get(), -1,
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        if (!renderer) {
            LOG_ERROR("Failed to create " << title << " renderer: " << SDL_GetError());
            exit(1);
        }
        SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
    }
}