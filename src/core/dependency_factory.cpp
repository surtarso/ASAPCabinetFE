#include "core/dependency_factory.h"
#include "core/app.h"
#include "config/config_service.h"
#include "core/window_manager.h"
#include "sound/sound_manager.h"
#include "utils/logging.h"

std::unique_ptr<IWindowManager> DependencyFactory::createWindowManager(const Settings& settings) {
    return std::make_unique<WindowManager>(settings);
}

std::unique_ptr<GuiManager> DependencyFactory::createGuiManager(IWindowManager* windowManager, IConfigService* configService) {
    auto gui = std::make_unique<GuiManager>(windowManager, configService);
    gui->initialize();
    return gui;
}

std::unique_ptr<AssetManager> DependencyFactory::createAssetManager(IWindowManager* windowManager, TTF_Font* font) {
    return std::make_unique<AssetManager>(windowManager->getPlayfieldRenderer(), 
                                          windowManager->getBackglassRenderer(),
                                          windowManager->getDMDRenderer(), font);
}

std::unique_ptr<Renderer> DependencyFactory::createRenderer(IWindowManager* windowManager) {
    return std::make_unique<Renderer>(windowManager->getPlayfieldRenderer(), 
                                      windowManager->getBackglassRenderer(),
                                      windowManager->getDMDRenderer());
}

std::unique_ptr<ISoundManager> DependencyFactory::createSoundManager(const std::string& exeDir, const Settings& settings) {
    auto sound = std::make_unique<SoundManager>(exeDir, settings);
    sound->loadSounds();
    return sound;
}

std::unique_ptr<ConfigService> DependencyFactory::createConfigService(const std::string& configPath) {
    auto config = std::make_unique<ConfigService>(configPath);
    //config->loadConfig();
    return config;
}

std::unique_ptr<ScreenshotManager> DependencyFactory::createScreenshotManager(const std::string& exeDir, 
                                                                             IConfigService* configService, 
                                                                             ISoundManager* soundManager) {
    return std::make_unique<ScreenshotManager>(exeDir, configService, &configService->getKeybindManager(), soundManager);
}

std::unique_ptr<InputManager> DependencyFactory::createInputManager(IConfigService* configService, 
                                                                   ScreenshotManager* screenshotManager) {
    auto input = std::make_unique<InputManager>(&configService->getKeybindManager());
    size_t dummyIndex = 0;
    bool dummyShowConfig = false;
    std::vector<TableLoader> dummyTables; // Added declaration
    input->setDependencies(nullptr, nullptr, configService, dummyIndex, dummyTables,
                           dummyShowConfig, "", screenshotManager, nullptr);
    input->registerActions();
    return input;
}

std::unique_ptr<ConfigUI> DependencyFactory::createConfigUI(IConfigService* configService, AssetManager* assets, 
                                                            size_t* currentIndex, std::vector<TableLoader>* tables, 
                                                            App* app, bool& showConfig) {
    return std::make_unique<ConfigUI>(configService, &configService->getKeybindManager(), assets, 
                                      currentIndex, tables, app, showConfig, false);
}