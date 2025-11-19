#include "core/app.h"
#include "core/window_manager.h"
#include "core/first_run.h"
#include "tables/table_loader.h"
#include "log/logger.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>
#include <thread>

struct SDL_Surface;

namespace fs = std::filesystem;

App::App(const std::string& configPath, bool forceSoftwareRenderer)
    : forceSoftwareRenderer_(forceSoftwareRenderer),
      exeDir_(),
      configPath_(configPath),
      showConfig_(false),
      overrideEditor_(nullptr),
      overrideManager_(),
      showEditor_(false),
      vpsdbCatalog_(nullptr),
      vpsdbJsonLoader_(nullptr),
      showVpsdb_(false),
      currentIndex_(0),
      lastTableIndex_(),
      font_(nullptr, TTF_CloseFont),
      joystickManager_(std::make_unique<JoystickManager>()),
      tableLoader_(std::make_unique<TableLoader>()),
      isLoadingTables_{false},
      loadingProgress_(std::make_shared<LoadingProgress>()) {
    exeDir_ = getExecutableDir();
    LOG_INFO("Config Path: " + configPath_);
    asap::logging::Logger::getInstance().setLoadingProgress(loadingProgress_);
}

App::~App() {
    // Ensure the loading thread is joined before destruction
    if (loadingThread_.joinable()) {
        {
            std::lock_guard<std::mutex> lock(loadingMutex_);
            isLoadingTables_ = false; // Signal thread to stop if needed
        }
        loadingCV_.notify_all();
        loadingThread_.join();
        LOG_DEBUG("Loading thread joined during shutdown");
    }
    cleanup();
}

void App::run() {
    initializeDependencies();
    while (!inputManager_->shouldQuit()) {
        handleEvents();
        if (!screenshotManager_->isActive()) {
            update();
            render();
        }
    }
}

void App::reloadWindows() {
    LOG_DEBUG("Config saved detected, updating windows");
    windowManager_->updateWindows(configManager_->getSettings());
    LOG_DEBUG("Windows updated after config save");
}

void App::reloadFont(bool isStandalone) {
    LOG_DEBUG("Config saved detected, updating font");
    if (!isStandalone) {
        const Settings& settings = configManager_->getSettings();
        font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
        if (!font_) {
            LOG_ERROR("Failed to reload font: " + std::string(TTF_GetError()));
        } else {
            assets_->setFont(font_.get());
            const TableData& table = tables_[currentIndex_];
            SDL_Rect titleRect = assets_->getTitleRect();
            titleRect.w = 0;
            titleRect.h = 0;
            assets_->reloadTitleTexture(table.title, settings.fontColor, titleRect);
        }
        LOG_DEBUG("Font updated after config save");
    } else {
        LOG_DEBUG("Skipping font reload in standalone mode");
    }
}

ISoundManager* App::getSoundManager() {
    return soundManager_.get();
}

void App::reloadAssetsAndRenderers() {
    reloadWindows();
    assets_->reloadAssets(windowManager_.get(), font_.get(), tables_, currentIndex_);
    renderer_->setRenderers(windowManager_.get());
    // Rebind ImGui backends to any newly created windows/renderers so UI remains visible
    if (imGuiManager_) {
        imGuiManager_->reinitialize();
    }
    LOG_DEBUG("Assets and renderers reloaded after config saved");
}

void App::reloadTablesAndTitle() {
    const Settings& settings = configManager_->getSettings();
    std::string currentTableIndex = settings.indexPath;

    if (settings.forceRebuildMetadata) {
        LOG_DEBUG("execDir: " + exeDir_ + ", index: " + currentTableIndex);
        std::error_code ec;
        bool removed = std::filesystem::remove(currentTableIndex, ec);
        if (!removed && ec) {
            LOG_ERROR("Failed to delete " + exeDir_ + currentTableIndex + ": " + ec.message());
        } else if (removed) {
            LOG_DEBUG("Successfully deleted " + exeDir_ + currentTableIndex);
        }
    }

    LOG_DEBUG("Reloading tables and title texture for TitleSource change");
    loadTablesThreaded(currentIndex_); // Use threaded loading, preserve currentIndex_
}

