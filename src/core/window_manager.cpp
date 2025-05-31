#include "core/window_manager.h"
#include "utils/logging.h"
#include <iostream>

WindowManager::WindowManager(const Settings& settings)
    : playfieldWindow_(nullptr, SDL_DestroyWindow),
      backglassWindow_(nullptr, SDL_DestroyWindow),
      dmdWindow_(nullptr, SDL_DestroyWindow),
      topperWindow_(nullptr, SDL_DestroyWindow),
      playfieldRenderer_(nullptr, SDL_DestroyRenderer),
      backglassRenderer_(nullptr, SDL_DestroyRenderer),
      dmdRenderer_(nullptr, SDL_DestroyRenderer),
      topperRenderer_(nullptr, SDL_DestroyRenderer) {
    updateWindows(settings);
}

void WindowManager::updateWindows(const Settings& settings) {
    WindowInfo windows[] = {
        {playfieldWindow_, playfieldRenderer_, "Playfield", true,
         settings.playfieldWindowWidth, settings.playfieldWindowHeight,
         settings.playfieldX, settings.playfieldY},
        {backglassWindow_, backglassRenderer_, "Backglass", settings.showBackglass,
         settings.backglassWindowWidth, settings.backglassWindowHeight,
         settings.backglassX, settings.backglassY},
        {dmdWindow_, dmdRenderer_, "DMD", settings.showDMD,
         settings.dmdWindowWidth, settings.dmdWindowHeight,
         settings.dmdX, settings.dmdY},
        {topperWindow_, topperRenderer_, "Topper", settings.showTopper,
         settings.topperWindowWidth, settings.topperWindowHeight,
         settings.topperWindowX, settings.topperWindowY}
    };

    for (auto& w : windows) {
        bool update = false;
        if (w.show != !!w.window) {
            update = true;
            // LOG_DEBUG("WindowManager: " << w.title << " needs update due to visibility change: " << w.show);
        } else if (w.window) {
            int currentWidth, currentHeight, currentX, currentY;
            SDL_GetWindowSize(w.window.get(), &currentWidth, &currentHeight);
            SDL_GetWindowPosition(w.window.get(), &currentX, &currentY);
            int scaledWidth = settings.enableDpiScaling ? static_cast<int>(w.width * settings.dpiScale) : w.width;
            int scaledHeight = settings.enableDpiScaling ? static_cast<int>(w.height * settings.dpiScale) : w.height;
            if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
                currentX != w.x || currentY != w.y) {
                update = true;
                LOG_DEBUG("WindowManager: " << w.title << " needs update - "
                          << "width: " << currentWidth << "->" << scaledWidth
                          << ", height: " << currentHeight << "->" << scaledHeight
                          << ", x: " << currentX << "->" << w.x
                          << ", y: " << currentY << "->" << w.y);
            }
        } else {
            update = w.show;
        }

        if (update) {
            // LOG_DEBUG("WindowManager: Updating " << w.title << " window");
            if (w.show) {
                createOrUpdateWindow(w.window, w.renderer, w.title,
                                     w.width, w.height,
                                     w.x, w.y,
                                     settings.dpiScale,
                                     settings.enableDpiScaling);
            } else {
                w.renderer.reset();
                w.window.reset();
            }
        }
    }
}

void WindowManager::createOrUpdateWindow(std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>& window,
                                        std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>& renderer,
                                        const char* title,
                                        int width, int height,
                                        int posX, int posY,
                                        float dpiScale, bool enableDpiScaling) {
    int scaledWidth = enableDpiScaling ? static_cast<int>(width * dpiScale) : width;
    int scaledHeight = enableDpiScaling ? static_cast<int>(height * dpiScale) : height;

    if (window) {
        int currentWidth, currentHeight, currentX, currentY;
        SDL_GetWindowSize(window.get(), &currentWidth, &currentHeight);
        SDL_GetWindowPosition(window.get(), &currentX, &currentY);

        if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
            currentX != posX || currentY != posY) {
            SDL_SetWindowSize(window.get(), scaledWidth, scaledHeight);
            SDL_SetWindowPosition(window.get(), posX, posY);

            int actualWidth, actualHeight, actualX, actualY;
            SDL_GetWindowSize(window.get(), &actualWidth, &actualHeight);
            SDL_GetWindowPosition(window.get(), &actualX, &actualY);
            if (actualWidth != scaledWidth || actualHeight != scaledHeight ||
                actualX != posX || actualY != posY) {
                LOG_DEBUG("WindowManager: Window " << title << " reset due to mismatch - "
                          << "width: " << actualWidth << "!=" << scaledWidth
                          << ", height: " << actualHeight << "!=" << scaledHeight
                          << ", x: " << actualX << "!=" << posX
                          << ", y: " << actualY << "!=" << posY);
                renderer.reset();
                window.reset();
            }
        }
    }

    if (!window) {
        window.reset(SDL_CreateWindow(title, posX, posY, scaledWidth, scaledHeight,
                                      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
        if (!window) {
            LOG_ERROR("WindowManager: Failed to create " << title << " window: " << SDL_GetError());
            exit(1);
        }

        renderer.reset(SDL_CreateRenderer(window.get(), -1,
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        if (!renderer) {
            LOG_ERROR("WindowManager: Failed to create renderer for " << title << ": " << SDL_GetError());
            exit(1);
        }
        SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
    }
}

void WindowManager::getWindowPositions(int& playfieldX, int& playfieldY, int& backglassX, int& backglassY,
                                       int& dmdX, int& dmdY, int& topperX, int& topperY) {
    if (playfieldWindow_) {
        SDL_GetWindowPosition(playfieldWindow_.get(), &playfieldX, &playfieldY);
    }
    if (backglassWindow_) {
        SDL_GetWindowPosition(backglassWindow_.get(), &backglassX, &backglassY);
    }
    if (dmdWindow_) {
        SDL_GetWindowPosition(dmdWindow_.get(), &dmdX, &dmdY);
    }
    if (topperWindow_) {
        SDL_GetWindowPosition(topperWindow_.get(), &topperX, &topperY);
    }
}