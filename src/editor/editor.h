#pragma once
#include "iapp.h"
#include "dependency_factory.h"
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
    bool loadingTables_ = false;
    bool loadingComplete_ = false;

    // SDL + ImGui
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    float dpiScale_ = 1.0f;

    // Async loading
    std::thread loadingThread_;

    // ASAPCabinetFE dependencies (reused config + table structure)
    std::unique_ptr<IConfigService> config_;
    std::unique_ptr<IKeybindProvider> keybindProvider_;
    std::unique_ptr<ITableLoader> tableLoader_;
};
