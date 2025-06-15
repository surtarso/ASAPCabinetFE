/**
 * @file app.h
 * @brief Defines the App class for managing the ASAPCabinetFE application.
 *
 * This header provides the App class, which serves as the main application controller.
 * It implements the IAppCallbacks interface to handle configuration and resource reloading,
 * and integrates components like window management, input handling, rendering, and UI
 * overlays using DependencyFactory.
 */

#ifndef APP_H
#define APP_H

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <SDL_events.h>
#include "config/iconfig_service.h"
#include "config/ui/config_ui.h"
#include "keybinds/iinput_manager.h"
#include "render/irenderer.h"
#include "render/iasset_manager.h"
#include "tables/itable_loader.h"
#include "tables/table_data.h"
#include "tables/table_override_editor.h"
#include "tables/table_override_manager.h"
#include "tables/vpsdb/vpsdb_catalog.h"
#include "sound/isound_manager.h"
#include "capture/iscreenshot_manager.h"
#include "core/iwindow_manager.h"
#include "core/joystick_manager.h"
#include "core/playfield_overlay.h"
#include "core/gui_manager.h"
#include "core/dependency_factory.h"
#include "core/iapp_callbacks.h"
#include "core/loading_progress.h"
#include "core/loading_screen.h"
struct Mix_Chunk;

/**
 * @class App
 * @brief Main application controller for ASAPCabinetFE.
 *
 * This class implements the IAppCallbacks interface to manage the application lifecycle,
 * including initialization, event handling, updating, rendering, and cleanup. It uses
 * DependencyFactory to create and integrate components like IWindowManager, GuiManager,
 * IInputManager, IRenderer, and PlayfieldOverlay, and handles table data, configuration,
 * and UI interactions.
 */
class App : public IAppCallbacks {
public:
    ISoundManager* getSoundManager() override; // Changed to ISoundManager*
    /**
     * @brief Constructs an App instance.
     *
     * Initializes the application with the specified configuration file path.
     *
     * @param configPath The file path to the configuration file.
     */
    App(const std::string& configPath);

    /**
     * @brief Destroys the App instance.
     *
     * Cleans up all managed components and resources.
     */
    ~App();

    /**
     * @brief Runs the main application loop.
     *
     * Executes the application’s main loop, handling events, updating state, and rendering.
     */
    void run();

    /**
     * @brief Reloads the font resource.
     *
     * Reloads the TTF font used for text rendering, optionally for standalone mode.
     *
     * @param isStandalone True for standalone configuration mode, false for main app mode.
     */
    void reloadFont(bool isStandalone = false) override;

    /**
     * @brief Reloads window configurations.
     *
     * Updates SDL windows and renderers using current settings.
     */
    void reloadWindows() override;

    /**
     * @brief Reloads assets and renderers.
     *
     * Reinitializes asset manager and renderer with current settings and table data.
     */
    void reloadAssetsAndRenderers() override;

    /**
     * @brief Reloads table data and window title.
     *
     * Reloads table data from the table loader and updates the window title.
     */
    void reloadTablesAndTitle() override;

    /**
     * @brief Reloads overlay settings.
     *
     * Updates the playfield overlay with current application settings.
     */
    void reloadOverlaySettings() override;

private:
    std::string exeDir_;                        ///< Executable directory for resolving paths.
    std::string configPath_;                    ///< Path to the configuration file.
    bool showConfig_ = false;                   ///< Flag controlling configuration UI visibility.
    std::unique_ptr<TableOverrideEditor> overrideEditor_; ///< Editor for table overrides
    TableOverrideManager overrideManager_; ///< Manager for table overrides
    bool showEditor_ = false;
    std::unique_ptr<vpsdb::VpsdbCatalog> vpsdbCatalog_;
    bool showVpsdb_ = false;
    size_t currentIndex_ = 0;                   ///< Index of the current table.
    size_t lastTableIndex_; ///< Track last table index for editor updates
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_; ///< TTF font for text rendering.
    std::unique_ptr<JoystickManager> joystickManager_;   ///< Manager for SDL joysticks.
    std::unique_ptr<IWindowManager> windowManager_;      ///< Manager for SDL windows and renderers.
    std::unique_ptr<GuiManager> guiManager_;             ///< Manager for ImGui UI rendering.
    std::unique_ptr<ISoundManager> soundManager_;        ///< Manager for sound playback.
    std::unique_ptr<IConfigService> configManager_;      ///< Configuration service for settings.
    std::unique_ptr<ConfigUI> configEditor_;             ///< Configuration UI editor.
    std::unique_ptr<IRenderer> renderer_;                ///< Renderer for playfield, backglass, and DMD.
    std::unique_ptr<IAssetManager> assets_;              ///< Manager for textures and video players.
    std::unique_ptr<IScreenshotManager> screenshotManager_; ///< Manager for screenshot capture.
    std::unique_ptr<IInputManager> inputManager_;        ///< Manager for input events.
    std::unique_ptr<ITableLoader> tableLoader_;          ///< Loader for table data.
    std::vector<TableData> tables_;                      ///< List of table data.
    std::unique_ptr<PlayfieldOverlay> playfieldOverlay_; ///< Overlay for ImGui UI elements.
    bool prevShowConfig_ = false;                        ///< Previous state of showConfig_ flag.
    std::atomic<bool> isLoadingTables_{false};          ///< Tracks loading status
    std::mutex tablesMutex_;                            ///< Protects tables_ vector
    std::shared_ptr<LoadingProgress> loadingProgress_; ///< Loading progress
    std::unique_ptr<LoadingScreen> loadingScreen_;     ///< Loading screen UI
    std::thread loadingThread_;              // Added to store the loading thread
    std::mutex loadingMutex_;                // Added for synchronizing loading state
    std::condition_variable loadingCV_;      // Added to signal loading completion

    /**
     * @brief Gets the executable directory.
     *
     * Retrieves the directory containing the application executable for path resolution.
     *
     * @return The executable directory path.
     */
    std::string getExecutableDir();

    /**
     * @brief Checks if the configuration is valid.
     *
     * Validates the current configuration settings.
     *
     * @return True if the configuration is valid, false otherwise.
     */
    bool isConfigValid();

    /**
     * @brief Loads table data. (leftover)
     *
     * Calls loadTablesThreaded() 
     */
    void loadTables();

    /**
     * @brief Loads the TTF font.
     *
     * Initializes the font resource for text rendering.
     */
    void loadFont();

    /**
     * @brief Handles SDL events.
     *
     * Processes SDL events (e.g., keyboard, joystick, mouse) and updates application state.
     */
    void handleEvents();

    /**
     * @brief Updates the application state.
     *
     * Updates the application state, including input, configuration, and UI interactions.
     */
    void update();

    /**
     * @brief Renders the application.
     *
     * Renders the playfield, backglass, DMD, and ImGui overlays using the renderer and GUI manager.
     */
    void render();

    /**
     * @brief Cleans up resources.
     *
     * Releases all managed components and SDL resources during destruction.
     */
    void cleanup();

    /**
     * @brief Initializes component dependencies.
     *
     * Creates and configures all components using DependencyFactory.
     */
    void initializeDependencies();

    /**
     * @brief Loads table data with threading.
     *
     * Populates the tables_ vector using the table loader.
     */
    void loadTablesThreaded(size_t oldIndex = 0); // Helper for threaded loading
};

#endif // APP_H