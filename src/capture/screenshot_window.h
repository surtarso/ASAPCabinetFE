/**
 * @file screenshot_window.h
 * @brief Defines the ScreenshotWindow class for managing the screenshot UI window in ASAPCabinetFE.
 *
 * This header provides the ScreenshotWindow class, which creates and manages an SDL
 * window for the screenshot capture UI. It handles rendering, user input, and cleanup,
 * integrating with IConfigService for settings and IKeybindProvider for keybindings.
 */

#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class IKeybindProvider
 * @brief Interface for keybind providers (forward declaration).
 */
class IKeybindProvider;

/**
 * @class ScreenshotWindow
 * @brief Manages the SDL window for screenshot capture UI.
 *
 * This class creates an SDL window and renderer for displaying the screenshot capture
 * UI, including a button for triggering captures. It uses IConfigService to access
 * configuration settings (e.g., font, window size) and IKeybindProvider to handle
 * keybind inputs for actions like capturing or exiting.
 */
class ScreenshotWindow {
public:
    /**
     * @brief Constructs a ScreenshotWindow instance.
     *
     * Initializes the window with configuration and keybind dependencies.
     *
     * @param configManager The configuration service for settings.
     * @param keybindProvider The keybind provider for input handling.
     */
    ScreenshotWindow(IConfigService* configManager, IKeybindProvider* keybindProvider);

    /**
     * @brief Destroys the ScreenshotWindow instance.
     *
     * Cleans up SDL resources (window, renderer, font, texture).
     */
    ~ScreenshotWindow();

    /**
     * @brief Initializes the SDL window and renderer.
     *
     * Sets up the window, renderer, font, and button texture with the specified size.
     *
     * @param width The window width.
     * @param height The window height.
     * @return True if initialization succeeds, false otherwise.
     */
    bool initialize(int width, int height);

    /**
     * @brief Renders the screenshot UI.
     *
     * Draws the UI elements (e.g., capture button) using the SDL renderer.
     */
    void render();

    /**
     * @brief Raises and focuses the window.
     *
     * Brings the window to the foreground and sets input focus.
     */
    void raiseAndFocus();

    /**
     * @brief Cleans up SDL resources.
     *
     * Destroys the window, renderer, font, and texture, releasing resources.
     */
    void cleanup();

    /**
     * @brief Gets the SDL window.
     *
     * @return Pointer to the SDL window.
     */
    SDL_Window* getWindow() const { return window_; }

private:
    IConfigService* configManager_;     ///< Configuration service for settings.
    IKeybindProvider* keybindProvider_; ///< Keybind provider for input handling.
    SDL_Window* window_;                ///< SDL window for the screenshot UI.
    SDL_Renderer* renderer_;            ///< SDL renderer for drawing UI elements.
    TTF_Font* font_;                    ///< TTF font for rendering text.
    SDL_Texture* textTexture_;          ///< Texture for the capture button text.
    SDL_Rect buttonRect_;               ///< Rectangle defining the capture button area.
    std::string buttonText_;            ///< Text displayed on the capture button.
};

#endif // SCREENSHOT_WINDOW_H