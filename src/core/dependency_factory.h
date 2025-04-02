#ifndef DEPENDENCY_FACTORY_H
#define DEPENDENCY_FACTORY_H

#include <memory>
#include <string>
#include "core/window_manager.h"
#include "core/gui_manager.h"
#include "render/asset_manager.h"
#include "render/renderer.h"
#include "keybinds/input_manager.h"
#include "config/iconfig_service.h"
#include "config/ui/config_ui.h"
#include "capture/screenshot_manager.h"
#include "sound/isound_manager.h"

class App;

class DependencyFactory {
public:
    static std::unique_ptr<WindowManager> createWindowManager(const Settings& settings);
    static std::unique_ptr<GuiManager> createGuiManager(IWindowManager* windowManager);
    static std::unique_ptr<AssetManager> createAssetManager(IWindowManager* windowManager, TTF_Font* font);
    static std::unique_ptr<Renderer> createRenderer(IWindowManager* windowManager);
    static std::unique_ptr<ISoundManager> createSoundManager(const std::string& exeDir, const Settings& settings);
    static std::unique_ptr<ConfigService> createConfigService(const std::string& configPath);
    static std::unique_ptr<ScreenshotManager> createScreenshotManager(const std::string& exeDir, 
                                                                    IConfigService* configService, 
                                                                    ISoundManager* soundManager);
    static std::unique_ptr<InputManager> createInputManager(IConfigService* configService, 
                                                           ScreenshotManager* screenshotManager);
    static std::unique_ptr<ConfigUI> createConfigUI(IConfigService* configService, AssetManager* assets, 
                                                    size_t* currentIndex, std::vector<TableLoader>* tables, 
                                                    App* app, bool& showConfig);
};

#endif // DEPENDENCY_FACTORY_H