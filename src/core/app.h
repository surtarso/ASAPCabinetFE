#ifndef APP_H
#define APP_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <memory>
#include <vector>
#include "table/table_manager.h"
#include "table/asset_manager.h"
#include "render/renderer.h"
#include "render/transition_manager.h"
#include "input/input_manager.h"
#include "capture/screenshot_manager.h"
#include "config/config_manager.h"
#include "config/config_gui.h"
#include "config/config_loader.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "utils/sdl_guards.h"

class App {
public:
    App();
    ~App();

    // Initialize the application (SDL, windows, renderers, resources)
    int initialize();

    // Run the main loop
    void run();

private:
    // Configuration
    std::string exeDir_;
    std::string configPath_;

    // SDL Resources
    SDLInitGuard sdlInit_;
    IMGInitGuard imgInit_;
    TTFInitGuard ttfInit_;
    MixerGuard mixerGuard_;

    // Windows and Renderers
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> primaryWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> primaryRenderer_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> secondaryWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> secondaryRenderer_;

    // Resources
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableChangeSound_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableLoadSound_;
    std::vector<Table> tables_;
    size_t currentIndex_;

    // Managers
    AssetManager assets_;
    TransitionManager transitionManager_;
    InputManager inputManager_;
    ScreenshotManager screenshotManager_;
    std::unique_ptr<ConfigManager> configManager_; // Now a unique_ptr
    std::unique_ptr<IniEditor> configEditor_;      // Now a unique_ptr
    Renderer renderer_;
    bool showConfig_;
    bool quit_;

    // Private methods
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