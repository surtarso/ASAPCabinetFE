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
#include "keybinds/keybind_manager.h" // Updated include
#include "render/renderer.h"
#include "render/asset_manager.h"
#include "render/table_loader.h"
#include "capture/screenshot_manager.h"

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
    std::map<char, size_t> letterIndex; // For JumpNextLetter/JumpPrevLetter

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> primaryWindow_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> secondaryWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> primaryRenderer_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> secondaryRenderer_;
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableChangeSound_;
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableLoadSound_;

    // Joystick management
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
    void createWindowsAndRenderers();
    void initializeImGui();
    void loadResources();
    void initializeActionHandlers();
    void handleEvents();
    void update();
    void render();
    void cleanup();
    void initializeDependencies();
};

#endif // APP_H