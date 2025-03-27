#ifndef APP_H
#define APP_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "config/settings_manager.h"
#include "config/ui/setup_editor.h"
#include "keybinds/keybind_manager.h"
#include "render/renderer.h"
#include "render/asset_manager.h"
#include "render/table_loader.h"
#include "capture/screenshot_manager.h"

class WindowManager; // Forward declaration

class App {
public:
    App(const std::string& configPath);
    ~App();
    void run();

private:
    using ActionHandler = std::function<void()>;

    std::string exeDir_;
    std::string configPath_;
    bool quit_ = false;
    bool showConfig_ = false;
    size_t currentIndex_ = 0;
    std::map<char, size_t> letterIndex;

    std::unique_ptr<WindowManager> windowManager_; // New class for windows/renderers
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableChangeSound_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableLoadSound_;
    std::vector<SDL_Joystick*> joysticks_;

    std::unique_ptr<SettingsManager> configManager_;
    std::unique_ptr<RuntimeEditor> configEditor_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<AssetManager> assets_;
    std::unique_ptr<ScreenshotManager> screenshotManager_;
    std::vector<TableLoader> tables_;
    std::map<std::string, ActionHandler> actionHandlers_;

    std::string getExecutableDir();
    bool isConfigValid();
    void runInitialConfig();
    void initializeSDL();
    void initializeJoysticks();
    void loadSounds();
    void loadFont();
    void initializeImGui();
    void initializeActionHandlers();
    void handleEvents();
    void handleConfigEvents(const SDL_Event& event);
    void handleRegularEvents(const SDL_Event& event);
    void update();
    void render();
    void cleanup();
    void initializeDependencies();
};

#endif // APP_H