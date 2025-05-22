#ifndef IWINDOW_MANAGER_H
#define IWINDOW_MANAGER_H

#include <SDL2/SDL.h>
#include "config/settings.h"

/**
 * @brief Interface for managing various app windows and their renderers.
 *
 * The IWindowManager interface provides an abstraction to access and manage
 * different SDL_Window and SDL_Renderer instances associated with the app,
 * such as the playfield, backglass, and DMD. This interface allows for retrieving
 * individual windows and renderers, updating window configurations based on specific
 * settings, and obtaining the current positions of these windows.
 *
 * @note Implementers of this interface must define all the pure virtual methods.
 *
 * Methods:
 *  - SDL_Window* getPlayfieldWindow(): Retrieves the window used for the playfield.
 *  - SDL_Window* getBackglassWindow(): Retrieves the window used for the backglass.
 *  - SDL_Window* getDMDWindow(): Retrieves the window used for the DMD.
 *  - SDL_Renderer* getPlayfieldRenderer(): Retrieves the renderer for the playfield window.
 *  - SDL_Renderer* getBackglassRenderer(): Retrieves the renderer for the backglass window.
 *  - SDL_Renderer* getDMDRenderer(): Retrieves the renderer for the DMD window.
 *  - void updateWindows(const Settings& settings): Updates the windows with new settings.
 *  - void getWindowPositions(int& playfieldX, int& playfieldY, int& backglassX, int& backglassY,
 *                              int& dmdX, int& dmdY): Retrieves the positions for the playfield,
 *                              backglass, and DMD windows.
 */
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