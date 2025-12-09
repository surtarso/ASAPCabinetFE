/**
 * @file window_manager.cpp
 * @brief Implementation of the WindowManager class for managing SDL windows and renderers.
 */

#include "core/window_manager.h"
#include "log/logging.h"
#include <SDL2/SDL.h>
#include <string>

WindowManager::WindowManager(const Settings& settings)
    : playfieldWindow_(nullptr, SDL_DestroyWindow),
      backglassWindow_(nullptr, SDL_DestroyWindow),
      dmdWindow_(nullptr, SDL_DestroyWindow),
      topperWindow_(nullptr, SDL_DestroyWindow),
      playfieldRenderer_(nullptr, SDL_DestroyRenderer),
      backglassRenderer_(nullptr, SDL_DestroyRenderer),
      dmdRenderer_(nullptr, SDL_DestroyRenderer),
      topperRenderer_(nullptr, SDL_DestroyRenderer) {
        forceSoftwareRenderer_ = settings.forceSoftwareRenderer;
        updateWindows(settings);
}

void WindowManager::updateWindows(const Settings& settings) {
    // Keep local copy of the software-renderer preference for subsequent window creations
    forceSoftwareRenderer_ = settings.forceSoftwareRenderer;
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
            // LOG_DEBUG("Needs update for " + std::string(w.title) + " due to visibility change: " + (w.show ? "true" : "false"));
        } else if (w.window) {
            int currentWidth, currentHeight, currentX, currentY;
            SDL_GetWindowSize(w.window.get(), &currentWidth, &currentHeight);
            SDL_GetWindowPosition(w.window.get(), &currentX, &currentY);
            int scaledWidth = settings.enableDpiScaling ? static_cast<int>(static_cast<float>(w.width) * settings.dpiScale) : w.width;
            int scaledHeight = settings.enableDpiScaling ? static_cast<int>(static_cast<float>(w.height) * settings.dpiScale) : w.height;
            if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
                currentX != w.x || currentY != w.y) {
                update = true;
                LOG_DEBUG(std::string(w.title) + " needs update - width: " + std::to_string(currentWidth) + "->" + std::to_string(scaledWidth) +
                          ", height: " + std::to_string(currentHeight) + "->" + std::to_string(scaledHeight) +
                          ", x: " + std::to_string(currentX) + "->" + std::to_string(w.x) +
                          ", y: " + std::to_string(currentY) + "->" + std::to_string(w.y));
            }
        } else {
            update = w.show;
        }

        if (update) {
            LOG_DEBUG("Updating " + std::string(w.title) + " window");
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

void WindowManager::createOrUpdateWindow(
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>& window,
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>& renderer,
    const char* title,
    int width, int height,
    int posX, int posY,
    float dpiScale, bool enableDpiScaling)
{
    float scaledWidthF  = enableDpiScaling ? static_cast<float>(width) * dpiScale  : static_cast<float>(width);
    float scaledHeightF = enableDpiScaling ? static_cast<float>(height) * dpiScale : static_cast<float>(height);

    int scaledWidth  = static_cast<int>(scaledWidthF);
    int scaledHeight = static_cast<int>(scaledHeightF);

#if defined(__APPLE__)
    // On macOS Retina, force accelerated renderer with target texture for sharp drawing
    // Uint32 sdlWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;
    Uint32 sdlWindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI; // No need for OpenGL flag for mac?
    window.reset(SDL_CreateWindow(title, posX, posY, scaledWidth, scaledHeight, sdlWindowFlags));
    if (!window) {
        LOG_ERROR("Failed to create " + std::string(title) + " window: " + std::string(SDL_GetError()));
        exit(1);
    }

    // Prefer accelerated with target texture
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE;
    renderer.reset(SDL_CreateRenderer(window.get(), -1, rendererFlags));
    if (!renderer) {
        LOG_WARN("Accelerated renderer failed on macOS, falling back to software: " + std::string(SDL_GetError()));
        renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC));
        if (!renderer) {
            LOG_ERROR("Failed to create renderer for " + std::string(title) + " on macOS: " + std::string(SDL_GetError()));
            exit(1);
        }
    }

#else
    // Existing cross-platform logic for Linux / Wayland
    if (window) {
        int currentWidth, currentHeight, currentX, currentY;
        SDL_GetWindowSize(window.get(), &currentWidth, &currentHeight);
        SDL_GetWindowPosition(window.get(), &currentX, &currentY);
        if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
            currentX != posX || currentY != posY) {
            SDL_SetWindowSize(window.get(), scaledWidth, scaledHeight);
            SDL_SetWindowPosition(window.get(), posX, posY);
            renderer.reset(); // recreate renderer if size changed
        }
    }

    if (!window) {
        window.reset(SDL_CreateWindow(title, posX, posY, scaledWidth, scaledHeight,
                                      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
        if (!window) {
            LOG_ERROR("Failed to create " + std::string(title) + " window: " + std::string(SDL_GetError()));
            exit(1);
        }

        if (forceSoftwareRenderer_) {
            renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC));
        } else {
            renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        }
        if (!renderer) {
            LOG_ERROR("Failed to create renderer for " + std::string(title) + ": " + std::string(SDL_GetError()));
            exit(1);
        }
    }
#endif

    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

    // Optional debug info
    SDL_RendererInfo rinfo;
    if (SDL_GetRendererInfo(renderer.get(), &rinfo) == 0) {
        LOG_DEBUG(std::string("Renderer for ") + std::string(title) + ", name=" +
                  (rinfo.name ? rinfo.name : "<unknown>") + ", flags=" + std::to_string(rinfo.flags));
    }
}

void WindowManager::getWindowSetup(int& playfieldX, int& playfieldY, int& playfieldWidth, int& playfieldHeight,
                                  int& backglassX, int& backglassY, int& backglassWidth, int& backglassHeight,
                                  int& dmdX, int& dmdY, int& dmdWidth, int& dmdHeight,
                                  int& topperX, int& topperY, int& topperWidth, int& topperHeight) {
    if (playfieldWindow_) {
        SDL_GetWindowPosition(playfieldWindow_.get(), &playfieldX, &playfieldY);
        SDL_GetWindowSize(playfieldWindow_.get(), &playfieldWidth, &playfieldHeight);
    }
    if (backglassWindow_) {
        SDL_GetWindowPosition(backglassWindow_.get(), &backglassX, &backglassY);
        SDL_GetWindowSize(backglassWindow_.get(), &backglassWidth, &backglassHeight);
    }
    if (dmdWindow_) {
        SDL_GetWindowPosition(dmdWindow_.get(), &dmdX, &dmdY);
        SDL_GetWindowSize(dmdWindow_.get(), &dmdWidth, &dmdHeight);
    }
    if (topperWindow_) {
        SDL_GetWindowPosition(topperWindow_.get(), &topperX, &topperY);
        SDL_GetWindowSize(topperWindow_.get(), &topperWidth, &topperHeight);
    }
}
