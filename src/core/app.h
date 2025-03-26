#ifndef APP_H
#define APP_H

#include <memory>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "config/config_manager.h"
#include "config/config_gui.h"
#include "input/input_manager.h"
#include "render/renderer.h"
#include "render/transition_manager.h"
#include "table/asset_manager.h"
#include "table/table_manager.h"
#include "capture/screenshot_manager.h"

class App {
public:
    App(const std::string& configPath);
    ~App();
    int initialize(int argc, char *argv[]);
    void run();

private:
    std::string exeDir_;
    std::string configPath_;
    bool quit_ = false;
    bool showConfig_ = false;
    size_t currentIndex_ = 0;

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> primaryWindow_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> secondaryWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> primaryRenderer_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> secondaryRenderer_;
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableChangeSound_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableLoadSound_;

    std::unique_ptr<ConfigManager> configManager_;
    std::unique_ptr<IniEditor> configEditor_;
    std::unique_ptr<InputManager> inputManager_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<AssetManager> assets_; // Change to unique_ptr
    std::unique_ptr<ScreenshotManager> screenshotManager_; // Change to unique_ptr
    TransitionManager transitionManager_;
    std::vector<Table> tables_;

    std::string getExecutableDir();
    bool isConfigValid();
    void runInitialConfig();
    void initializeSDL();
    void createWindowsAndRenderers();
    void initializeImGui();
    void loadResources();
    void handleEvents();
    void update();
    void render();
    void cleanup();
};

#endif // APP_H