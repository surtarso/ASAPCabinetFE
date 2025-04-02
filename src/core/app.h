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
    App(const std::string& configPath);
    ~App();
    void run();
    void reloadFont();           // Called by ConfigUI when font settings change
    void onConfigSaved(bool isStandalone = false); // Callback for config save events

private:
    // Core app state
    std::string exeDir_;         // Directory where the executable lives
    std::string configPath_;     // Path to config.ini
    bool quit_ = false;          // Flag to exit the main loop
    bool showConfig_ = false;    // Toggle for showing config UI
    size_t currentIndex_ = 0;    // Index of the current table in tables_

    // Major components (owned by App, ordered as initialized)
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> font_; // Font for text rendering
    std::unique_ptr<JoystickManager> joystick_manager_;  // Manages joystick initialization
    std::unique_ptr<IWindowManager> windowManager_; // Handles SDL windows and renderers
    std::unique_ptr<GuiManager> guiManager_;        // Manages ImGui lifecycle
    std::unique_ptr<ISoundManager> soundManager_;   // Sound playback system
    std::unique_ptr<IConfigService> configManager_; // Config file management
    std::unique_ptr<ConfigUI> configEditor_;        // UI for editing config
    std::unique_ptr<IRenderer> renderer_;           // Renders assets to screens
    std::unique_ptr<AssetManager> assets_;          // Manages textures and media
    std::unique_ptr<ScreenshotManager> screenshotManager_; // Screenshot capture logic
    std::unique_ptr<IInputManager> inputManager_;   // Handles user input
    std::vector<TableLoader> tables_;               // List of VPX tables

    // Private helpers
    std::string getExecutableDir();     // Gets the app's executable directory
    bool prevShowConfig_ = false;       // Tracks previous config UI state
    bool isConfigValid();               // Checks if config is usable
    void runInitialConfig();            // Runs setup wizard if config is invalid
    void loadFont();                    // Loads font from settings
    void handleEvents();                // Processes SDL events
    void update();                      // Updates app state
    void render();                      // Draws to screens
    void cleanup();                     // Frees resources on shutdown
    void initializeDependencies();      // Sets up all components
};

#endif // APP_H