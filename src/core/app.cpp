#include "core/app.h"
#include "core/iwindow_manager.h"
#include "core/window_manager.h"
#include "core/dependency_factory.h"
#include "core/joystick_manager.h"
#include "core/first_run.h"
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
      joystickManager_(std::make_unique<JoystickManager>()) {
    exeDir_ = getExecutableDir();
    configPath_ = exeDir_ + configPath_;  // Make configPath absolute
    LOG_DEBUG("Config path: " << configPath_);
    LOG_DEBUG("Exe dir set to: " << exeDir_);

    // Load config early to get logFile setting
    configManager_ = DependencyFactory::createConfigService(configPath_);
    // Ensure logFile is absolute by prepending exeDir_
    std::string logFile = configManager_->getSettings().logFile;
    if (!logFile.empty() && 
        logFile.find('/') != 0 &&  // Check if not already absolute
        logFile.find('\\') != 0) {  // Windows compatibility
        logFile = exeDir_ + logFile;
    } else if (logFile.empty()) {
        logFile = exeDir_ + "logs/debug.log";  // Fallback to default absolute path
    }
    // Initialize logger with absolute path
    Logger::getInstance().initialize(logFile,
    #ifdef DEBUG_LOGGING
            true
    #else
            false
    #endif
    );
}

App::~App() {
    cleanup();
    LOG_INFO("App destructor completed");
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

void App::onConfigSaved(bool isStandalone) {
    LOG_DEBUG("Config saved detected, forcing font reload");
    if (!isStandalone) {
        reloadFont();
        LOG_DEBUG("Font reload completed in onConfigSaved");
    } else {
        LOG_DEBUG("Skipping font reload in standalone mode");
    }
}

void App::reloadFont() {
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        LOG_ERROR("Failed to reload font: " << TTF_GetError());
    } else {
        assets_->setFont(font_.get());
        const TableLoader& table = tables_[currentIndex_];
        assets_->titleTexture.reset(assets_->renderText(
            assets_->getPlayfieldRenderer(), font_.get(), table.title,
            settings.fontColor, assets_->titleRect));
        int texWidth = 0;
        if (assets_->titleTexture) {
            SDL_QueryTexture(assets_->titleTexture.get(), nullptr, nullptr, &texWidth, nullptr);
        }
        LOG_DEBUG("Font reloaded with size " << settings.fontSize << ", texture width: " << texWidth);
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
        LOG_ERROR("Failed to load font: " << TTF_GetError());
    }
}

void App::loadTables() {
    tables_ = loadTableList(configManager_->getSettings());
    if (tables_.empty()) {
        LOG_ERROR("Edit config.ini, no .vpx files found in " << configManager_->getSettings().vpxTablesPath);
        exit(1);
    }
    LOG_DEBUG("Loaded " << tables_.size() << " tables");
}

void App::initializeDependencies() {
    configManager_ = DependencyFactory::createConfigService(configPath_);
    // Logger already initialized in constructor, no need to re-initialize here
    if (!isConfigValid()) {
        LOG_INFO("Config invalid, running initial config");
        if (!runInitialConfig(configManager_.get(), configPath_)) {
            LOG_ERROR("Initial config failed or was aborted. Exiting...");
            exit(1);
        }
        configManager_->loadConfig();
    }

    windowManager_ = DependencyFactory::createWindowManager(configManager_->getSettings());
    guiManager_ = DependencyFactory::createGuiManager(windowManager_.get(), configManager_.get());
    soundManager_ = DependencyFactory::createSoundManager(exeDir_, configManager_->getSettings());

    loadFont();
    loadTables();

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

    LOG_INFO("Initialization complete");
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (!screenshotManager_->isActive()) {
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
}

void App::update() {
    assets_->clearOldVideoPlayers();
    prevShowConfig_ = inputManager_->isConfigActive();
}

void App::render() {
    if (renderer_ && assets_) {
        SDL_SetRenderDrawColor(windowManager_->getPlayfieldRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getPlayfieldRenderer());
        SDL_SetRenderDrawColor(windowManager_->getBackglassRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getBackglassRenderer());
        SDL_SetRenderDrawColor(windowManager_->getDMDRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getDMDRenderer());

        guiManager_->newFrame();
        renderer_->render(*assets_);
        if (showConfig_) {
            configEditor_->drawGUI();
        }
        guiManager_->render(windowManager_->getPlayfieldRenderer());

        SDL_RenderPresent(windowManager_->getPlayfieldRenderer());
        SDL_RenderPresent(windowManager_->getBackglassRenderer());
        SDL_RenderPresent(windowManager_->getDMDRenderer());
    }
}

void App::cleanup() {
    LOG_DEBUG("Cleaning up");
    if (assets_) {
        assets_->cleanupVideoPlayers();
        assets_.reset();
    }
    LOG_INFO("Cleanup complete");
}