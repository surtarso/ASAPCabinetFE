#ifndef IWINDOW_MANAGER_H
#define IWINDOW_MANAGER_H

#include <SDL2/SDL.h>
#include "config/settings.h"

class IWindowManager {
public:
    virtual ~IWindowManager() = default;
    virtual SDL_Window* getPlayfieldWindow() = 0;
    virtual SDL_Window* getBackglassWindow() = 0;
    virtual SDL_Window* getDMDWindow() = 0;
    virtual SDL_Renderer* getPlayfieldRenderer() = 0;
    virtual SDL_Renderer* getBackglassRenderer() = 0;
    virtual SDL_Renderer* getDMDRenderer() = 0;
    virtual void updateWindows(const Settings& settings) = 0;
    virtual void getWindowPositions(int& playfieldX, int& playfieldY, int& backglassX, int& backglassY, 
                                    int& dmdX, int& dmdY) = 0;
};

#endif // IWINDOW_MANAGER_H