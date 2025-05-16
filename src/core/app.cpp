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
    configPath_ = exeDir_ + configPath_;
    LOG_DEBUG("App: Config path: " << configPath_);
    LOG_DEBUG("App: Exec dir set to: " << exeDir_);

    configManager_ = DependencyFactory::createConfigService(configPath_);
    std::string logFile = configManager_->getSettings().logFile;
    if (!logFile.empty() && 
        logFile.find('/') != 0 && 
        logFile.find('\\') != 0) {
        logFile = exeDir_ + logFile;
    } else if (logFile.empty()) {
        logFile = exeDir_ + "logs/debug.log";
    }
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
    LOG_INFO("App: App destructor completed");
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
            auto* concreteAssets = dynamic_cast<AssetManager*>(assets_.get());
            if (concreteAssets) {
                concreteAssets->setFont(font_.get());
                const TableLoader& table = tables_[currentIndex_];
                concreteAssets->titleTexture.reset(concreteAssets->renderText(
                    concreteAssets->getPlayfieldRenderer(), font_.get(), table.title,
                    settings.fontColor, concreteAssets->titleRect));
                int texWidth = 0;
                if (concreteAssets->titleTexture) {
                    SDL_QueryTexture(concreteAssets->titleTexture.get(), nullptr, nullptr, &texWidth, nullptr);
                }
                LOG_DEBUG("App: Font reloaded with size " << settings.fontSize << ", texture width: " << texWidth);
            } else {
                LOG_ERROR("App: Failed to cast IAssetManager to AssetManager for font reload");
            }
        }
        LOG_DEBUG("App: Font updated after config save");
    } else {
        LOG_DEBUG("App: Skipping font reload in standalone mode");
    }
}

void App::onConfigSaved() {
    configManager_->loadConfig();
    auto* concreteAssets = dynamic_cast<AssetManager*>(assets_.get());
    if (concreteAssets) {
        concreteAssets->cleanupVideoPlayers();
        concreteAssets->setPlayfieldRenderer(windowManager_->getPlayfieldRenderer());
        concreteAssets->setBackglassRenderer(windowManager_->getBackglassRenderer());
        concreteAssets->setDMDRenderer(windowManager_->getDMDRenderer());
        concreteAssets->loadTableAssets(currentIndex_, tables_);
    } else {
        LOG_ERROR("App: Failed to cast IAssetManager to AssetManager");
    }

    Renderer* concreteRenderer = dynamic_cast<Renderer*>(renderer_.get());
    if (concreteRenderer) {
        concreteRenderer->setPlayfieldRenderer(windowManager_->getPlayfieldRenderer());
        concreteRenderer->setBackglassRenderer(windowManager_->getBackglassRenderer());
        concreteRenderer->setDMDRenderer(windowManager_->getDMDRenderer());
    } else {
        LOG_ERROR("App: Failed to cast IRenderer to Renderer");
    }

    LOG_INFO("App: Configuration saved and assets reloaded");
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
    tables_ = loadTableList(configManager_->getSettings());
    if (tables_.empty()) {
        LOG_ERROR("App: Edit config.ini, no .vpx files found in " << configManager_->getSettings().VPXTablesPath);
        exit(1);
    }
    LOG_INFO("App: Loaded " << tables_.size() << " tables");
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

    loadFont();
    loadTables();

    assets_ = DependencyFactory::createAssetManager(windowManager_.get(), font_.get());
    if (auto* concreteAssets = dynamic_cast<AssetManager*>(assets_.get())) {
        concreteAssets->setSettingsManager(configManager_.get());
        concreteAssets->loadTableAssets(currentIndex_, tables_);
    } else {
        LOG_ERROR("App: Failed to cast IAssetManager to AssetManager");
        exit(1);
    }
    screenshotManager_ = DependencyFactory::createScreenshotManager(exeDir_, configManager_.get(), soundManager_.get());
    renderer_ = DependencyFactory::createRenderer(windowManager_.get());
    inputManager_ = DependencyFactory::createInputManager(configManager_.get(), screenshotManager_.get());
    configEditor_ = DependencyFactory::createConfigUI(configManager_.get(), dynamic_cast<AssetManager*>(assets_.get()), &currentIndex_, &tables_, this, showConfig_);

    inputManager_->setDependencies(dynamic_cast<AssetManager*>(assets_.get()), soundManager_.get(), configManager_.get(), 
                                   currentIndex_, tables_, showConfig_, exeDir_, screenshotManager_.get(),
                                   windowManager_.get());
    inputManager_->setRuntimeEditor(configEditor_.get());
    inputManager_->registerActions();

    LOG_INFO("App: Initialization complete");
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
    auto* concreteAssets = dynamic_cast<AssetManager*>(assets_.get());
    if (concreteAssets) {
        concreteAssets->clearOldVideoPlayers();
    }
    prevShowConfig_ = inputManager_->isConfigActive();
}

void App::render() {
    if (renderer_ && assets_) {
        const Settings& settings = configManager_->getSettings();
        
        SDL_SetRenderDrawColor(windowManager_->getPlayfieldRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getPlayfieldRenderer());
        
        if (settings.showBackglass) {
            SDL_SetRenderDrawColor(windowManager_->getBackglassRenderer(), 0, 0, 0, 255);
            SDL_RenderClear(windowManager_->getBackglassRenderer());
        }
        
        if (settings.showDMD) {
            SDL_SetRenderDrawColor(windowManager_->getDMDRenderer(), 0, 0, 0, 255);
            SDL_RenderClear(windowManager_->getDMDRenderer());
        }

        guiManager_->newFrame();
        renderer_->render(*assets_);
        if (showConfig_) {
            configEditor_->drawGUI();
        }
        guiManager_->render(windowManager_->getPlayfieldRenderer());

        SDL_RenderPresent(windowManager_->getPlayfieldRenderer());
        if (settings.showBackglass) {
            SDL_RenderPresent(windowManager_->getBackglassRenderer());
        }
        if (settings.showDMD) {
            SDL_RenderPresent(windowManager_->getDMDRenderer());
        }
    }
}

void App::cleanup() {
    LOG_DEBUG("App: Cleaning up");
    auto* concreteAssets = dynamic_cast<AssetManager*>(assets_.get());
    if (concreteAssets) {
        concreteAssets->cleanupVideoPlayers();
    }
    assets_.reset();
    LOG_INFO("App: Cleanup complete");
}