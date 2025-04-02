#include "core/app.h"
#include "core/iwindow_manager.h"
#include "core/window_manager.h"
#include "core/dependency_factory.h"  // For creating components
#include "utils/logging.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>

namespace fs = std::filesystem;

App::App(const std::string& configPath) 
    : configPath_(configPath), 
      font_(nullptr, TTF_CloseFont),
      joystick_manager_(std::make_unique<JoystickManager>()) {  // Still handles joysticks
    exeDir_ = getExecutableDir();
    configPath_ = exeDir_ + configPath_;
    LOG_DEBUG("Config path: " << configPath_);
    LOG_DEBUG("Exe dir set to: " << exeDir_);
}

App::~App() {
    cleanup();
    LOG_DEBUG("App destructor completed");
}

// Main loop: Runs the app until quit is triggered
void App::run() {
    initializeDependencies();
    while (!inputManager_->shouldQuit()) {
        handleEvents();
        update();
        render();
    }
}

// Callback from ConfigUI when settings are saved
void App::onConfigSaved(bool isStandalone) {
    LOG_DEBUG("Config saved detected, forcing font reload");
    if (!isStandalone) {
        reloadFont();
        LOG_DEBUG("Font reload completed in onConfigSaved");
    } else {
        LOG_DEBUG("Skipping font reload in standalone mode");
    }
}

// Reloads font when settings change
void App::reloadFont() {
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        std::cerr << "Failed to reload font: " << TTF_GetError() << std::endl;
    } else {
        assets_->setFont(font_.get());
        const TableLoader& table = tables_[currentIndex_];
        assets_->tableNameTexture.reset(assets_->renderText(
            assets_->getPrimaryRenderer(), font_.get(), table.tableName,
            settings.fontColor, assets_->tableNameRect));
        int texWidth = 0;
        if (assets_->tableNameTexture) {
            SDL_QueryTexture(assets_->tableNameTexture.get(), nullptr, nullptr, &texWidth, nullptr);
        }
        LOG_DEBUG("Font reloaded with size " << settings.fontSize << ", texture width: " << texWidth);
    }
}

// Gets the directory of the executable
std::string App::getExecutableDir() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1) {
        LOG_DEBUG("Warning: Couldn't determine executable path, using './'");
        return "./";
    }
    path[count] = '\0';
    std::string fullPath = std::string(path);
    size_t lastSlash = fullPath.find_last_of('/');
    return (lastSlash == std::string::npos) ? "./" : fullPath.substr(0, lastSlash + 1);
}

// Checks if the config has required fields
bool App::isConfigValid() {
    return configManager_->isConfigValid();
}

// Runs the initial config wizard if config is invalid
void App::runInitialConfig() {
    SDL_Window* configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                800, 500, SDL_WINDOW_SHOWN);
    SDL_Renderer* configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    guiManager_ = std::make_unique<GuiManager>(configWindow, configRenderer);  // Temp GuiManager for setup
    guiManager_->initialize();

    bool showConfig = true;
    ConfigUI configEditor(configManager_.get(), &configManager_->getKeybindManager(), 
                          nullptr, nullptr, nullptr, this, showConfig, true);

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            guiManager_->processEvent(event);
            configEditor.handleEvent(event);
            if (event.type == SDL_QUIT) {
                std::cerr << "Config window closed without saving. Exiting..." << std::endl;
                exit(1);
            }
        }
        guiManager_->newFrame();
        configEditor.drawGUI();
        guiManager_->render(configRenderer);
        SDL_RenderPresent(configRenderer);

        if (!showConfig && isConfigValid()) break;
        else if (!showConfig) {
            std::cerr << "Configuration invalid. Please fix VPX.ExecutableCmd and VPX.TablesPath." << std::endl;
            showConfig = true;
        }
    }

    guiManager_.reset();  // Cleanup temp GuiManager
    SDL_DestroyRenderer(configRenderer);
    SDL_DestroyWindow(configWindow);
}

// Loads the font from config settings
void App::loadFont() {
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
}

