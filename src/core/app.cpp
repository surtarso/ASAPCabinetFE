#include "core/app.h"
#include "core/iwindow_manager.h"
#include "core/window_manager.h"
#include "core/dependency_factory.h"
#include "core/joystick_manager.h"
#include "core/first_run.h"
#include "core/playfield_overlay.h"
#include "tables/table_loader.h"
#include "utils/logging.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>
#include <thread> // Added for std::thread

namespace fs = std::filesystem;

App::App(const std::string& configPath) 
    : configPath_(configPath), 
      font_(nullptr, TTF_CloseFont),
      joystickManager_(std::make_unique<JoystickManager>()),
      tableLoader_(std::make_unique<TableLoader>()),
      isLoadingTables_{false},
      loadingProgress_(std::make_shared<LoadingProgress>()) {
    exeDir_ = getExecutableDir();
    //LOG_INFO("App: Executable directory set to " << exeDir_);
    configPath_ = exeDir_ + configPath_;
    LOG_INFO("Config Path: " << configPath_);
    std::string logFile = exeDir_ + "logs/debug.txt";
    // LOG_INFO("App: Initializing logger with file " << logFile);
    Logger::getInstance().initialize(logFile,
#ifdef DEBUG_LOGGING
            true
#else
            false
#endif
    );
    Logger::getInstance().setLoadingProgress(loadingProgress_);
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
        LOG_DEBUG("App: Loading thread joined during shutdown");
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
    LOG_DEBUG("App: Config saved detected, updating windows");
    windowManager_->updateWindows(configManager_->getSettings());
    LOG_DEBUG("App: Windows updated after config save");
}

void App::reloadFont(bool isStandalone) {
    LOG_DEBUG("App: Config saved detected, updating font");
    if (!isStandalone) {
        const Settings& settings = configManager_->getSettings();
        font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
        if (!font_) {
            LOG_ERROR("App: Failed to reload font: " << TTF_GetError());
        } else {
            assets_->setFont(font_.get());
            const TableData& table = tables_[currentIndex_];
            SDL_Rect titleRect = assets_->getTitleRect();
            titleRect.w = 0;
            titleRect.h = 0;
            assets_->reloadTitleTexture(table.title, settings.fontColor, titleRect);
        }
        LOG_DEBUG("App: Font updated after config save");
    } else {
        LOG_DEBUG("App: Skipping font reload in standalone mode");
    }
}

ISoundManager* App::getSoundManager() {
    return soundManager_.get();
}

void App::reloadAssetsAndRenderers() {
    reloadWindows();
    assets_->reloadAssets(windowManager_.get(), font_.get(), tables_, currentIndex_);
    renderer_->setRenderers(windowManager_.get());
    LOG_DEBUG("App: Assets and renderers reloaded after config saved");
}

void App::reloadTablesAndTitle() {
    const Settings& settings = configManager_->getSettings();
    std::string currentTableIndex = settings.indexPath;

    if (settings.forceRebuildMetadata) {
        LOG_DEBUG("App: execDir: " << exeDir_ << "index: " << currentTableIndex);
        std::error_code ec;
        bool removed = std::filesystem::remove(exeDir_ + currentTableIndex, ec);
        if (!removed && ec) {
            LOG_ERROR("Failed to delete " << exeDir_ + currentTableIndex << ": " << ec.message());
        } else if (removed) {
            LOG_DEBUG("Successfully deleted " << exeDir_ + currentTableIndex);
        }
    }

    LOG_DEBUG("App: Reloading tables and title texture for TitleSource change");
    loadTablesThreaded(currentIndex_); // Use threaded loading, preserve currentIndex_
}

void App::reloadOverlaySettings() {
    LOG_DEBUG("App: Reloading overlay settings");
    if (playfieldOverlay_) {
        playfieldOverlay_->updateSettings(configManager_->getSettings());
        LOG_DEBUG("App: Overlay settings reloaded");
    } else {
        LOG_ERROR("App: PlayfieldOverlay is null, cannot reload settings");
    }
}

std::string App::getExecutableDir() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1) {
        LOG_ERROR("App: Warning: Couldn't determine executable path, using './'");
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
        LOG_ERROR("App: Failed to load font: " << TTF_GetError());
    }
}

void App::loadTables() {
    loadTablesThreaded(); // Use threaded loading
}

void App::loadTablesThreaded(size_t oldIndex) {
    if (isLoadingTables_) {
        LOG_INFO("App: Table loading already in progress, skipping");
        return;
    }

    // If a previous thread exists, join it before starting a new one
    if (loadingThread_.joinable()) {
        loadingThread_.join();
    }

    isLoadingTables_ = true;
    Logger::getInstance().setLoadingProgress(loadingProgress_);
    
    loadingThread_ = std::thread([this, oldIndex]() {
        try {
            auto loadedTables = tableLoader_->loadTableList(configManager_->getSettings(), loadingProgress_.get());
            if (loadedTables.empty()) {
                LOG_ERROR("App: No .vpx files found in " << configManager_->getSettings().VPXTablesPath);
                std::lock_guard<std::mutex> lock(loadingMutex_);
                isLoadingTables_ = false;
                Logger::getInstance().setLoadingProgress(nullptr);
                loadingCV_.notify_all();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(tablesMutex_);
                tables_ = std::move(loadedTables);
                currentIndex_ = (oldIndex >= tables_.size()) ? tables_.size() - 1 : oldIndex;
            }

            SDL_Event event;
            event.type = SDL_USEREVENT;
            SDL_PushEvent(&event);

            {
                std::lock_guard<std::mutex> lock(loadingMutex_);
                isLoadingTables_ = false;
                Logger::getInstance().setLoadingProgress(nullptr);
                LOG_INFO("Loaded " << tables_.size() << " table(s).");
            }
            loadingCV_.notify_all();
        } catch (const std::exception& e) {
            LOG_ERROR("App: Exception in loading thread: " << e.what());
            std::lock_guard<std::mutex> lock(loadingMutex_);
            isLoadingTables_ = false;
            Logger::getInstance().setLoadingProgress(nullptr);
            loadingCV_.notify_all();
        }
    });
}