void App::reloadOverlaySettings() {
    LOG_DEBUG("Reloading overlay settings");
    if (playfieldOverlay_) {
        playfieldOverlay_->updateSettings(configManager_->getSettings());
        LOG_DEBUG("Overlay settings reloaded");
    } else {
        LOG_ERROR("PlayfieldOverlay is null, cannot reload settings");
    }
}

std::string App::getExecutableDir() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1) {
        LOG_ERROR("Warning: Couldn't determine executable path, using './'");
        return "./";
    }
    path[count] = '\0';
    std::string fullPath = std::string(path);
    size_t lastSlash = fullPath.find_last_of('/');
    return (lastSlash == std::string::npos) ? "./" : fullPath.substr(0, lastSlash + 1);
}

bool App::isConfigValid() {
    return configManager_->isConfigValid();
}

void App::loadFont() {
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        LOG_ERROR("Failed to load font: " + std::string(TTF_GetError()));
    }
}

void App::loadTables() {
    loadTablesThreaded(); // Use threaded loading
}

void App::loadTablesThreaded(size_t oldIndex) {
    if (isLoadingTables_) {
        LOG_DEBUG("Table loading already in progress, skipping");
        return;
    }

    // If a previous thread exists, join it before starting a new one
    if (loadingThread_.joinable()) {
        loadingThread_.join();
    }

    isLoadingTables_ = true;
    asap::logging::Logger::getInstance().setLoadingProgress(loadingProgress_);

    loadingThread_ = std::thread([this, oldIndex]() {
        try {
            auto loadedTables = tableLoader_->loadTableList(configManager_->getSettings(), loadingProgress_.get());
            if (loadedTables.empty()) {
                LOG_ERROR("No .vpx files found in " + configManager_->getSettings().VPXTablesPath);
                std::lock_guard<std::mutex> lock(loadingMutex_);
                isLoadingTables_ = false;
                asap::logging::Logger::getInstance().setLoadingProgress(nullptr);
                loadingCV_.notify_all();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(tablesMutex_);
                tables_ = std::move(loadedTables);
                currentIndex_ = (oldIndex >= tables_.size()) ? tables_.size() - 1 : oldIndex;
            }

            // Save forceRebuildMetadata as false after loading tables
            // This prevents loading the app with 'true'.
            bool flagsReset = false;
            {
                Settings& settings = const_cast<Settings&>(configManager_->getSettings());
                bool wasForceRebuild = settings.forceRebuildMetadata;
                if (wasForceRebuild) {
                    settings.forceRebuildMetadata = false;
                    configManager_->saveConfig();
                    flagsReset = true;
                    LOG_INFO("'Force Rebuild Metadata' was set to false after table loading");
                } else {
                    LOG_DEBUG("forceRebuildMetadata was already false, no reset needed");
                }
            }

            SDL_Event event;
            event.type = SDL_USEREVENT;
            SDL_PushEvent(&event);

            {
                std::lock_guard<std::mutex> lock(loadingMutex_);
                isLoadingTables_ = false;
                asap::logging::Logger::getInstance().setLoadingProgress(nullptr);
                LOG_DEBUG("Loaded " + std::to_string(tables_.size()) + " table(s).");
                if (flagsReset && playfieldOverlay_) {
                    playfieldOverlay_->ResetMetadataFlags();
                }
            }
            loadingCV_.notify_all();
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in loading thread: " + std::string(e.what()));
            std::lock_guard<std::mutex> lock(loadingMutex_);
            isLoadingTables_ = false;
            asap::logging::Logger::getInstance().setLoadingProgress(nullptr);
            loadingCV_.notify_all();
        }
    });
}

