#ifndef IWINDOW_MANAGER_H
#define IWINDOW_MANAGER_H

#include <SDL2/SDL.h>

class IWindowManager {
public:
    virtual ~IWindowManager() = default;
    virtual SDL_Window* getPrimaryWindow() = 0;
    virtual SDL_Window* getSecondaryWindow() = 0;
    virtual SDL_Renderer* getPrimaryRenderer() = 0;
    virtual SDL_Renderer* getSecondaryRenderer() = 0;
};

#endif // IWINDOW_MANAGER_H