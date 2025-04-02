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
#include "core/joystick_manager.h"
#include "sound/isound_manager.h"
#include "core/gui_manager.h"
#include "core/dependency_factory.h"

class App {
public:
    // Constructor: Initializes app with config file path
    App(const std::string& configPath);
    
    // Destructor: Ensures proper cleanup
    ~App();
    
    // Main loop: Runs the application
    void run();
    
    // Reloads font when settings change (called by ConfigUI)
    void reloadFont();
    
    // Callback for config save events from ConfigUI
    void onConfigSaved(bool isStandalone = false);

private:
    // Core app state
    std::string exeDir_;         // Directory of the executable
    std::string configPath_;     // Path to config.ini
    bool quit_ = false;          // Flag to exit main loop
    bool showConfig_ = false;    // Toggles config UI visibility
    size_t currentIndex_ = 0;    // Index of current table in tables_

    // Major components (owned by App, ordered by init sequence)
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_;  // Font for text rendering
    std::unique_ptr<JoystickManager> joystickManager_;    // Manages joystick input
    std::unique_ptr<IWindowManager> windowManager_;       // SDL window and renderer management
    std::unique_ptr<GuiManager> guiManager_;              // ImGui lifecycle management
    std::unique_ptr<ISoundManager> soundManager_;         // Sound playback system
    std::unique_ptr<IConfigService> configManager_;       // Config file handling
    std::unique_ptr<ConfigUI> configEditor_;              // Config editing UI
    std::unique_ptr<IRenderer> renderer_;                 // Renders assets to screens
    std::unique_ptr<AssetManager> assets_;                // Manages textures and videos
    std::unique_ptr<ScreenshotManager> screenshotManager_; // Screenshot capture logic
    std::unique_ptr<IInputManager> inputManager_;         // Handles user input
    std::vector<TableLoader> tables_;                     // List of loaded VPX tables

    // Private helpers
    std::string getExecutableDir();     // Resolves executable directory
    bool prevShowConfig_ = false;       // Tracks previous config UI state
    bool isConfigValid();               // Validates config integrity
    void loadTables();                  // Loads VPX tables from settings
    void loadFont();                    // Loads font from config settings
    void handleEvents();                // Processes SDL events
    void update();                      // Updates app state
    void render();                      // Renders to screens
    void cleanup();                     // Frees resources on shutdown
    void initializeDependencies();      // Initializes all components
};

#endif // APP_H