#include "core/app.h"
#include "core/iwindow_manager.h"
#include "core/window_manager.h"
#include "core/dependency_factory.h"
#include "core/joystick_manager.h"
#include "core/first_run.h"  // For runInitialConfig
#include "utils/logging.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>

namespace fs = std::filesystem;

// Constructor: Sets up initial state and joystick manager
App::App(const std::string& configPath) 
    : configPath_(configPath), 
      font_(nullptr, TTF_CloseFont),
      joystickManager_(std::make_unique<JoystickManager>()) {
    exeDir_ = getExecutableDir();
    configPath_ = exeDir_ + configPath_;
    LOG_DEBUG("Config path: " << configPath_);
    LOG_DEBUG("Exe dir set to: " << exeDir_);
}

// Destructor: Triggers cleanup
App::~App() {
    cleanup();
    LOG_DEBUG("App destructor completed");
}

// Main loop: Runs the app until quit
void App::run() {
    initializeDependencies();
    while (!inputManager_->shouldQuit()) {
        handleEvents();
        update();
        render();
    }
}

// Callback: Handles config save events from ConfigUI
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

// Resolves the executable’s directory for relative paths
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

// Checks if config has required fields
bool App::isConfigValid() {
    return configManager_->isConfigValid();
}

// Loads font from config settings
void App::loadFont() {
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
    // Note: Not setting assets_->setFont() here—assets_ isn’t created yet
}

// Loads VPX tables from settings
void App::loadTables() {
    tables_ = loadTableList(configManager_->getSettings());
    if (tables_.empty()) {
        std::cerr << "Edit config.ini, no .vpx files found in " << configManager_->getSettings().vpxTablesPath << std::endl;
        exit(1);
    }
    LOG_DEBUG("Loaded " << tables_.size() << " tables");
}

// Initializes all components using DependencyFactory
void App::initializeDependencies() {
    // Step 1: Config setup and initial run if needed
    configManager_ = DependencyFactory::createConfigService(configPath_);
    if (!isConfigValid()) {
        LOG_DEBUG("Config invalid, running initial config");
        if (!runInitialConfig(configManager_.get(), configPath_)) {
            std::cerr << "Initial config failed or was aborted. Exiting..." << std::endl;
            exit(1);
        }
        configManager_->loadConfig();  // Reload after wizard
    }

    // Step 2: Core systems (order matters for dependencies)
    windowManager_ = DependencyFactory::createWindowManager(configManager_->getSettings());
    guiManager_ = DependencyFactory::createGuiManager(windowManager_.get());
    soundManager_ = DependencyFactory::createSoundManager(exeDir_, configManager_->getSettings());

    // Step 3: Load font and tables (app-specific logic)
    loadFont();
    loadTables();

    // Step 4: Create remaining components via factory
    assets_ = DependencyFactory::createAssetManager(windowManager_.get(), font_.get());
    assets_->setSettingsManager(configManager_.get());
    screenshotManager_ = DependencyFactory::createScreenshotManager(exeDir_, configManager_.get(), soundManager_.get());
    renderer_ = DependencyFactory::createRenderer(windowManager_.get());
    inputManager_ = DependencyFactory::createInputManager(configManager_.get(), screenshotManager_.get());
    configEditor_ = DependencyFactory::createConfigUI(configManager_.get(), assets_.get(), &currentIndex_, &tables_, this, showConfig_);

    // Step 5: Configure input manager (post-creation setup)
    inputManager_->setDependencies(assets_.get(), soundManager_.get(), configManager_.get(), 
                                   currentIndex_, tables_, showConfig_, exeDir_, screenshotManager_.get());
    inputManager_->setRuntimeEditor(configEditor_.get());
    inputManager_->registerActions();

    // Step 6: Load initial table assets
    assets_->loadTableAssets(currentIndex_, tables_);

    LOG_DEBUG("Initialization complete");
}

// Processes SDL events (input, joysticks)
void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        guiManager_->processEvent(event);
        inputManager_->handleEvent(event);
        if (showConfig_) {
            configEditor_->handleEvent(event);
        }
        if (event.type == SDL_JOYDEVICEADDED) {
            joystickManager_->addJoystick(event.jdevice.which);
        } else if (event.type == SDL_JOYDEVICEREMOVED) {
            joystickManager_->removeJoystick(event.jdevice.which);
        }
    }
}

// Updates app state (e.g., clears old video players)
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
        assets_->cleanupVideoPlayers();  // Delegate video cleanup to AssetManager
        assets_.reset();                 // Reset assets (smart pointers handle textures)
    }
    LOG_DEBUG("Cleanup complete");
}