#include "editor/editor.h"
#include "log/logging.h"
#include "core/dependency_factory.h"
#include "core/ui/imgui_manager.h"
#include "data/asapcab/asapcab_index_manager.h"
#include "panels/metadata_view/metadata_view.h"
#include "tables/table_loader.h"
#include "tables/table_patcher.h"
#include "panels/vpsdb_catalog/vpsdb_catalog_manager.h"
#include "panels/vpsdb_catalog/vpsdb_catalog_json.h"
#include "config/ui/config_ui.h"
#include "utils/version_checker.h"
#include "version.h"
#include <SDL.h>
#include <stdexcept>
#include <thread>

Editor::Editor(const std::string& configPath, const std::string& exeDir)
    : showMetadataEditor_(false),
      showMetadataView_(false),
      showVpsdbBrowser_(false),
      showEditorSettings_(false),
      showDownloadMediaPanel_(false),
      hotReload_(false),
      configPath_(configPath),
      exeDir_(exeDir),
      window_(nullptr),
      renderer_(nullptr),
      imguiManager_(nullptr),
      loadingProgress_(std::make_shared<LoadingProgress>()),
      vpsdbCatalog_(nullptr),
      vpsdbJsonLoader_(nullptr),
      tablePatcher_(nullptr)
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
    tablePatcher_ = std::make_unique<TablePatcher>();

    overrideManager_ = std::make_unique<TableOverrideManager>();

    // Create sound manager for editor (only for metadataView preview)
    soundManager_ = DependencyFactory::createSoundManager(config_->getSettings());

    // Then pass it to screenshotManager if needed, or leave nullptr if not
    screenshotManager_ = DependencyFactory::createScreenshotManager(exeDir_, config_.get(), keybindProvider_.get(), soundManager_.get());

    MediaPreview::instance().setCacheDir(config_.get()->getSettings().previewCacheDir);

    // Create Editor UI
    editorUI_ = std::make_unique<EditorUI>(
        showMetadataEditor_,
        showMetadataView_,
        showVpsdbBrowser_,
        showEditorSettings_,
        showDownloadMediaPanel_,
        hotReload_,
        config_.get(),
        tableLoader_.get(),
        tableLauncher_.get(),
        tableCallbacks_.get(),
        loadingProgress_,
        screenshotManager_.get(),
        tablePatcher_.get()
    );

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

    // --- Version check ---
    VersionChecker versionChecker(ASAPCABINETFE_VERSION_STRING,
                                  "https://raw.githubusercontent.com/surtarso/ASAPCabinetFE/main/latest_version.txt");

    versionChecker.setUpdateCallback([this](const std::string& latest){
        this->latestVersion_ = latest;
        this->showUpdateModal_ = true;
    });

    // Launch async so we don't block editor start
    std::thread([vc = std::move(versionChecker)]() mutable {
        vc.checkForUpdate();
    }).detach();
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

        if (hotReload_) {
            std::string exePath = config_->getSettings().exeDir + "ASAPCabinetFE";
            configUI_->restartApp(exePath.c_str());
        }

        // ========================= Version update modal =========================
        if (showUpdateModal_) {
            ImGui::OpenPopup("Update Available!");
        }

        if (ImGui::BeginPopupModal("Update Available!", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("A new version of ASAPCabinetFE is available!");
            ImGui::Separator();
            ImGui::Text("Current: %s", VersionChecker::normalizeVersion(ASAPCABINETFE_VERSION_STRING).c_str());
            ImGui::Text("Latest: %s", latestVersion_.c_str());
            ImGui::Separator();

            if (ImGui::Button("Download")) {
                SDL_OpenURL("https://github.com/surtarso/ASAPCabinetFE/releases/latest");
            }

            // --- MOCK self updater button
            // ImGui::SameLine();
            // if (ImGui::Button("Update")) {
            //     fs::path tempDir = fs::temp_directory_path() / "ASAPCabinetFE_Update";
            //     fs::create_directories(tempDir);

            //     fs::path archivePath = tempDir / "update.tar.gz";
            //     std::string url = "https://github.com/surtarso/ASAPCabinetFE/releases/latest/download/ASAPCabinetFE.tar.gz";

            //     // Use downloader (returns bool)
            //     // if (myDownloader.download(url, archivePath)) {
            //     //     // Extract downloaded archive to exeDir
            //     //     extractTarGz(archivePath, config_->getSettings().exeDir);

            //     //     // Trigger app restart
            //     //     hotReload_ = true;
            //     // } else {
            //     //     showError("Failed to download update.");
            //     // }

            //     // Close the popup either way
            //     showUpdateModal_ = false;
            //     ImGui::CloseCurrentPopup();
            // }

            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                showUpdateModal_ = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        //
        if (showDownloadMediaPanel_) {
            ImGui::OpenPopup("Media Download");
        }
        if (ImGui::BeginPopupModal("Media Download", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select Options:");
            ImGui::Separator();

            // Resize VPIN images to current windows sizes
            ImGui::Checkbox("Resize VPinMedia Images to current Windows Settings.",
                &config_->getMutableSettings().resizeToWindows);

            ImGui::Text("Select Media to Download:");
            ImGui::Separator();

            // Launchbox (for Topper Logos)
            ImGui::Checkbox("Download Game Logo for Metadata based Topper",
                &config_->getMutableSettings().downloadTopperLogoImage);

            // Launchbox Flyers (front/back)
            ImGui::Checkbox("Download Flyer Images (Front/Back)",
                &config_->getMutableSettings().downloadFlyersImage);

            // Vpin Playfield
            ImGui::Checkbox("Download Playfield Images",
                &config_->getMutableSettings().downloadPlayfieldImage);

            // This was already correct
            ImGui::Checkbox("Download Backglass Images" ,
                &config_->getMutableSettings().downloadBackglassImage);

            // Vpin DMD
            ImGui::Checkbox("Download Full-DMD Images (4:3)",
                &config_->getMutableSettings().downloadDmdImage);

            // VPin Wheel
            ImGui::Checkbox("Download Wheel Images",
                &config_->getMutableSettings().downloadWheelImage);

            ImGui::Separator();

            if (ImGui::Button("Download")) {
                // configUI_->saveConfig(); (we dont save here or we override user options)
                editorUI_->setScannerMode(ScannerMode::MediaDb);
                showDownloadMediaPanel_ = false;
                ImGui::CloseCurrentPopup();
                editorUI_->rescanAsyncPublic(editorUI_->scannerMode());
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                showDownloadMediaPanel_ = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // --- Choose which panel to render ---
        if (editorUI_->loading()) {
            // If loading, render the loading screen
            loadingScreen_->render();
        // If not loading, render the normal editor UI logic
        } else {

            // ========================= METADATA EDITOR PANEL =========================
            if (showMetadataEditor_) {

                if (!metadataEditor_) {

                    int filteredIndex = editorUI_->selectedIndex();

                    TableData* realTable = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(editorUI_->tableMutex());
                        auto& filtered = editorUI_->filteredTables();
                        const std::string& selectedPath = filtered[filteredIndex].vpxFile;

                        // Map filtered -> master
                        for (auto& t : editorUI_->tables()) {
                            if (t.vpxFile == selectedPath) {
                                realTable = &t;
                                break;
                            }
                        }
                    }

                    // No fallback ImGui UI. You ONLY reach this if the table is valid.
                    metadataEditor_ = std::make_unique<TableOverrideEditor>(*realTable, *overrideManager_);
                }

                if (metadataEditor_) {
                    if (!metadataEditor_->render()) {
                        bool saved = metadataEditor_->wasSaved();
                        metadataEditor_.reset();
                        showMetadataEditor_ = false;

                        // if (saved) {  // only called on save, not delete.
                            {
                                std::lock_guard<std::mutex> lock(editorUI_->tableMutex());

                                int filteredIndex = editorUI_->selectedIndex();
                                auto& filtered = editorUI_->filteredTables();
                                const std::string& selectedPath = filtered[filteredIndex].vpxFile;

                                for (auto& t : editorUI_->tables()) {

                                    // Match the same table used by TableOverrideEditor
                                    if (t.vpxFile == selectedPath) {
                                        // REAPPLY THE OVERRIDE STATE TO THE MASTER TABLE
                                        overrideManager_->applyOverrides(t);  // ← THIS SETS isManualVpsId CORRECTLY

                                        // Update UI flag
                                        t.hasOverride = overrideManager_->overrideFileExists(t);

                                        // FORCE INDEX SAVE — THIS IS THE KEY
                                        tableCallbacks_->save(config_->getSettings(), editorUI_->tables(), nullptr);
                                        break;
                                    }
                                }
                            }
                            editorUI_->filterAndSortTablesPublic();
                        // }
                    }
                }

            // ========================= METADATA VIEW PANEL =========================
            } else if (showMetadataView_) {

                static MetadataView metadataView;

                int filteredIndex = editorUI_->selectedIndex();
                TableData* realTable = nullptr;

                {
                    std::lock_guard<std::mutex> lock(editorUI_->tableMutex());
                    auto& filtered = editorUI_->filteredTables();
                    const std::string& selectedPath = filtered[filteredIndex].vpxFile;

                    for (auto& t : editorUI_->tables()) {
                        if (t.vpxFile == selectedPath) {
                            realTable = &t;
                            break;
                        }
                    }
                }

                const Settings& settings = config_->getSettings();

                int width = 0, height = 0;
                SDL_GetWindowSize(window_, &width, &height);
                metadataView.setSoundManager(soundManager_.get());
                metadataView.render(*realTable, width, height, settings, renderer_);

                // Floating close/edit buttons
                ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 200.0f,
                                            ImGui::GetIO().DisplaySize.y - 50.0f));
                ImGui::SetNextWindowBgAlpha(0.3f);
                ImGui::Begin("Close Metadata", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoDecoration
                );

                if (ImGui::Button("Edit Metadata")) {
                    showMetadataView_ = false;
                    showMetadataEditor_ = true;
                }

                ImGui::SameLine();

                if (ImGui::Button("Close"))
                    showMetadataView_ = false;

                ImGui::End();

            // ========================= VPSDB PANEL =========================
            } else if (showVpsdbBrowser_) {
                if (!vpsdbCatalog_) {
                    // Lazy initialization
                    LOG_DEBUG("Editor: Initializing VpsdbCatalog...");
                    vpsdbJsonLoader_ = std::make_unique<vpsdb::VpsdbJsonLoader>(config_->getSettings());
                    vpsdbCatalog_ = std::make_unique<vpsdb::VpsdbCatalog>(renderer_, // Use the Editor's main renderer
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

            // ========================= SETTINGS PANEL =========================
            } else if (showEditorSettings_) {
                if (configUI_) {
                    configUI_->drawGUI();
                    // The showEditorSettings_ flag is passed by reference to ConfigUI.
                    // When ConfigUI's internal flag is toggled (e.g., closing the window),
                    // the reference is updated. We then save the config (or not) and refresh state.
                    if (configUI_->shouldClose() && showEditorSettings_ == false) {
                        configUI_->refreshUIState(); // Refresh UI state after closing
                    }
                } else {
                    ImGui::Text("ConfigUI failed to initialize.");
                    if (ImGui::Button("Close Settings")) showEditorSettings_ = false;
                }

            } else {
                // ========================= EDITOR UI =========================
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
