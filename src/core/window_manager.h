/**
 * @file window_manager.h
 * @brief Defines the WindowManager class for managing SDL windows and renderers in ASAPCabinetFE.
 *
 * This header provides the WindowManager class, which implements the IWindowManager
 * interface to manage SDL windows and renderers for the playfield, backglass, and DMD
 * displays. It initializes and updates display resources based on application settings.
 */

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "core/iwindow_manager.h"
#include <SDL2/SDL.h>
#include <memory>
#include "config/iconfig_service.h"

/**
 * @class WindowManager
 * @brief Manages SDL windows and renderers for VPX displays.
 *
 * This class implements the IWindowManager interface to create, manage, and update
 * SDL windows and renderers for the playfield, backglass, and DMD displays. It uses
 * application settings to configure window properties and supports DPI scaling.
 */
class WindowManager : public IWindowManager {
public:
    /**
     * @brief Constructs a WindowManager instance.
     *
     * Initializes SDL windows and renderers for the playfield, backglass, and DMD
     * displays based on the provided settings.
     *
     * @param settings The application settings for window configuration.
     */
    explicit WindowManager(const Settings& settings);

    /**
     * @brief Destroys the WindowManager instance.
     *
     * Default destructor, no special cleanup required (managed by smart pointers).
     */
    ~WindowManager() = default;

    /**
     * @brief Gets the playfield window.
     *
     * @return The SDL window for the playfield, or nullptr if not available.
     */
    SDL_Window* getPlayfieldWindow() override { return playfieldWindow_.get(); }

    /**
     * @brief Gets the backglass window.
     *
     * @return The SDL window for the backglass, or nullptr if not available.
     */
    SDL_Window* getBackglassWindow() override { return backglassWindow_.get(); }

    /**
     * @brief Gets the DMD window.
     *
     * @return The SDL window for the DMD, or nullptr if not available.
     */
    SDL_Window* getDMDWindow() override { return dmdWindow_.get(); }

    /**
     * @brief Gets the playfield renderer.
     *
     * @return The SDL renderer for the playfield, or nullptr if not available.
     */
    SDL_Renderer* getPlayfieldRenderer() override { return playfieldRenderer_.get(); }

    /**
     * @brief Gets the backglass renderer.
     *
     * @return The SDL renderer for the backglass, or nullptr if not available.
     */
    SDL_Renderer* getBackglassRenderer() override { return backglassRenderer_.get(); }

    /**
     * @brief Gets the DMD renderer.
     *
     * @return The SDL renderer for the DMD, or nullptr if not available.
     */
    SDL_Renderer* getDMDRenderer() override { return dmdRenderer_.get(); }

    /**
     * @brief Updates windows with new settings.
     *
     * Reconfigures the playfield, backglass, and DMD windows and renderers based on
     * the provided application settings.
     *
     * @param settings The application settings to apply.
     */
    void updateWindows(const Settings& settings) override;

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
     */
    void getWindowPositions(int& playfieldX, int& playfieldY, int& backglassX, int& backglassY, 
                            int& dmdX, int& dmdY) override;

private:
    /**
     * @brief Creates or updates an SDL window and renderer.
     *
     * Configures an SDL window and its associated renderer with the specified properties,
     * supporting DPI scaling and custom positioning.
     *
     * @param window The smart pointer to the SDL window.
     * @param renderer The smart pointer to the SDL renderer.
     * @param title The title of the window.
     * @param width The width of the window in pixels.
     * @param height The height of the window in pixels.
     * @param posX The x-coordinate of the window.
     * @param posY The y-coordinate of the window.
     * @param dpiScale The DPI scaling factor for the window.
     * @param enableDpiScaling True to enable DPI scaling, false otherwise.
     */
    void createOrUpdateWindow(std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>& window,
                              std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>& renderer,
                              const char* title,
                              int width, int height,
                              int posX, int posY, float dpiScale, bool enableDpiScaling);

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> playfieldWindow_;   ///< Window for the playfield display.
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> backglassWindow_;   ///< Window for the backglass display.
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> dmdWindow_;         ///< Window for the DMD display.
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> playfieldRenderer_; ///< Renderer for the playfield window.
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> backglassRenderer_; ///< Renderer for the backglass window.
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> dmdRenderer_;      ///< Renderer for the DMD window.
};

#endif // WINDOW_MANAGER_H