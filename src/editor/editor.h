#pragma once

#include "editor/ui/editor_ui.h"
#include "core/iapp.h"
#include "core/dependency_factory.h"
#include "launcher/itable_launcher.h"
#include "tables/itable_callbacks.h"
#include "core/ui/loading_progress.h"
#include "core/ui/loading_screen.h"
#include <SDL2/SDL.h>
#include <thread>
#include <string>
#include <memory>
#include <atomic>

/**
 * @class Editor
 * @brief Standalone editor for table management inside ASAPCabinetFE (--editor mode)
 */

 // Forward declarations
class ImGuiManager; // forward declaration

namespace vpsdb {
    class VpsdbCatalog;
    class VpsdbJsonLoader;
}

class Editor : public IApp {
public:
    explicit Editor(const std::string& configPath);
    ~Editor() override;

    void run() override;

private:
    void initializeSDL();
    void mainLoop();
    void cleanup();

    // --- Sub-editor state flags ---
    bool showMetadataEditor_ = false;
    bool showVpsdbBrowser_ = false;
    bool showEditorSettings_ = false;

    // --- Core state ---
    std::string configPath_;
    bool exitRequested_ = false;

    // --- SDL window + renderer ---
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    // --- ImGui Manager (shared with ASAPCabinetFE) ---
    std::unique_ptr<ImGuiManager> imguiManager_;

    // --- Async loading thread ---
    std::thread loadingThread_;

    // --- ASAPCabinetFE dependencies ---
    std::unique_ptr<IConfigService> config_;
    std::unique_ptr<IKeybindProvider> keybindProvider_; /// To listen to keystrokes
    std::unique_ptr<ITableLoader> tableLoader_;
    std::unique_ptr<ITableLauncher> tableLauncher_;
    std::unique_ptr<ITableCallbacks> tableCallbacks_;
    std::unique_ptr<EditorUI> editorUI_;
    std::shared_ptr<LoadingProgress> loadingProgress_; ///< Loading progress
    std::unique_ptr<vpsdb::VpsdbCatalog> vpsdbCatalog_;
    std::unique_ptr<vpsdb::VpsdbJsonLoader> vpsdbJsonLoader_;

    std::unique_ptr<LoadingScreen> loadingScreen_;     ///< Loading screen UI
};
