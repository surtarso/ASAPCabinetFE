/**
 * @file dependency_factory.h
 * @brief Defines the DependencyFactory class for creating component instances in ASAPCabinetFE.
 *
 * This header provides the DependencyFactory class, which contains static methods for
 * creating instances of core components, such as window managers, GUI managers, asset
 * managers, and input managers. It serves as a centralized factory for dependency
 * injection in the application, facilitating initialization of ASAPCabinetFE's
 * subsystems. The factory methods are designed to be extensible, allowing future
 * integration with configUI for customizable settings (e.g., window sizes, asset
 * loading options).
 */

#ifndef DEPENDENCY_FACTORY_H
#define DEPENDENCY_FACTORY_H // Header guard to prevent multiple inclusions

#include <memory>
#include <string> // For std::string in file paths and settings
#include "iwindow_manager.h" // Interface for window management
#include "imgui_manager.h" // GUI manager for ImGui rendering
#include "render/iasset_manager.h" // Interface for asset management
#include "tables/table_data.h" // Table data structure for asset loading
#include "render/irenderer.h" // Renderer for playfield, backglass, and DMD
#include "keybinds/iinput_manager.h" // Input manager for keybind handling
#include "config/iconfig_service.h" // Interface for configuration service
#include "config/ui/config_ui.h" // Configuration UI for user settings
#include "capture/iscreenshot_manager.h" // Interface for screenshot management
#include "sound/isound_manager.h" // Interface for sound management
#include "keybinds/ikeybind_provider.h"
#include "launcher/itable_launcher.h" // Interface for launching tables

/**
 * @class App
 * @brief Main application class (forward declaration).
 *
 * Represents the main application instance of ASAPCabinetFE, used as a dependency
 * in components like ConfigUI. This forward declaration allows the header to
 * reference App without including its full definition.
 */
class App;

/**
 * @class DependencyFactory
 * @brief Static factory for creating component instances.
 *
 * This class provides static methods to create instances of key application components,
 * such as IWindowManager, GuiManager, IAssetManager, and others, configured with the
 * necessary dependencies. It is used by the App class to initialize the application.
 * The factory supports customization through settings (e.g., via IConfigService) and
 * can be extended with configUI to allow user-defined parameters (e.g., window sizes,
 * asset loading behavior).
 */
class DependencyFactory {
public:
    static std::unique_ptr<IKeybindProvider> createKeybindProvider();
    /**
     * @brief Creates a window manager instance.
     *
     * Initializes a WindowManager with the provided settings for configuring SDL windows
     * and renderers (e.g., playfield, backglass, DMD). The settings can include resolution,
     * fullscreen mode, and other display options, which may be made configurable via
     * configUI in the future.
     *
     * @param settings The application settings for window configuration.
     * @return A unique pointer to an IWindowManager instance.
     */
    static std::unique_ptr<IWindowManager> createWindowManager(const Settings& settings);

    /**
     * @brief Creates a GUI manager instance.
     *
     * Initializes a GuiManager with the provided window manager and configuration service,
     * and calls its initialize method to set up the ImGui context. The configuration
     * service provides settings (e.g., UI theme) that could be user-customizable via
     * configUI.
     *
     * @param windowManager The window manager for accessing SDL windows and renderers.
     * @param configService The configuration service for accessing settings.
     * @return A unique pointer to a GuiManager instance.
     */
    static std::unique_ptr<GuiManager> createGuiManager(IWindowManager* windowManager, IConfigService* configService);

    /**
     * @brief Creates an asset manager instance.
     *
     * Initializes an AssetManager with renderers from the window manager, a TTF font,
     * a configuration service, and table data, then loads assets for the specified
     * table index. The method currently skips asset loading (commented out) but can
     * be extended to support configurable asset paths or preload options via configUI.
     *
     * @param windowManager The window manager for accessing renderers.
     * @param font The TTF font for text rendering.
     * @param configService The configuration service for settings.
     * @param index The index of the table to load assets for.
     * @param tables The list of table data.
     * @param soundManager The sound manager for handling audio assets.
     * @return A unique pointer to an IAssetManager instance.
     */
    static std::unique_ptr<IAssetManager> createAssetManager(IWindowManager* windowManager, TTF_Font* font, 
                                                            IConfigService* configService, size_t index, 
                                                            const std::vector<TableData>& tables,
                                                            ISoundManager* soundManager);

