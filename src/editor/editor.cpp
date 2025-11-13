#include "editor/editor.h"
#include "log/logging.h"
#include "core/dependency_factory.h"
#include "core/ui/imgui_manager.h"
#include "core/ui/metadata_panel.h"
#include "tables/table_loader.h"
#include "tables/asap_index_manager.h"
#include "tables/vpsdb/vpsdb_catalog_manager.h"
#include "tables/vpsdb/vpsdb_catalog_json.h"
#include "config/ui/config_ui.h"
#include <SDL.h>
#include <stdexcept>

Editor::Editor(const std::string& configPath)
    : showMetadataEditor_(false),
      showMetadataView_(false),
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

    // Initialize ImGui using the shared manager (no manual backend calls) [why not create with factory?]
    imguiManager_ = std::make_unique<ImGuiManager>(window_, renderer_, config_.get());
    imguiManager_->initialize();

    // Initialize the loading screen UI, passing it the shared progress object
    loadingScreen_ = std::make_unique<LoadingScreen>(loadingProgress_);

    // Create Table Loader and Launcher after GUI is ready
    tableLoader_ = std::make_unique<TableLoader>();
    tableLauncher_ = DependencyFactory::createTableLauncher(config_.get());
    tableCallbacks_ = std::make_unique<AsapIndexManager>(config_->getSettings());

    overrideManager_ = std::make_unique<TableOverrideManager>();

    // Create Editor UI
    editorUI_ = std::make_unique<EditorUI>(
        showMetadataEditor_,
        showMetadataView_,
        showVpsdbBrowser_,
        showEditorSettings_,
        config_.get(),
        tableLoader_.get(),
        tableLauncher_.get(),
        tableCallbacks_.get(),
        loadingProgress_
    );

    // Use DependencyFactory to create ConfigUI ---
    // Pass nullptr for IAssetManager* and App* (which is IAppCallbacks* in this context),
    // as the Editor doesn't manage those directly?
    configUI_ = DependencyFactory::createConfigUI(
        config_.get(),
        keybindProvider_.get(),
        nullptr, // IAssetManager*
        &dummyCurrentIndex_,
        &dummyTables_,
        nullptr, // App* (IAppCallbacks*)
        showEditorSettings_
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

            // Pass events to ConfigUI for keybinding capture
            if (configUI_) {
                configUI_->handleEvent(event);
            }

            if (event.type == SDL_QUIT)
                return;
        }

        imguiManager_->newFrame();

        // Choose which panel to render
        if (editorUI_->loading()) {
            // If loading, render the loading screen
            loadingScreen_->render();
        // If not loading, render the normal editor UI logic
        } else {
            // METADATA EDITOR (TODO: hook from tables/table_overrides)
            if (showMetadataEditor_) {
                // Lazy-create metadataEditor_ using the selected table from EditorUI.
                // Do NOT invent accessor methods — use tables() and selectedIndex() that already exist.
                if (!metadataEditor_) {
                    int idx = editorUI_->selectedIndex();
                    // Validate selected index and table vector bounds under the table mutex.
                    {
                        std::lock_guard<std::mutex> lock(editorUI_->tableMutex());
                        auto &tables = editorUI_->tables();
                        if (idx >= 0 && static_cast<size_t>(idx) < tables.size()) {
                            auto &table = tables[idx];
                            metadataEditor_ = std::make_unique<TableOverrideEditor>(table, *overrideManager_);
                        } else {
                            // No valid selection: show a small notice and a close button.
                            ImGui::Text("No table selected for metadata editing.");
                            if (ImGui::Button("Close")) {
                                showMetadataEditor_ = false;
                            }
                        }
                    }
                }

                // If we have an instance, render it. The editor returns false when it requests to be closed.
                if (metadataEditor_) {
                    if (!metadataEditor_->render()) {
                        // Editor requested close (Save or Discard). If saved, update UI view.
                        bool saved = metadataEditor_->wasSaved();

                        // Destroy the editor instance.
                        metadataEditor_.reset();

                        // Close the UI panel.
                        showMetadataEditor_ = false;

                        // If saved, refresh EditorUI table view so overridden values appear.
                        // Use existing public method filterAndSortTablesPublic() to refresh filteredTables.
                        if (saved) {
                            editorUI_->filterAndSortTablesPublic();
                        }
                    }
                }

            // METADATA PANEL (TODO NEW PANEL)
            } else if (showMetadataView_) {
                // Lazy-create metadata panel
                static MetadataPanel metadataPanel; // persistent instance

                // Lock editorUI_ tables to get selected table
                std::lock_guard<std::mutex> lock(editorUI_->tableMutex());
                auto &tables = editorUI_->tables();
                int idx = editorUI_->selectedIndex();

                if (idx >= 0 && static_cast<size_t>(idx) < tables.size()) {
                    TableData &table = tables[idx];

                    // Retrieve current SDL window size for adaptive layout
                    int width = 0, height = 0;
                    SDL_GetWindowSize(window_, &width, &height);

                    // Use your ConfigService settings object (already held by config_)
                    const Settings &settings = config_->getSettings();

                    // Render the metadata panel (auto detects portrait/landscape)
                    metadataPanel.render(table, width, height, settings);
                } else {
                    ImGui::Text("No table selected.");
                }

                // Bottom-right close button overlay TODO: better position, add one to switch to override manager
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 180.0f, ImGui::GetIO().DisplaySize.y - 50.0f));
                ImGui::SetNextWindowBgAlpha(0.3f);
                ImGui::Begin("Close Metadata", nullptr,
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                if (ImGui::Button("Close Metadata View")) showMetadataView_ = false;
                ImGui::End();

            // VPSDB PANEL
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

            // SETTINGS PANEL (config_ui)
            } else if (showEditorSettings_) {
                if (configUI_) {
                    configUI_->drawGUI();
                    // The showEditorSettings_ flag is passed by reference to ConfigUI.
                    // When ConfigUI's internal flag is toggled (e.g., closing the window),
                    // the reference is updated. We then save the config and refresh state.
                    if (configUI_->shouldClose() && showEditorSettings_ == false) {
                        configUI_->saveConfig(); // Save configuration when closing
                        configUI_->refreshUIState(); // Refresh UI state after closing
                    }
                } else {
                    ImGui::Text("ConfigUI failed to initialize.");
                    if (ImGui::Button("Close Settings")) showEditorSettings_ = false;
                }

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
