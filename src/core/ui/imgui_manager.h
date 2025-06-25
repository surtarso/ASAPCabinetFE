/**
 * @file gui_manager.h
 * @brief Defines the ImGuiManager class for managing ImGui contexts in ASAPCabinetFE.
 *
 * This header provides the ImGuiManager class, which initializes and manages ImGui
 * contexts for rendering UI elements in both the main application (using IWindowManager)
 * and standalone initial configuration modes (using raw SDL window/renderer). It
 * processes SDL events and renders ImGui draw data.
 */

#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include <SDL2/SDL.h>
#include <memory>

/**
 * @class ImGuiContext
 * @brief ImGui context structure (forward declaration).
 */
struct ImGuiContext;

/**
 * @class IWindowManager
 * @brief Interface for window management (forward declaration).
 */
class IWindowManager;

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class ImGuiManager
 * @brief Manages ImGui contexts for UI rendering.
 *
 * This class initializes and manages ImGui contexts for rendering UI elements in
 * two modes: the main application (using IWindowManager for playfield, backglass,
 * and DMD windows) and standalone initial configuration (using a single SDL window
 * and renderer). It handles SDL event processing and ImGui rendering, integrating
 * with IConfigService for settings.
 */
class ImGuiManager {
public:
    /**
     * @brief Constructs a ImGuiManager for the main application.
     *
     * Initializes the ImGui context for the main application using the provided
     * window manager and configuration service.
     *
     * @param windowManager The window manager for accessing SDL windows and renderers.
     * @param configService The configuration service for accessing settings.
     */
    ImGuiManager(IWindowManager* windowManager, IConfigService* configService);

    /**
     * @brief Constructs a ImGuiManager for standalone initial configuration.
     *
     * Initializes the ImGui context for initial configuration using a single SDL
     * window and renderer, with the provided configuration service.
     *
     * @param window The SDL window for the configuration UI.
     * @param renderer The SDL renderer for the configuration UI.
     * @param configService The configuration service for accessing settings.
     */
    ImGuiManager(SDL_Window* window, SDL_Renderer* renderer, IConfigService* configService);

    /**
     * @brief Destroys the ImGuiManager instance.
     *
     * Cleans up the ImGui context and any associated resources.
     */
    ~ImGuiManager();

    /**
     * @brief Initializes the ImGui context.
     *
     * Sets up the ImGui context for rendering, configuring it for the main application
     * or initial configuration mode.
     */
    void initialize();

    /**
     * @brief Starts a new ImGui frame.
     *
     * Prepares ImGui for a new rendering frame, updating internal state for UI elements.
     */
    void newFrame();

    /**
     * @brief Renders ImGui draw data.
     *
     * Renders the ImGui UI elements to the specified SDL renderer.
     *
     * @param renderer The SDL renderer to use for rendering ImGui draw data.
     */
    void render(SDL_Renderer* renderer);

    /**
     * @brief Processes an SDL event for ImGui.
     *
     * Forwards the provided SDL event to ImGui for handling UI interactions (e.g.,
     * mouse clicks, keyboard input).
     *
     * @param event The SDL event to process.
     */
    void processEvent(const SDL_Event& event);

private:
    IWindowManager* windowManager_;  ///< Window manager for main app mode (nullptr in config mode).
    IConfigService* configService_;  ///< Configuration service for settings.
    SDL_Window* configWindow_;       ///< SDL window for config mode (nullptr in main app mode).
    SDL_Renderer* configRenderer_;   ///< SDL renderer for config mode (nullptr in main app mode).
    ImGuiContext* context_;          ///< ImGui context for UI rendering.
};

#endif // GUI_MANAGER_H