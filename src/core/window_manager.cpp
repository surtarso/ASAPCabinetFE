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
    // Check if Playfield settings changed
    bool updatePlayfield = false;
    if (playfieldWindow_) {
        int currentWidth, currentHeight, currentX, currentY;
        SDL_GetWindowSize(playfieldWindow_.get(), &currentWidth, &currentHeight);
        SDL_GetWindowPosition(playfieldWindow_.get(), &currentX, &currentY);
        int currentDisplay = SDL_GetWindowDisplayIndex(playfieldWindow_.get());
        int scaledWidth = settings.enableDpiScaling ? static_cast<int>(settings.playfieldWindowWidth * settings.dpiScale) : settings.playfieldWindowWidth;
        int scaledHeight = settings.enableDpiScaling ? static_cast<int>(settings.playfieldWindowHeight * settings.dpiScale) : settings.playfieldWindowHeight;
        int targetX = settings.playfieldX == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(settings.playfieldWindowMonitor) : settings.playfieldX;
        int targetY = settings.playfieldY == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(settings.playfieldWindowMonitor) : settings.playfieldY;
        if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
            currentDisplay != settings.playfieldWindowMonitor || currentX != targetX || currentY != targetY) {
            updatePlayfield = true;
        }
    } else {
        updatePlayfield = true;
    }

    // Check if Backglass settings changed
    bool updateBackglass = false;
    if (settings.showBackglass != !!backglassWindow_) {
        updateBackglass = true;
    } else if (backglassWindow_) {
        int currentWidth, currentHeight, currentX, currentY;
        SDL_GetWindowSize(backglassWindow_.get(), &currentWidth, &currentHeight);
        SDL_GetWindowPosition(backglassWindow_.get(), &currentX, &currentY);
        int currentDisplay = SDL_GetWindowDisplayIndex(backglassWindow_.get());
        int scaledWidth = settings.enableDpiScaling ? static_cast<int>(settings.backglassWindowWidth * settings.dpiScale) : settings.backglassWindowWidth;
        int scaledHeight = settings.enableDpiScaling ? static_cast<int>(settings.backglassWindowHeight * settings.dpiScale) : settings.backglassWindowHeight;
        int targetX = settings.backglassX == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(settings.backglassWindowMonitor) : settings.backglassX;
        int targetY = settings.backglassY == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(settings.backglassWindowMonitor) : settings.backglassY;
        if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
            currentDisplay != settings.backglassWindowMonitor || currentX != targetX || currentY != targetY) {
            updateBackglass = true;
        }
    }

    // Check if DMD settings changed
    bool updateDMD = false;
    if (settings.showDMD != !!dmdWindow_) {
        updateDMD = true;
    } else if (dmdWindow_) {
        int currentWidth, currentHeight, currentX, currentY;
        SDL_GetWindowSize(dmdWindow_.get(), &currentWidth, &currentHeight);
        SDL_GetWindowPosition(dmdWindow_.get(), &currentX, &currentY);
        int currentDisplay = SDL_GetWindowDisplayIndex(dmdWindow_.get());
        int scaledWidth = settings.enableDpiScaling ? static_cast<int>(settings.dmdWindowWidth * settings.dpiScale) : settings.dmdWindowWidth;
        int scaledHeight = settings.enableDpiScaling ? static_cast<int>(settings.dmdWindowHeight * settings.dpiScale) : settings.dmdWindowHeight;
        int targetX = settings.dmdX == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(settings.dmdWindowMonitor) : settings.dmdX;
        int targetY = settings.dmdY == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(settings.dmdWindowMonitor) : settings.dmdY;
        if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
            currentDisplay != settings.dmdWindowMonitor || currentX != targetX || currentY != targetY) {
            updateDMD = true;
        }
    }

    // Update only the necessary windows
    if (updatePlayfield) {
        LOG_DEBUG("WindowsManager: Updating Playfield window");
        createOrUpdateWindow(playfieldWindow_, playfieldRenderer_, "Playfield",
                             settings.playfieldWindowMonitor,
                             settings.playfieldWindowWidth,
                             settings.playfieldWindowHeight,
                             settings.playfieldX,
                             settings.playfieldY,
                             settings.dpiScale,
                             settings.enableDpiScaling);
    }

    if (updateBackglass) {
        LOG_DEBUG("WindowsManager: Updating Backglass window");
        if (settings.showBackglass) {
            createOrUpdateWindow(backglassWindow_, backglassRenderer_, "Backglass",
                                 settings.backglassWindowMonitor,
                                 settings.backglassWindowWidth,
                                 settings.backglassWindowHeight,
                                 settings.backglassX,
                                 settings.backglassY,
                                 settings.dpiScale,
                                 settings.enableDpiScaling);
        } else {
            backglassRenderer_.reset();
            backglassWindow_.reset();
        }
    }

    if (updateDMD) {
        LOG_DEBUG("WindowsManager: Updating DMD window");
        if (settings.showDMD) {
            createOrUpdateWindow(dmdWindow_, dmdRenderer_, "DMD",
                                 settings.dmdWindowMonitor,
                                 settings.dmdWindowWidth,
                                 settings.dmdWindowHeight,
                                 settings.dmdX,
                                 settings.dmdY,
                                 settings.dpiScale,
                                 settings.enableDpiScaling);
        } else {
            dmdRenderer_.reset();
            dmdWindow_.reset();
        }
    }
}
void WindowManager::createOrUpdateWindow(std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>& window,
                                        std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>& renderer,
                                        const char* title, int monitor, int width, int height,
                                        int posX, int posY, float dpiScale, bool enableDpiScaling) {
    int scaledWidth = enableDpiScaling ? static_cast<int>(width * dpiScale) : width;
    int scaledHeight = enableDpiScaling ? static_cast<int>(height * dpiScale) : height;

    if (window) {
        int currentWidth, currentHeight, currentX, currentY;
        SDL_GetWindowSize(window.get(), &currentWidth, &currentHeight);
        SDL_GetWindowPosition(window.get(), &currentX, &currentY);
        int currentDisplay = SDL_GetWindowDisplayIndex(window.get());

        // Use -1 to indicate centered position for comparison
        int targetX = posX == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(monitor) : posX;
        int targetY = posY == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(monitor) : posY;

        if (currentWidth != scaledWidth || currentHeight != scaledHeight ||
            currentDisplay != monitor || currentX != targetX || currentY != targetY) {
            SDL_SetWindowSize(window.get(), scaledWidth, scaledHeight);
            SDL_SetWindowPosition(window.get(), targetX, targetY);

            int actualWidth, actualHeight, actualX, actualY;
            SDL_GetWindowSize(window.get(), &actualWidth, &actualHeight);
            SDL_GetWindowPosition(window.get(), &actualX, &actualY);
            int actualDisplay = SDL_GetWindowDisplayIndex(window.get());
            if (actualWidth != scaledWidth || actualHeight != scaledHeight ||
                actualX != targetX || actualY != targetY || actualDisplay != monitor) {
                renderer.reset();
                window.reset();
            }
        }
    }

    if (!window) {
        int windowX = posX == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(monitor) : posX;
        int windowY = posY == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(monitor) : posY;
        window.reset(SDL_CreateWindow(title, windowX, windowY, scaledWidth, scaledHeight,
                                      SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
        if (!window) {
            LOG_ERROR("WindowsManager: Failed to create " << title << " window: " << SDL_GetError());
            exit(1);
        }

        renderer.reset(SDL_CreateRenderer(window.get(), -1,
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        if (!renderer) {
            LOG_ERROR("WindowsManager: Failed to create " << title << " renderer: " << SDL_GetError());
            exit(1);
        }
        SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
    }
}

void WindowManager::getWindowPositions(int& playfieldX, int& playfieldY, int& backglassX, int& backglassY, int& dmdX, int& dmdY) {
    if (playfieldWindow_) {
        SDL_GetWindowPosition(playfieldWindow_.get(), &playfieldX, &playfieldY);
    }
    if (backglassWindow_) {
        SDL_GetWindowPosition(backglassWindow_.get(), &backglassX, &backglassY);
    }
    if (dmdWindow_) {
        SDL_GetWindowPosition(dmdWindow_.get(), &dmdX, &dmdY);
    }
}