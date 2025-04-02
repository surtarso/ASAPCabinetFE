#ifndef APP_H
#define APP_H

#include <memory>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "config/iconfig_service.h"
#include "config/ui/config_ui.h"
#include "keybinds/iinput_manager.h"
#include "render/irenderer.h"
#include "render/asset_manager.h"
#include "render/table_loader.h"
#include "capture/screenshot_manager.h"
#include "core/iwindow_manager.h"
#include "core/system_initializer.h"
#include "sound/isound_manager.h"

class IWindowManager;
class IRenderer;
class IInputManager;

class App {
public:
    App(const std::string& configPath);
    ~App();
    void run();
    void reloadFont(); // Added back for ConfigEditor to call
    void onConfigSaved(bool isStandalone = false); // Modified to take isStandalone parameter

private:
    std::string exeDir_;
    std::string configPath_;
    bool quit_ = false;
    bool showConfig_ = false;
    size_t currentIndex_ = 0;

    std::unique_ptr<SystemInitializer> system_;
    std::unique_ptr<IWindowManager> windowManager_;
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;
    std::unique_ptr<ISoundManager> soundManager_;

    std::unique_ptr<IConfigService> configManager_;
    std::unique_ptr<ConfigUI> configEditor_;
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<AssetManager> assets_;
    std::unique_ptr<ScreenshotManager> screenshotManager_;
    std::unique_ptr<IInputManager> inputManager_;
    std::vector<TableLoader> tables_;

    std::string getExecutableDir();
    bool prevShowConfig_ = false;
    bool isConfigValid();
    void runInitialConfig();
    void loadFont();
    void initializeImGui();
    void handleEvents();
    void update();
    void render();
    void cleanup();
    void initializeDependencies();
};

#endif // APP_H