void App::initializeDependencies() {
    keybindProvider_ = DependencyFactory::createKeybindProvider();
    configManager_ = DependencyFactory::createConfigService(configPath_, keybindProvider_.get());
    // Apply CLI override to force software renderer if requested
    if (forceSoftwareRenderer_) {
        Settings& mutableSettings = configManager_->getMutableSettings();
        mutableSettings.forceSoftwareRenderer = true;
        LOG_INFO("CLI override: forcing software renderer for all windows");
    }
    if (!isConfigValid()) {
        LOG_WARN("Config invalid, running initial config...");
        if (!runInitialConfig(configManager_.get(), keybindProvider_.get(), configPath_)) {
            LOG_ERROR("Initial config failed or was aborted. Exiting...");
            exit(1);
        }
        configManager_->loadConfig();
    }

    windowManager_ = DependencyFactory::createWindowManager(configManager_->getSettings());
    imGuiManager_ = DependencyFactory::createImGuiManager(windowManager_.get(), configManager_.get());
    soundManager_ = DependencyFactory::createSoundManager(configManager_->getSettings());

    if (!configManager_->getSettings().ambienceSound.empty()) {
        if (soundManager_) soundManager_->playAmbienceMusic(configManager_->getSettings().ambienceSound);
    }

    loadFont();
    loadingScreen_ = std::make_unique<LoadingScreen>(loadingProgress_);
    loadTables();

    tableCallbacks_ = DependencyFactory::createTableCallbacks(configManager_.get());
    tableLauncher_ = DependencyFactory::createTableLauncher(configManager_.get());

    assets_ = DependencyFactory::createAssetManager(windowManager_.get(), font_.get(), configManager_.get(), currentIndex_, tables_, soundManager_.get());
    screenshotManager_ = DependencyFactory::createScreenshotManager(exeDir_, configManager_.get(), keybindProvider_.get(), soundManager_.get());
    renderer_ = DependencyFactory::createRenderer(windowManager_.get());
    inputManager_ = DependencyFactory::createInputManager(keybindProvider_.get());
    inputManager_->setDependencies(this, assets_.get(), soundManager_.get(), configManager_.get(),
                                  currentIndex_, tables_, showConfig_, showEditor_, showVpsdb_, exeDir_, screenshotManager_.get(),
                                  windowManager_.get(), isLoadingTables_, tableLauncher_.get(), tableCallbacks_.get());

    configEditor_ = DependencyFactory::createConfigUI(configManager_.get(), keybindProvider_.get(), assets_.get(), &currentIndex_, &tables_, this, showConfig_);

    playfieldOverlay_ = std::make_unique<PlayfieldOverlay>(
        &tables_, &currentIndex_, configManager_.get(), windowManager_.get(), assets_.get(),
        [this]() { if (configEditor_) configEditor_->refreshUIState(); }
    );

    inputManager_->setRuntimeEditor(configEditor_.get());
    inputManager_->registerActions();

    LOG_INFO("Initialization completed.");
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (!screenshotManager_->isActive()) {
            imGuiManager_->processEvent(event);
            ImGuiIO& io = ImGui::GetIO();
            if (event.type == SDL_TEXTINPUT && io.WantCaptureKeyboard) {
                LOG_DEBUG("Consuming SDL_TEXTINPUT event due to ImGui WantCaptureKeyboard");
                return;
            }
            inputManager_->handleEvent(event);
            if (showConfig_) {
                configEditor_->handleEvent(event);
            }
            if (event.type == SDL_JOYDEVICEADDED) {
                joystickManager_->addJoystick(event.jdevice.which);
            } else if (event.type == SDL_JOYDEVICEREMOVED) {
                joystickManager_->removeJoystick(event.jdevice.which);
            }
            if (event.type == SDL_USEREVENT) {
                assets_->reloadAssets(windowManager_.get(), font_.get(), tables_, currentIndex_);
                LOG_DEBUG("Assets reloaded after table loading");
            }
        }
    }
}

void App::update() {
    assets_->clearOldVideoPlayers();
    prevShowConfig_ = inputManager_->isConfigActive();
}