// Sets up all components using the factory
void App::initializeDependencies() {
    configManager_ = DependencyFactory::createConfigService(configPath_);
    if (!isConfigValid()) {
        LOG_DEBUG("Config invalid, running initial config");
        runInitialConfig();
        configManager_->loadConfig();
    }

    windowManager_ = DependencyFactory::createWindowManager(configManager_->getSettings());
    guiManager_ = DependencyFactory::createGuiManager(windowManager_.get());
    soundManager_ = DependencyFactory::createSoundManager(exeDir_, configManager_->getSettings());
    loadFont();

    // Load tables directly here since TableManager is optional for now
    tables_ = loadTableList(configManager_->getSettings());
    if (tables_.empty()) {
        std::cerr << "Edit config.ini, no .vpx files found in " << configManager_->getSettings().vpxTablesPath << std::endl;
        exit(1);
    }

    assets_ = DependencyFactory::createAssetManager(windowManager_.get(), font_.get());
    assets_->setSettingsManager(configManager_.get());
    screenshotManager_ = DependencyFactory::createScreenshotManager(exeDir_, configManager_.get(), soundManager_.get());
    renderer_ = DependencyFactory::createRenderer(windowManager_.get());
    inputManager_ = DependencyFactory::createInputManager(configManager_.get(), screenshotManager_.get());
    configEditor_ = DependencyFactory::createConfigUI(configManager_.get(), assets_.get(), &currentIndex_, &tables_, this, showConfig_);

    inputManager_->setDependencies(assets_.get(), soundManager_.get(), configManager_.get(), 
                                   currentIndex_, tables_, showConfig_, exeDir_, screenshotManager_.get());
    inputManager_->setRuntimeEditor(configEditor_.get());
    inputManager_->registerActions();
    assets_->loadTableAssets(currentIndex_, tables_);

    LOG_DEBUG("Initialization complete");
}

// Handles SDL events (input, joystick, etc.)
void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        guiManager_->processEvent(event);
        inputManager_->handleEvent(event);
        if (showConfig_) {
            configEditor_->handleEvent(event);
        }
        if (event.type == SDL_JOYDEVICEADDED) {
            joystick_manager_->addJoystick(event.jdevice.which);
        } else if (event.type == SDL_JOYDEVICEREMOVED) {
            joystick_manager_->removeJoystick(event.jdevice.which);
        }
    }
}

// Updates app state (e.g., video cleanup, config state)
void App::update() {
    assets_->clearOldVideoPlayers();
    prevShowConfig_ = inputManager_->isConfigActive();
}

// Renders to both screens
void App::render() {
    if (renderer_ && assets_) {
        SDL_SetRenderDrawColor(windowManager_->getPrimaryRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getPrimaryRenderer());
        SDL_SetRenderDrawColor(windowManager_->getSecondaryRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getSecondaryRenderer());

        guiManager_->newFrame();
        renderer_->render(*assets_);
        if (showConfig_) {
            configEditor_->drawGUI();
        }
        guiManager_->render(windowManager_->getPrimaryRenderer());

        SDL_RenderPresent(windowManager_->getPrimaryRenderer());
        SDL_RenderPresent(windowManager_->getSecondaryRenderer());
    }
}

// Cleans up resources on shutdown
void App::cleanup() {
    LOG_DEBUG("Cleaning up");
    if (assets_) {
        VideoContext* tablePlayer = assets_->getTableVideoPlayer();
        if (tablePlayer && tablePlayer->player) {
            libvlc_media_player_stop(tablePlayer->player);
            libvlc_media_player_release(tablePlayer->player);
            tablePlayer->player = nullptr;
        }
        VideoContext* backglassPlayer = assets_->getBackglassVideoPlayer();
        if (backglassPlayer && backglassPlayer->player) {
            libvlc_media_player_stop(backglassPlayer->player);
            libvlc_media_player_release(backglassPlayer->player);
            backglassPlayer->player = nullptr;
        }
        VideoContext* dmdPlayer = assets_->getDmdVideoPlayer();
        if (dmdPlayer && dmdPlayer->player) {
            libvlc_media_player_stop(dmdPlayer->player);
            libvlc_media_player_release(dmdPlayer->player);
            dmdPlayer->player = nullptr;
        }

        cleanupVideoContext(tablePlayer);
        assets_->tableVideoPlayer = nullptr;
        cleanupVideoContext(backglassPlayer);
        assets_->backglassVideoPlayer = nullptr;
        cleanupVideoContext(dmdPlayer);
        assets_->dmdVideoPlayer = nullptr;
        assets_->clearOldVideoPlayers();
        assets_.reset();
    }
    LOG_DEBUG("Cleanup complete");
}