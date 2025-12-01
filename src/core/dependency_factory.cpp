/**
 * @file dependency_factory.cpp
 * @brief Implements the DependencyFactory class for creating component instances in ASAPCabinetFE.
 *
 * This file provides the implementation of the DependencyFactory class, which creates
 * instances of core components, such as WindowManager, ImGuiManager, AssetManager, and
 * others, with their required dependencies. It is used by the App class to initialize
 * the application. The factory methods initialize components with settings from
 * IConfigService and other dependencies, with potential for user customization via
 * configUI (e.g., window configurations, asset loading options, keybinds). The file
 * includes a dummy atomic flag for loading state and commented-out asset loading
 * logic for future expansion.
 */

#include "dependency_factory.h"
#include "app.h"
#include "config/config_service.h"
#include "window_manager.h"
#include "render/assets/asset_manager.h"
#include "render/renderer.h"
#include "sound/pulseaudio_player.h"
#include "capture/screenshot_manager.h"
#include "keybinds/keybind_manager.h"
#include "keybinds/input_manager.h"
#include "data/asapcab/asapcab_index_manager.h"
#include "launcher/table_launcher.h"
#include "log/logging.h"


std::unique_ptr<IKeybindProvider> DependencyFactory::createKeybindProvider() {
    return std::make_unique<KeybindManager>();
}

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
 * Initializes a ImGuiManager with the provided window manager and configuration service,
 * and calls its initialize method to set up the ImGui context.
 *
 * @param windowManager The window manager for accessing SDL windows and renderers.
 * @param configService The configuration service for accessing settings.
 * @return A unique pointer to a ImGuiManager instance.
 */
std::unique_ptr<ImGuiManager> DependencyFactory::createImGuiManager(IWindowManager* windowManager, IConfigService* configService) {
    auto gui = std::make_unique<ImGuiManager>(windowManager, configService);
    gui->initialize(); // Sets up ImGui context with window and renderer data
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
                                                                    IConfigService* configService,
                                                                    [[maybe_unused]] size_t index,
                                                                    [[maybe_unused]] const std::vector<TableData>& tables,
                                                                    ISoundManager* soundManager) {
    auto assets = std::make_unique<AssetManager>(windowManager->getPlayfieldRenderer(),
                                                 windowManager->getBackglassRenderer(),
                                                 windowManager->getDMDRenderer(),
                                                 windowManager->getTopperRenderer(),
                                                 font, soundManager);
    assets->setSettingsManager(configService);
    // assets->loadTableAssets(index, tables); // Commented out for now, to be implemented with dynamic asset loading
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
std::unique_ptr<IRenderer> DependencyFactory::createRenderer(IWindowManager* windowManager) {
    return std::make_unique<Renderer>(windowManager->getPlayfieldRenderer(),
                                      windowManager->getBackglassRenderer(),
                                      windowManager->getDMDRenderer(),
                                      windowManager->getTopperRenderer());
}

/**
 * @brief Creates a sound manager instance.
 *
 * Initializes a PulseAudioPlayer with the executable directory and settings, then loads
 * sound resources.
 *
 * @param settings The application settings for sound configuration.
 * @return A unique pointer to an ISoundManager instance.
 */
std::unique_ptr<ISoundManager> DependencyFactory::createSoundManager(const Settings& settings) {
    auto sound = std::make_unique<PulseAudioPlayer>(settings);
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
std::unique_ptr<IConfigService> DependencyFactory::createConfigService(
    const std::string& configPath, IKeybindProvider* keybindProvider) {
    return std::make_unique<ConfigService>(configPath, keybindProvider);
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
                                                                                IKeybindProvider* keybindProvider,
                                                                                ISoundManager* soundManager) {
    return std::make_unique<ScreenshotManager>(exeDir, configService, keybindProvider, soundManager);
}

/**
 * @brief Creates an input manager instance.
 *
 * Initializes an InputManager with the keybind manager (from configService)
 * @return A unique pointer to an InputManager instance.
 */
std::unique_ptr<IInputManager> DependencyFactory::createInputManager(IKeybindProvider* keybindProvider) {
    auto input = std::make_unique<InputManager>(keybindProvider);
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
std::unique_ptr<ConfigUI> DependencyFactory::createConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider,
                                                            IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables,
                                                            App* app, bool& showConfig) {
    return std::make_unique<ConfigUI>(configService, keybindProvider, assets, currentIndex, tables, app, showConfig, false);
}

std::unique_ptr<ITableLauncher> DependencyFactory::createTableLauncher(IConfigService* configService) {
    return std::make_unique<TableLauncher>(configService);
}

std::unique_ptr<ITableCallbacks> DependencyFactory::createTableCallbacks(IConfigService* configService) {
    return std::make_unique<AsapIndexManager>(configService->getSettings());
}
