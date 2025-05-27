/**
 * @file dependency_factory.cpp
 * @brief Implements the DependencyFactory class for creating component instances in ASAPCabinetFE.
 *
 * This file provides the implementation of the DependencyFactory class, which creates
 * instances of core components, such as WindowManager, GuiManager, AssetManager, and
 * others, with their required dependencies. It is used by the App class to initialize
 * the application.
 */

#include "core/dependency_factory.h"
#include "core/app.h"
#include "config/config_service.h"
#include "core/window_manager.h"
#include "render/asset_manager.h"
#include "sound/pulseaudio_player.h"
#include "capture/screenshot_manager.h"
#include "utils/logging.h"

/**
 * @brief Creates a window manager instance.
 *
 * Initializes a WindowManager with the provided settings for configuring SDL windows
 * and renderers.
 *
 * @param settings The application settings for window configuration.
 * @return A unique pointer to an IWindowManager instance.
 */
std::unique_ptr<IWindowManager> DependencyFactory::createWindowManager(const Settings& settings) {
    return std::make_unique<WindowManager>(settings);
}

/**
 * @brief Creates a GUI manager instance.
 *
 * Initializes a GuiManager with the provided window manager and configuration service,
 * and calls its initialize method to set up the ImGui context.
 *
 * @param windowManager The window manager for accessing SDL windows and renderers.
 * @param configService The configuration service for accessing settings.
 * @return A unique pointer to a GuiManager instance.
 */
std::unique_ptr<GuiManager> DependencyFactory::createGuiManager(IWindowManager* windowManager, IConfigService* configService) {
    auto gui = std::make_unique<GuiManager>(windowManager, configService);
    gui->initialize();
    return gui;
}

/**
 * @brief Creates an asset manager instance.
 *
 * Initializes an AssetManager with renderers from the window manager, a TTF font,
 * a configuration service, and table data, then loads assets for the specified table index.
 *
 * @param windowManager The window manager for accessing renderers.
 * @param font The TTF font for text rendering.
 * @param configService The configuration service for settings.
 * @param index The index of the table to load assets for.
 * @param tables The list of table data.
 * @param soundManager The sound manager for handling audio assets.
 * @return A unique pointer to an IAssetManager instance.
 */
std::unique_ptr<IAssetManager> DependencyFactory::createAssetManager(IWindowManager* windowManager, TTF_Font* font, 
                                                                    IConfigService* configService, size_t index, 
                                                                    const std::vector<TableData>& tables,
                                                                    ISoundManager* soundManager) {
    auto assets = std::make_unique<AssetManager>(windowManager->getPlayfieldRenderer(), 
                                                 windowManager->getBackglassRenderer(),
                                                 windowManager->getDMDRenderer(), font,
                                                 soundManager);
    assets->setSettingsManager(configService);
    assets->loadTableAssets(index, tables);
    return assets;
}

/**
 * @brief Creates a renderer instance.
 *
 * Initializes a Renderer with renderers from the window manager for playfield, backglass,
 * and DMD displays.
 *
 * @param windowManager The window manager for accessing renderers.
 * @return A unique pointer to a Renderer instance.
 */
std::unique_ptr<Renderer> DependencyFactory::createRenderer(IWindowManager* windowManager) {
    return std::make_unique<Renderer>(windowManager->getPlayfieldRenderer(), 
                                      windowManager->getBackglassRenderer(),
                                      windowManager->getDMDRenderer());
}

/**
 * @brief Creates a sound manager instance.
 *
 * Initializes a PulseAudioPlayer with the executable directory and settings, then loads
 * sound resources.
 *
 * @param exeDir The executable directory for resolving sound file paths.
 * @param settings The application settings for sound configuration.
 * @return A unique pointer to an ISoundManager instance.
 */
std::unique_ptr<ISoundManager> DependencyFactory::createSoundManager(const std::string& exeDir, const Settings& settings) {
    auto sound = std::make_unique<PulseAudioPlayer>(exeDir, settings);
    sound->loadSounds();
    return sound;
}

/**
 * @brief Creates a configuration service instance.
 *
 * Initializes a ConfigService with the specified configuration file path.
 *
 * @param configPath The file path to the configuration file.
 * @return A unique pointer to a ConfigService instance.
 */
std::unique_ptr<ConfigService> DependencyFactory::createConfigService(const std::string& configPath) {
    auto config = std::make_unique<ConfigService>(configPath);
    return config;
}

/**
 * @brief Creates a screenshot manager instance.
 *
 * Initializes a ScreenshotManager with the executable directory, configuration service,
 * keybind manager (from configService), and sound manager.
 *
 * @param exeDir The executable directory for resolving screenshot paths.
 * @param configService The configuration service for settings.
 * @param soundManager The sound manager for screenshot-related sounds.
 * @return A unique pointer to an IScreenshotManager instance.
 */
std::unique_ptr<IScreenshotManager> DependencyFactory::createScreenshotManager(const std::string& exeDir, 
                                                                              IConfigService* configService, 
                                                                              ISoundManager* soundManager) {
    return std::make_unique<ScreenshotManager>(exeDir, configService, &configService->getKeybindManager(), soundManager);
}

/**
 * @brief Creates an input manager instance.
 *
 * Initializes an InputManager with the keybind manager (from configService) and
 * screenshot manager, setting minimal dependencies and registering actions.
 *
 * @param configService The configuration service for accessing keybind settings.
 * @param screenshotManager The screenshot manager for screenshot mode.
 * @return A unique pointer to an InputManager instance.
 */
std::unique_ptr<InputManager> DependencyFactory::createInputManager(IConfigService* configService, 
                                                                   IScreenshotManager* screenshotManager) {
    auto input = std::make_unique<InputManager>(&configService->getKeybindManager());
    size_t dummyIndex = 0;
    bool dummyShowConfig = false;
    std::vector<TableData> dummyTables;
    input->setDependencies(nullptr, nullptr, configService, dummyIndex, dummyTables,
                           dummyShowConfig, "", screenshotManager, nullptr);
    input->registerActions();
    return input;
}

/**
 * @brief Creates a configuration UI instance.
 *
 * Initializes a ConfigUI with the configuration service, keybind manager (from
 * configService), asset manager, table data, main application instance, and
 * configuration UI visibility flag.
 *
 * @param configService The configuration service for settings.
 * @param assets The asset manager for accessing textures and video players.
 * @param currentIndex Pointer to the current table index.
 * @param tables Pointer to the list of table data.
 * @param app The main application instance.
 * @param showConfig Reference to the configuration UI visibility flag.
 * @return A unique pointer to a ConfigUI instance.
 */
std::unique_ptr<ConfigUI> DependencyFactory::createConfigUI(IConfigService* configService, IAssetManager* assets, 
                                                            size_t* currentIndex, std::vector<TableData>* tables, 
                                                            App* app, bool& showConfig) {
    return std::make_unique<ConfigUI>(configService, &configService->getKeybindManager(), assets, 
                                      currentIndex, tables, app, showConfig, false);
}