void App::initializeDependencies() {
    configManager_ = DependencyFactory::createConfigService(configPath_);
    if (!isConfigValid()) {
        LOG_INFO("App: Config invalid, running initial config");
        if (!runInitialConfig(configManager_.get(), configPath_)) {
            LOG_ERROR("App: Initial config failed or was aborted. Exiting...");
            exit(1);
        }
        configManager_->loadConfig();
    }

    windowManager_ = DependencyFactory::createWindowManager(configManager_->getSettings());
    guiManager_ = DependencyFactory::createGuiManager(windowManager_.get(), configManager_.get());
    soundManager_ = DependencyFactory::createSoundManager(exeDir_, configManager_->getSettings());

    if (!configManager_->getSettings().ambienceSound.empty()) {
        soundManager_->playAmbienceMusic(configManager_->getSettings().ambienceSound);
    }

    loadFont();
    loadingScreen_ = std::make_unique<LoadingScreen>(loadingProgress_);
    loadTables();

    assets_ = DependencyFactory::createAssetManager(windowManager_.get(), font_.get(), configManager_.get(), currentIndex_, tables_, soundManager_.get());
    screenshotManager_ = DependencyFactory::createScreenshotManager(exeDir_, configManager_.get(), soundManager_.get());
    renderer_ = DependencyFactory::createRenderer(windowManager_.get());
    inputManager_ = DependencyFactory::createInputManager(configManager_.get(), screenshotManager_.get());
    inputManager_->setDependencies(assets_.get(), soundManager_.get(), configManager_.get(), 
                                   currentIndex_, tables_, showConfig_, exeDir_, screenshotManager_.get(),
                                   windowManager_.get(), isLoadingTables_); // Updated

    configEditor_ = DependencyFactory::createConfigUI(configManager_.get(), assets_.get(), &currentIndex_, &tables_, this, showConfig_);

    playfieldOverlay_ = std::make_unique<PlayfieldOverlay>(
        &tables_, &currentIndex_, configManager_.get(), windowManager_.get(), assets_.get()
    );

    inputManager_->setRuntimeEditor(configEditor_.get());
    inputManager_->registerActions();

    LOG_INFO("Initialization completed.");
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (!screenshotManager_->isActive()) {
            guiManager_->processEvent(event);
            ImGuiIO& io = ImGui::GetIO();
            if (event.type == SDL_TEXTINPUT && io.WantCaptureKeyboard) {
                LOG_DEBUG("App: Consuming SDL_TEXTINPUT event due to ImGui WantCaptureKeyboard");
                continue;
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
            // Handle custom event for asset reloading
            if (event.type == SDL_USEREVENT) {
                assets_->reloadAssets(windowManager_.get(), font_.get(), tables_, currentIndex_);
                LOG_DEBUG("App: Assets reloaded after table loading");
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
        LOG_ERROR("App::render: renderer_ or assets_ is null");
        return;
    }

    const Settings& settings = configManager_->getSettings();
    SDL_Renderer* playfieldRenderer = windowManager_->getPlayfieldRenderer();
    SDL_Renderer* backglassRenderer = settings.showBackglass ? windowManager_->getBackglassRenderer() : nullptr;
    SDL_Renderer* dmdRenderer = settings.showDMD ? windowManager_->getDMDRenderer() : nullptr;
    SDL_Renderer* topperRenderer = settings.showTopper ? windowManager_->getTopperRenderer() : nullptr;

    if (!playfieldRenderer) {
        LOG_ERROR("App::render: playfieldRenderer is null");
        return;
    }

    SDL_SetRenderDrawColor(playfieldRenderer, 0, 0, 0, 255);
    SDL_RenderClear(playfieldRenderer);
    
    if (settings.showBackglass && backglassRenderer) {
        SDL_SetRenderDrawColor(backglassRenderer, 0, 0, 0, 255);
        SDL_RenderClear(backglassRenderer);
    } else if (settings.showBackglass) {
        LOG_DEBUG("App::render: backglassRenderer is null but showBackglass is true");
    }
    
    if (settings.showDMD && dmdRenderer) {
        SDL_SetRenderDrawColor(dmdRenderer, 0, 0, 0, 255);
        SDL_RenderClear(dmdRenderer);
    } else if (settings.showDMD) {
        LOG_DEBUG("App::render: dmdRenderer is null but showDMD is true");
    }

    if (settings.showTopper && topperRenderer) {
        SDL_SetRenderDrawColor(topperRenderer, 0, 0, 0, 255);
        SDL_RenderClear(topperRenderer);
    } else if (settings.showTopper) {
        LOG_DEBUG("App::render: topperRenderer is null but showTopper is true");
    }

    guiManager_->newFrame();
    
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
    }
    
    guiManager_->render(playfieldRenderer);

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