    /**
     * @brief Creates a renderer instance.
     *
     * Initializes a Renderer with renderers from the window manager for playfield,
     * backglass, and DMD displays. Future enhancements could allow renderer settings
     * (e.g., resolution, effects) to be configurable via configUI.
     *
     * @param windowManager The window manager for accessing renderers.
     * @return A unique pointer to a Renderer instance.
     */
    static std::unique_ptr<IRenderer> createRenderer(IWindowManager* windowManager);

    /**
     * @brief Creates a sound manager instance.
     *
     * Initializes a PulseAudioPlayer with the executable directory and settings,
     * then loads sound resources. Settings may include volume or sound file paths,
     * which could be user-configurable via configUI.
     *
     * @param settings The application settings for sound configuration.
     * @return A unique pointer to an ISoundManager instance.
     */
    static std::unique_ptr<ISoundManager> createSoundManager(const Settings& settings);

    /**
     * @brief Creates a configuration service instance.
     *
     * Initializes a ConfigService with the specified configuration file path.
     * The service loads and manages settings that could be extended for user
     * customization via configUI.
     *
     * @param configPath The file path to the configuration file.
     * @return A unique pointer to a ConfigService instance.
     */
    static std::unique_ptr<IConfigService> createConfigService(const std::string& configPath, IKeybindProvider* keybindProvider);
    /**
     * @brief Creates a screenshot manager instance.
     *
     * Initializes a ScreenshotManager with the executable directory, configuration
     * service, keybind manager (from configService), and sound manager. The manager
     * handles screenshot capture and could support configurable save paths or formats
     * via configUI.
     *
     * @param exeDir The executable directory for resolving screenshot paths.
     * @param configService The configuration service for settings.
     * @param soundManager The sound manager for screenshot-related sounds.
     * @return A unique pointer to an IScreenshotManager instance.
     */
    static std::unique_ptr<IScreenshotManager> createScreenshotManager(const std::string& exeDir, IConfigService* configService, 
                                                                      IKeybindProvider* keybindProvider, ISoundManager* soundManager);

    /**
     * @brief Creates an input manager instance.
     *
     * Initializes an InputManager with the keybind manager (from configService) and
     * screenshot manager, setting minimal dependencies with dummy values (e.g., index,
     * tables) for basic functionality. The method registers actions and could be
     * extended to support configurable keybinds via configUI.
     *
     * @param configService The configuration service for accessing keybind settings.
     * @param screenshotManager The screenshot manager for screenshot mode.
     * @return A unique pointer to an InputManager instance.
     */
    static std::unique_ptr<IInputManager> createInputManager(IKeybindProvider* keybindProvider,
                                                            IScreenshotManager* screenshotManager,
                                                            ITableLauncher* tableLauncher,
                                                            ITableCallbacks* tableCallbacks);

    /**
     * @brief Creates a configuration UI instance.
     *
     * Initializes a ConfigUI with the configuration service, keybind manager (from
     * configService), asset manager, table data, main application instance, and
     * configuration UI visibility flag. This UI allows user interaction with settings
     * and could be enhanced with additional configurable options via configUI.
     *
     * @param configService The configuration service for settings.
     * @param assets The asset manager for accessing textures and video players.
     * @param currentIndex Pointer to the current table index.
     * @param tables Pointer to the list of table data.
     * @param app The main application instance.
     * @param showConfig Reference to the configuration UI visibility flag.
     * @return A unique pointer to a ConfigUI instance.
     */
    static std::unique_ptr<ConfigUI> createConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
                                                    IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
                                                    App* app, bool& showConfig);

    static std::unique_ptr<ITableLauncher> createTableLauncher(IConfigService* configService);                                                    

    /**
     * @brief Creates a table callbacks instance.
     *
     * Initializes an AsapIndexManager with the configuration service to handle table data persistence.
     *
     * @param configService The configuration service for settings.
     * @return A unique pointer to an ITableCallbacks instance.
     */
    static std::unique_ptr<ITableCallbacks> createTableCallbacks(IConfigService* configService);
};

#endif // DEPENDENCY_FACTORY_H