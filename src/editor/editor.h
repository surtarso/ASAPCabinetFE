#pragma once
#include "editor/editor_ui.h"
#include "core/iapp.h"
#include "core/dependency_factory.h"
#include "launcher/itable_launcher.h"
#include <SDL2/SDL.h>
#include <thread>
#include <string>
#include <memory>

/**
 * @class Editor
 * @brief Standalone editor for table and INI management inside ASAPCabinetFE (--editor mode)
 */
class Editor : public IApp {
public:
    explicit Editor(const std::string& configPath);
    ~Editor() override;

    void run() override;

private:
    void initializeSDL();
    void initializeImGui();
    void mainLoop();
    void cleanup();

    // Core paths and state
    std::string configPath_;
    bool exitRequested_ = false;

    // SDL + ImGui
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    // Async loading
    std::thread loadingThread_;

    // ASAPCabinetFE dependencies (shared resources)
    std::unique_ptr<IConfigService> config_;
    std::unique_ptr<IKeybindProvider> keybindProvider_;
    std::unique_ptr<ITableLoader> tableLoader_;
    std::unique_ptr<ITableLauncher> tableLauncher_;
    std::unique_ptr<EditorUI> editorUI_;

    // States for sub-editors (will be used later)
    bool showMetadataEditor_ = false;
    bool showIniEditor_ = false;
};
