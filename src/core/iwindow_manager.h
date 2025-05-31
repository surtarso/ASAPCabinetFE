/**
 * @file iwindow_manager.h
 * @brief Defines the IWindowManager interface for managing SDL windows and renderers in ASAPCabinetFE.
 *
 * This header provides the IWindowManager interface, which specifies methods for accessing
 * and managing SDL windows and renderers for the playfield, backglass, and DMD displays.
 * It supports updating window configurations and retrieving window positions based on
 * application settings.
 */

#ifndef IWINDOW_MANAGER_H
#define IWINDOW_MANAGER_H

#include <SDL2/SDL.h>
#include "config/settings.h"

/**
 * @class IWindowManager
 * @brief Interface for managing SDL windows and renderers.
 *
 * This pure virtual class defines methods for accessing SDL windows and renderers
 * for the playfield, backglass, and DMD displays, updating their configurations
 * using application settings, and retrieving their positions. Implementers, such as
 * WindowManager, coordinate with components like Renderer and AssetManager to
 * manage display resources.
 */
class IWindowManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IWindowManager() = default;

    /**
     * @brief Gets the playfield window.
     *
     * Retrieves the SDL window used for the playfield display.
     *
     * @return The SDL window for the playfield, or nullptr if not available.
     */
    virtual SDL_Window* getPlayfieldWindow() = 0;

    /**
     * @brief Gets the backglass window.
     *
     * Retrieves the SDL window used for the backglass display.
     *
     * @return The SDL window for the backglass, or nullptr if not available.
     */
    virtual SDL_Window* getBackglassWindow() = 0;

    /**
     * @brief Gets the DMD window.
     *
     * Retrieves the SDL window used for the DMD (Dot Matrix Display).
     *
     * @return The SDL window for the DMD, or nullptr if not available.
     */
    virtual SDL_Window* getDMDWindow() = 0;

    /**
     * @brief Gets the DMD window.
     *
     * Retrieves the SDL window used for the topper display.
     *
     * @return The SDL window for the topper display, or nullptr if not available.
     */
    virtual SDL_Window* getTopperWindow() = 0;

    /**
     * @brief Gets the playfield renderer.
     *
     * Retrieves the SDL renderer for the playfield window.
     *
     * @return The SDL renderer for the playfield, or nullptr if not available.
     */
    virtual SDL_Renderer* getPlayfieldRenderer() = 0;

    /**
     * @brief Gets the backglass renderer.
     *
     * Retrieves the SDL renderer for the backglass window.
     *
     * @return The SDL renderer for the backglass, or nullptr if not available.
     */
    virtual SDL_Renderer* getBackglassRenderer() = 0;

    /**
     * @brief Gets the DMD renderer.
     *
     * Retrieves the SDL renderer for the DMD window.
     *
     * @return The SDL renderer for the DMD, or nullptr if not available.
     */
    virtual SDL_Renderer* getDMDRenderer() = 0;

    /**
     * @brief Gets the Topper renderer.
     *
     * Retrieves the SDL renderer for the Topper window.
     *
     * @return The SDL renderer for the Topper, or nullptr if not available.
     */
    virtual SDL_Renderer* getTopperRenderer() = 0;

    /**
     * @brief Updates windows with new settings.
     *
     * Reconfigures the playfield, backglass, and DMD windows based on the provided
     * application settings, such as resolution, position, and DPI scaling.
     *
     * @param settings The application settings to apply.
     */
    virtual void updateWindows(const Settings& settings) = 0;

    /**
     * @brief Gets the positions of all windows.
     *
     * Retrieves the x and y coordinates of the playfield, backglass, and DMD windows.
     *
     * @param playfieldX The x-coordinate of the playfield window.
     * @param playfieldY The y-coordinate of the playfield window.
     * @param backglassX The x-coordinate of the backglass window.
     * @param backglassY The y-coordinate of the backglass window.
     * @param dmdX The x-coordinate of the DMD window.
     * @param dmdY The y-coordinate of the DMD window.
     * @param topperX The x-coordinate of the Topper window.
     * @param topperY The y-coordinate of the Topper window.
     */
    virtual void getWindowPositions(int& playfieldX, int& playfieldY, int& backglassX, int& backglassY, 
                                    int& dmdX, int& dmdY, int& topperX, int& topperY) = 0;
};

#endif // IWINDOW_MANAGER_H