void App::render() {
    if (!renderer_ || !assets_) {
        LOG_ERROR("renderer_ or assets_ is null");
        return;
    }

    const Settings& settings = configManager_->getSettings();
    SDL_Renderer* playfieldRenderer = windowManager_->getPlayfieldRenderer();
    SDL_Renderer* backglassRenderer = settings.showBackglass ? windowManager_->getBackglassRenderer() : nullptr;
    SDL_Renderer* dmdRenderer = settings.showDMD ? windowManager_->getDMDRenderer() : nullptr;
    SDL_Renderer* topperRenderer = settings.showTopper ? windowManager_->getTopperRenderer() : nullptr;

    if (!playfieldRenderer) {
        LOG_ERROR("playfieldRenderer is null");
        return;
    }

    SDL_SetRenderDrawColor(playfieldRenderer, 0, 0, 0, 255);
    SDL_RenderClear(playfieldRenderer);

    if (settings.showBackglass && backglassRenderer) {
        SDL_SetRenderDrawColor(backglassRenderer, 0, 0, 0, 255);
        SDL_RenderClear(backglassRenderer);
    } else if (settings.showBackglass) {
        LOG_DEBUG("backglassRenderer is null but showBackglass is true");
    }

    if (settings.showDMD && dmdRenderer) {
        SDL_SetRenderDrawColor(dmdRenderer, 0, 0, 0, 255);
        SDL_RenderClear(dmdRenderer);
    } else if (settings.showDMD) {
        LOG_DEBUG("dmdRenderer is null but showDMD is true");
    }

    if (settings.showTopper && topperRenderer) {
        SDL_SetRenderDrawColor(topperRenderer, 0, 0, 0, 255);
        SDL_RenderClear(topperRenderer);
    } else if (settings.showTopper) {
        LOG_DEBUG("topperRenderer is null but showTopper is true");
    }

    imGuiManager_->newFrame();

    // Render loading screen if tables are loading
    if (isLoadingTables_) {
        // Ensure loadingScreen_ is initialized before calling render
        if (!loadingScreen_) {
            loadingScreen_ = std::make_unique<LoadingScreen>(loadingProgress_);
        }
        loadingScreen_->render(); // Call the new LoadingScreen's render method
    } else if (!tables_.empty()) {
        renderer_->render(*assets_);
        if (playfieldOverlay_) {
            playfieldOverlay_->render();
        }
        if (showConfig_) {
            configEditor_->drawGUI();
        }
        // Render TableOverrideEditor if toggled
        if (showEditor_ && currentIndex_ < tables_.size()) {
            if (!overrideEditor_ || lastTableIndex_ != currentIndex_) {
                overrideEditor_ = std::make_unique<TableOverrideEditor>(tables_[currentIndex_], overrideManager_);
                lastTableIndex_ = currentIndex_;
                LOG_DEBUG("Initialized TableOverrideEditor for table index: " + std::to_string(currentIndex_) + ", title: " + tables_[currentIndex_].title);
            }
            if (!overrideEditor_->render()) {
                if (overrideEditor_->wasSaved()) {
                    reloadTablesAndTitle();
                    LOG_DEBUG("Closed TableOverrideEditor after Save, triggered table reload");
                } else {
                    LOG_DEBUG("Closed TableOverrideEditor after Discard, no reload");
                }
                overrideEditor_.reset();
                showEditor_ = false;
            }
        }
        // Render vpsdb catalog
        if (showVpsdb_) {
            if (!vpsdbCatalog_) {
                vpsdbJsonLoader_ = std::make_unique<vpsdb::VpsdbJsonLoader>(configManager_->getSettings().vpsDbPath,
                                                                        configManager_->getSettings());
                vpsdbCatalog_ = std::make_unique<vpsdb::VpsdbCatalog>(configManager_->getSettings().vpsDbPath,
                                                                    windowManager_->getPlayfieldRenderer(),
                                                                    configManager_->getSettings(),
                                                                    *vpsdbJsonLoader_);
                LOG_DEBUG("vpsdbCatalog and vpsdbJsonLoader initialized");
            }
            if (!vpsdbCatalog_->render()) {
                vpsdbCatalog_.reset();
                vpsdbJsonLoader_.reset();
                showVpsdb_ = false;
                LOG_DEBUG("Closed VpsdbCatalog and vpsdbJsonLoader");
            }
        }
    }

    imGuiManager_->render(playfieldRenderer);

    SDL_RenderPresent(playfieldRenderer);
    if (settings.showBackglass && backglassRenderer) {
        SDL_RenderPresent(backglassRenderer);
    }
    if (settings.showDMD && dmdRenderer) {
        SDL_RenderPresent(dmdRenderer);
    }
    if (settings.showTopper && topperRenderer) {
        SDL_RenderPresent(topperRenderer);
    }
}

void App::cleanup() {
    assets_->cleanupVideoPlayers();
    assets_.reset();
    LOG_INFO("App cleanup complete.");
}
