#include "editor/editor.h"
#include "log/logging.h"
#include "core/dependency_factory.h"
#include "core/ui/imgui_manager.h"
#include "tables/table_loader.h"
#include "tables/vpsdb/vpsdb_catalog_manager.h"
#include "tables/vpsdb/vpsdb_catalog_json.h"
#include <SDL.h>
#include <stdexcept>

Editor::Editor(const std::string& configPath)
    : showMetadataEditor_(false),
      showVpsdbBrowser_(false),
      showEditorSettings_(false),
      configPath_(configPath),
      window_(nullptr),
      renderer_(nullptr),
      imguiManager_(nullptr),
      loadingProgress_(std::make_shared<LoadingProgress>()),
      vpsdbCatalog_(nullptr),
      vpsdbJsonLoader_(nullptr)
    {
    initializeSDL();

    // Load ASAPCabinetFE configuration through shared interfaces
    auto keybindProvider = DependencyFactory::createKeybindProvider();
    config_ = DependencyFactory::createConfigService(configPath_, keybindProvider.get());
    keybindProvider_ = std::move(keybindProvider);

    inputManager_ = DependencyFactory::createInputManager(keybindProvider_.get());

    // Initialize ImGui using the shared manager (no manual backend calls)
    imguiManager_ = std::make_unique<ImGuiManager>(window_, renderer_, config_.get());
    imguiManager_->initialize();

    // Initialize the loading screen UI, passing it the shared progress object
    loadingScreen_ = std::make_unique<LoadingScreen>(loadingProgress_);

    // Create Table Loader and Launcher after GUI is ready
    tableLoader_ = std::make_unique<TableLoader>();
    tableLauncher_ = DependencyFactory::createTableLauncher(config_.get());

    // Create Editor UI
    editorUI_ = std::make_unique<EditorUI>(
        showMetadataEditor_,
        showVpsdbBrowser_,
        showEditorSettings_,
        config_.get(),
        tableLoader_.get(),
        tableLauncher_.get(),
        loadingProgress_
    );

    LOG_INFO("Editor initialized successfully.");
}

Editor::~Editor() {
    cleanup();
}

void Editor::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        LOG_ERROR("SDL_Init Error: " + std::string(SDL_GetError()));
        throw std::runtime_error("Editor SDL initialization failed");
    }

    window_ = SDL_CreateWindow(
        "ASAPCabinetFE Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window_) {
        throw std::runtime_error("Failed to create SDL window");
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        throw std::runtime_error("Failed to create SDL renderer");
    }

    SDL_StartTextInput(); // Enables text input events
    LOG_INFO("SDL initialized for Editor.");
}

void Editor::mainLoop() {
    while (!editorUI_->shouldExit()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            imguiManager_->processEvent(event);
            if (event.type == SDL_QUIT)
                return;
        }

        imguiManager_->newFrame();

        // Choose which panel to render
        if (editorUI_->loading()) {
            // If loading, render the loading screen
            loadingScreen_->render();
        } else {
            // If not loading, render the normal editor UI logic
            if (showMetadataEditor_) {
                ImGui::Text("Metadata Editor would be here");
                if (ImGui::Button("Close Meta")) showMetadataEditor_ = false;
            } else if (showVpsdbBrowser_) {
                if (!vpsdbCatalog_) {
                    // Lazy initialization
                    LOG_DEBUG("Editor: Initializing VpsdbCatalog...");
                    vpsdbJsonLoader_ = std::make_unique<vpsdb::VpsdbJsonLoader>(config_->getSettings().vpsDbPath,
                                                                            config_->getSettings());
                    vpsdbCatalog_ = std::make_unique<vpsdb::VpsdbCatalog>(config_->getSettings().vpsDbPath,
                                                                        renderer_, // Use the Editor's main renderer
                                                                        config_->getSettings(),
                                                                        *vpsdbJsonLoader_);
                    LOG_DEBUG("Editor: vpsdbCatalog and vpsdbJsonLoader initialized");
                }

                // Render the catalog. It returns false when it wants to close.
                if (!vpsdbCatalog_->render()) {
                    vpsdbCatalog_.reset();
                    vpsdbJsonLoader_.reset();
                    showVpsdbBrowser_ = false; // The flag that opened it is set back to false
                    LOG_DEBUG("Editor: Closed VpsdbCatalog and vpsdbJsonLoader");
                }
                // ImGui::Text("VPSDB Browser would be here");
                // if (ImGui::Button("Close Browser")) showVpsdbBrowser_ = false;
            } else if (showEditorSettings_) {
                ImGui::Text("Editor Settings would be here");
                if (ImGui::Button("Close Settings")) showEditorSettings_ = false;
            } else {
                editorUI_->draw();
            }
        }
        SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
        SDL_RenderClear(renderer_);
        imguiManager_->render(renderer_);
        SDL_RenderPresent(renderer_);
    }
}

void Editor::cleanup() {
    if (loadingThread_.joinable())
        loadingThread_.join();

    imguiManager_.reset();

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
    LOG_INFO("Editor cleaned up successfully.");
}

void Editor::run() {
    mainLoop();
}
