#include "core/dependency_factory.h"
#include "core/app.h"
#include "config/config_service.h"  // Still need concrete type for creation
#include "sound/sound_manager.h"    // Still need concrete type for creation
#include "utils/logging.h"

std::unique_ptr<WindowManager> DependencyFactory::createWindowManager(const Settings& settings) {
    return std::make_unique<WindowManager>(settings);
}

std::unique_ptr<GuiManager> DependencyFactory::createGuiManager(IWindowManager* windowManager) {
    auto gui = std::make_unique<GuiManager>(windowManager);
    gui->initialize();
    return gui;
}

std::unique_ptr<AssetManager> DependencyFactory::createAssetManager(IWindowManager* windowManager, TTF_Font* font) {
    return std::make_unique<AssetManager>(windowManager->getPrimaryRenderer(), 
                                          windowManager->getSecondaryRenderer(), font);
}

std::unique_ptr<Renderer> DependencyFactory::createRenderer(IWindowManager* windowManager) {
    return std::make_unique<Renderer>(windowManager->getPrimaryRenderer(), 
                                      windowManager->getSecondaryRenderer());
}

std::unique_ptr<ISoundManager> DependencyFactory::createSoundManager(const std::string& exeDir, const Settings& settings) {  // Fixed: ISoundManager
    auto sound = std::make_unique<SoundManager>(exeDir, settings);
    sound->loadSounds();
    return sound;  // Implicitly upcast to ISoundManager
}

std::unique_ptr<ConfigService> DependencyFactory::createConfigService(const std::string& configPath) {
    auto config = std::make_unique<ConfigService>(configPath);
    config->loadConfig();
    return config;
}

std::unique_ptr<ScreenshotManager> DependencyFactory::createScreenshotManager(const std::string& exeDir, IConfigService* configService, ISoundManager* soundManager) {
    return std::make_unique<ScreenshotManager>(exeDir, configService, &configService->getKeybindManager(), soundManager);
}

std::unique_ptr<InputManager> DependencyFactory::createInputManager(IConfigService* configService, ScreenshotManager* screenshotManager) {
    auto input = std::make_unique<InputManager>(&configService->getKeybindManager());
    input->registerActions();
    return input;  // setDependencies called later in App
}

std::unique_ptr<ConfigUI> DependencyFactory::createConfigUI(IConfigService* configService, AssetManager* assets, 
                                                            size_t* currentIndex, std::vector<TableLoader>* tables, 
                                                            App* app, bool& showConfig) {
    return std::make_unique<ConfigUI>(configService, &configService->getKeybindManager(), assets, 
                                      currentIndex, tables, app, showConfig, false);
}