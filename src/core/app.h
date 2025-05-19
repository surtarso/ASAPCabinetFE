#ifndef APP_H
#define APP_H

#include <memory>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "config/iconfig_service.h"
#include "config/ui/config_gui.h"
#include "keybinds/iinput_manager.h"
#include "render/irenderer.h"
#include "render/iasset_manager.h"
#include "render/itable_loader.h"
#include "render/table_data.h"
#include "capture/iscreenshot_manager.h"
#include "core/iwindow_manager.h"
#include "core/joystick_manager.h"
#include "sound/isound_manager.h"
#include "core/gui_manager.h"
#include "core/dependency_factory.h"
#include "core/iapp_callbacks.h"

class App : public IAppCallbacks {  // Inherit from IAppCallbacks
public:
    App(const std::string& configPath);
    ~App();
    void run();
    void reloadFont(bool isStandalone = false) override;         
    void reloadWindows() override;                               
    void reloadAssetsAndRenderers() override;                    
    void reloadTablesAndTitle() override;                        

private:
    std::string exeDir_;
    std::string configPath_;
    bool showConfig_ = false;
    size_t currentIndex_ = 0;

    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;
    std::unique_ptr<JoystickManager> joystickManager_;
    std::unique_ptr<IWindowManager> windowManager_;
    std::unique_ptr<GuiManager> guiManager_;
    std::unique_ptr<ISoundManager> soundManager_;
    std::unique_ptr<IConfigService> configManager_;
    std::unique_ptr<ConfigUI> configEditor_;
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IAssetManager> assets_;
    std::unique_ptr<IScreenshotManager> screenshotManager_;
    std::unique_ptr<IInputManager> inputManager_;
    std::unique_ptr<ITableLoader> tableLoader_;
    std::vector<TableData> tables_;

    std::string getExecutableDir();
    bool prevShowConfig_ = false;
    bool isConfigValid();
    void loadTables();
    void loadFont();
    void handleEvents();
    void update();
    void render();
    void cleanup();
    void initializeDependencies();
};

#endif // APP_H