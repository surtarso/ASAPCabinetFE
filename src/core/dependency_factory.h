/**
 * @file dependency_factory.h
 * @brief Defines the DependencyFactory class for creating component instances in ASAPCabinetFE.
 *
 * This header provides the DependencyFactory class, which contains static methods for
 * creating instances of core components, such as window managers, GUI managers, asset
 * managers, and input managers. It serves as a centralized factory for dependency
 * injection in the application.
 */

#ifndef DEPENDENCY_FACTORY_H
#define DEPENDENCY_FACTORY_H

#include <memory>
#include <string>
#include "core/iwindow_manager.h"
#include "core/gui_manager.h"
#include "render/iasset_manager.h"
#include "render/table_data.h"
#include "render/renderer.h"
#include "keybinds/input_manager.h"
#include "config/iconfig_service.h"
#include "config/ui/config_gui.h"
#include "capture/iscreenshot_manager.h"
#include "sound/isound_manager.h"

/**
 * @class App
 * @brief Main application class (forward declaration).
 */
class App;

/**
 * @class DependencyFactory
 * @brief Static factory for creating component instances.
 *
 * This class provides static methods to create instances of key application components,
 * such as IWindowManager, GuiManager, IAssetManager, and others, configured with the
 * necessary dependencies. It is used by the App class to initialize the application.
 */
class DependencyFactory {
public:
    /**
     * @brief Creates a window manager instance.
     *
     * @param settings The application settings for window configuration.
     * @return A unique pointer to an IWindowManager instance.
     */
    static std::unique_ptr<IWindowManager> createWindowManager(const Settings& settings);

    /**
     * @brief Creates a GUI manager instance.
     *
     * @param windowManager The window manager for accessing SDL windows and renderers.
     * @param configService The configuration service for accessing settings.
     * @return A unique pointer to a GuiManager instance.
     */
    static std::unique_ptr<GuiManager> createGuiManager(IWindowManager* windowManager, IConfigService* configService);

    /**
     * @brief Creates an asset manager instance.
     *
     * @param windowManager The window manager for accessing renderers.
     * @param font The TTF font for text rendering.
     * @param configService The configuration service for settings.
     * @param index The index of the table to load assets for.
     * @param tables The list of table data.
     * @return A unique pointer to an IAssetManager instance.
     */
    static std::unique_ptr<IAssetManager> createAssetManager(IWindowManager* windowManager, TTF_Font* font, 
                                                            IConfigService* configService, size_t index, 
                                                            const std::vector<TableData>& tables);

    /**
     * @brief Creates a renderer instance.
     *
     * @param windowManager The window manager for accessing renderers.
     * @return A unique pointer to a Renderer instance.
     */
    static std::unique_ptr<Renderer> createRenderer(IWindowManager* windowManager);

    /**
     * @brief Creates a sound manager instance.
     *
     * @param exeDir The executable directory for resolving sound file paths.
     * @param settings The application settings for sound configuration.
     * @return A unique pointer to an ISoundManager instance.
     */
    static std::unique_ptr<ISoundManager> createSoundManager(const std::string& exeDir, const Settings& settings);

    /**
     * @brief Creates a configuration service instance.
     *
     * @param configPath The file path to the configuration file.
     * @return A unique pointer to a ConfigService instance.
     */
    static std::unique_ptr<ConfigService> createConfigService(const std::string& configPath);

    /**
     * @brief Creates a screenshot manager instance.
     *
     * @param exeDir The executable directory for resolving screenshot paths.
     * @param configService The configuration service for settings.
     * @param soundManager The sound manager for screenshot-related sounds.
     * @return A unique pointer to an IScreenshotManager instance.
     */
    static std::unique_ptr<IScreenshotManager> createScreenshotManager(const std::string& exeDir, 
                                                                      IConfigService* configService, 
                                                                      ISoundManager* soundManager);

    /**
     * @brief Creates an input manager instance.
     *
     * @param configService The configuration service for accessing keybind settings.
     * @param screenshotManager The screenshot manager for screenshot mode.
     * @return A unique pointer to an InputManager instance.
     */
    static std::unique_ptr<InputManager> createInputManager(IConfigService* configService, 
                                                           IScreenshotManager* screenshotManager);

    /**
     * @brief Creates a configuration UI instance.
     *
     * @param configService The configuration service for settings.
     * @param assets The asset manager for accessing textures and video players.
     * @param currentIndex Pointer to the current table index.
     * @param tables Pointer to the list of table data.
     * @param app The main application instance.
     * @param showConfig Reference to the configuration UI visibility flag.
     * @return A unique pointer to a ConfigUI instance.
     */
    static std::unique_ptr<ConfigUI> createConfigUI(IConfigService* configService, IAssetManager* assets, 
                                                    size_t* currentIndex, std::vector<TableData>* tables, 
                                                    App* app, bool& showConfig);
};

#endif // DEPENDENCY_FACTORY_H