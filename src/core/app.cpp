#include "core/app.h"
#include "core/iwindow_manager.h"
#include "core/window_manager.h"
#include "config/settings_manager.h"
#include "sound/sound_manager.h"
#include "utils/sdl_guards.h"
#include "utils/logging.h"

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "version.h"
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;

App::App(const std::string& configPath) 
    : configPath_(configPath),
      sdlGuard_(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK),
      mixerGuard_(44100, MIX_DEFAULT_FORMAT, 2, 2048),
      ttfGuard_(),
      imgGuard_(IMG_INIT_PNG | IMG_INIT_JPG),
      font_(nullptr, TTF_CloseFont),
      soundManager_(nullptr),
      configManager_(nullptr),
      configEditor_(nullptr),
      renderer_(nullptr),
      assets_(nullptr),
      screenshotManager_(nullptr) {
    exeDir_ = getExecutableDir();
    LOG_DEBUG("Config path: " << configPath_);
    LOG_DEBUG("Exe dir set to: " << exeDir_);
}

App::~App() {
    cleanup();
    LOG_DEBUG("App destructor completed");
}

void App::run() {
    initializeDependencies();
    while (!quit_) {
        handleEvents();
        update();
        render();
    }
}

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

bool App::isConfigValid() {
    const Settings& settings = configManager_->getSettings();
    if (settings.vpxExecutableCmd.empty() || !fs::exists(settings.vpxExecutableCmd)) {
        std::cerr << "Invalid VPX executable path: " << settings.vpxExecutableCmd << std::endl;
        return false;
    }
    if (settings.vpxTablesPath.empty() || !fs::exists(settings.vpxTablesPath)) {
        std::cerr << "Invalid table path: " << settings.vpxTablesPath << std::endl;
        return false;
    }
    bool hasVpxFiles = false;
    for (const auto& entry : fs::recursive_directory_iterator(settings.vpxTablesPath)) {
        if (entry.path().extension() == ".vpx") {
            hasVpxFiles = true;
            break;
        }
    }
    if (!hasVpxFiles) {
        std::cerr << "No .vpx files found in table path or subdirectories: " << settings.vpxTablesPath << std::endl;
        return false;
    }
    return true;
}

void App::runInitialConfig() {
    // Temporary window for initial setup
    SDL_Window* configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                800, 500, SDL_WINDOW_SHOWN);
    SDL_Renderer* configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    IMGUI_CHECKVERSION();
    ImGuiContext* setupContext = ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(configWindow, configRenderer);
    ImGui_ImplSDLRenderer2_Init(configRenderer);

    bool showConfig = true;
    SetupEditor configEditor(configPath_, showConfig, configManager_.get(), &configManager_->getKeybindManager());
    configEditor.setFillParentWindow(true);

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            configEditor.handleEvent(event);
            if (event.type == SDL_QUIT) {
                std::cerr << "Config window closed without saving. Exiting..." << std::endl;
                exit(1);
            }
        }
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        configEditor.drawGUI();
        ImGui::Render();
        SDL_SetRenderDrawColor(configRenderer, 0, 0, 0, 255);
        SDL_RenderClear(configRenderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), configRenderer);
        SDL_RenderPresent(configRenderer);

        if (!showConfig && isConfigValid()) break;
        else if (!showConfig) {
            std::cerr << "Configuration invalid. Please fix VPX.ExecutableCmd and VPX.TablesPath." << std::endl;
            showConfig = true;
        }
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(setupContext);
    SDL_DestroyRenderer(configRenderer);
    SDL_DestroyWindow(configWindow);
}

void App::initializeSDL() {
    // Check if initialization succeeded using guard status
    if (!sdlGuard_.success) {
        std::cerr << "SDL initialization failed" << std::endl;
        exit(1);
    }
    if (!mixerGuard_.success) {
        std::cerr << "SDL_mixer initialization failed" << std::endl;
        exit(1);
    }
    if (!ttfGuard_.success) {
        std::cerr << "TTF initialization failed" << std::endl;
        exit(1);
    }
    if (!imgGuard_.flags) {
        std::cerr << "IMG initialization failed" << std::endl;
        exit(1);
    }
    
    // Mixer-specific init that isn't covered by MixerGuard
    if (Mix_Init(MIX_INIT_MP3) != MIX_INIT_MP3) {
        std::cerr << "Mix_Init Error: " << Mix_GetError() << std::endl;
        exit(1);
    }
    
    LOG_DEBUG("SDL initialized successfully with RAII guards");
}

void App::initializeJoysticks() {
    int numJoysticks = SDL_NumJoysticks();
    LOG_DEBUG("Found " << numJoysticks << " joysticks");
    for (int i = 0; i < numJoysticks; ++i) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (joystick) {
            joysticks_.push_back(joystick);
            LOG_DEBUG("Opened joystick " << i << ": " << SDL_JoystickName(joystick));
        } else {
            LOG_DEBUG("Failed to open joystick " << i << ": " << SDL_GetError());
        }
    }
}

void App::loadFont() {
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
}

void App::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(windowManager_->getPrimaryWindow(), windowManager_->getPrimaryRenderer());
    ImGui_ImplSDLRenderer2_Init(windowManager_->getPrimaryRenderer());
}

void App::initializeDependencies() {
    initializeSDL();
    initializeJoysticks();

    configManager_ = std::make_unique<SettingsManager>(configPath_);
    configManager_->loadConfig();

    if (!isConfigValid()) {
        LOG_DEBUG("Config invalid, running initial config");
        runInitialConfig();
    }

    windowManager_ = std::make_unique<WindowManager>(configManager_->getSettings());
    initializeImGui();

    soundManager_ = std::make_unique<SoundManager>(exeDir_, configManager_->getSettings());
    soundManager_->loadSounds();

    loadFont();
    tables_ = loadTableList(configManager_->getSettings());
    if (tables_.empty()) {
        std::cerr << "Edit config.ini, no .vpx files found in " << configManager_->getSettings().vpxTablesPath << std::endl;
        exit(1);
    }

    for (size_t i = 0; i < tables_.size(); ++i) {
        if (!tables_[i].tableName.empty()) {
            char firstChar = tables_[i].tableName[0];
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = i;
            }
        }
    }

    assets_ = std::make_unique<AssetManager>(windowManager_->getPrimaryRenderer(), 
                                             windowManager_->getSecondaryRenderer(), font_.get());
    assets_->setSettingsManager(configManager_.get());
    screenshotManager_ = std::make_unique<ScreenshotManager>(exeDir_, configManager_.get(), 
                                                             &configManager_->getKeybindManager(), 
                                                             soundManager_.get());
    configEditor_ = std::make_unique<RuntimeEditor>(configPath_, showConfig_, configManager_.get(),
                                                    &configManager_->getKeybindManager(), assets_.get(),
                                                    &currentIndex_, &tables_); // Fixed: Replaced ¤tIndex_ with currentIndex_
    renderer_ = std::make_unique<Renderer>(windowManager_->getPrimaryRenderer(), 
                                           windowManager_->getSecondaryRenderer());

    assets_->loadTableAssets(currentIndex_, tables_);
    initializeActionHandlers();

    LOG_DEBUG("Initialization complete");
}

void App::initializeActionHandlers() {
    actionHandlers_["PreviousTable"] = [this]() {
        LOG_DEBUG("Previous table triggered");
        size_t newIndex = (currentIndex_ + tables_.size() - 1) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            soundManager_->playSound("scroll_prev");
        }
    };

    actionHandlers_["NextTable"] = [this]() {
        LOG_DEBUG("Next table triggered");
        size_t newIndex = (currentIndex_ + 1) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            soundManager_->playSound("scroll_next");
        }
    };

    actionHandlers_["FastPrevTable"] = [this]() {
        LOG_DEBUG("Fast previous table triggered");
        size_t newIndex = (currentIndex_ + tables_.size() - 10) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            soundManager_->playSound("scroll_fast_prev");
        }
    };

    actionHandlers_["FastNextTable"] = [this]() {
        LOG_DEBUG("Fast next table triggered");
        size_t newIndex = (currentIndex_ + 10) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            soundManager_->playSound("scroll_fast_next");
        }
    };

    actionHandlers_["JumpPrevLetter"] = [this]() {
        LOG_DEBUG("Jump previous letter triggered");
        char currentChar = tables_[currentIndex_].tableName[0];
        char key = std::isalpha(currentChar) ? std::toupper(currentChar) : currentChar;
        auto it = letterIndex.find(key);
        if (it != letterIndex.begin()) {
            auto prevIt = std::prev(it);
            size_t newIndex = prevIt->second;
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                soundManager_->playSound("scroll_jump_prev");
            }
        } else {
            auto lastIt = std::prev(letterIndex.end());
            size_t newIndex = lastIt->second;
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                soundManager_->playSound("scroll_jump_prev");
            }
        }
    };

    actionHandlers_["JumpNextLetter"] = [this]() {
        LOG_DEBUG("Jump next letter triggered");
        char currentChar = tables_[currentIndex_].tableName[0];
        char key = std::isalpha(currentChar) ? std::toupper(currentChar) : currentChar;
        auto it = letterIndex.find(key);
        if (it != letterIndex.end() && std::next(it) != letterIndex.end()) {
            size_t newIndex = std::next(it)->second;
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                soundManager_->playSound("scroll_jump_next");
            }
        } else {
            size_t newIndex = letterIndex.begin()->second;
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                soundManager_->playSound("scroll_jump_next");
            }
        }
    };

    actionHandlers_["RandomTable"] = [this]() {
        LOG_DEBUG("Random table triggered");
        if (!tables_.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, tables_.size() - 1);
            size_t newIndex = dist(gen);
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                soundManager_->playSound("scroll_random");
            }
        }
    };

    actionHandlers_["LaunchTable"] = [this]() {
        LOG_DEBUG("Launch table triggered");
        soundManager_->playSound("launch_table");
        const Settings& settings = configManager_->getSettings();
        std::string command = settings.vpxStartArgs + " " + settings.vpxExecutableCmd + " " +
                              settings.vpxSubCmd + " \"" + tables_[currentIndex_].vpxFile + "\" " +
                              settings.vpxEndArgs;
        LOG_DEBUG("Launching: " << command);
        int result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "Warning: VPX launch failed with exit code " << result << std::endl;
        }
    };

    actionHandlers_["ScreenshotMode"] = [this]() {
        LOG_DEBUG("Screenshot mode triggered");
        soundManager_->playSound("launch_screenshot");
        screenshotManager_->launchScreenshotMode(tables_[currentIndex_].vpxFile);
    };

    actionHandlers_["ToggleConfig"] = [this]() {
        LOG_DEBUG("ToggleConfig action triggered");
        soundManager_->playSound("config_toggle");
        showConfig_ = !showConfig_;
        LOG_DEBUG("Toggled showConfig to: " << (showConfig_ ? 1 : 0));
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("Quit triggered");
        soundManager_->playSound("quit");
        quit_ = true;
    };

    actionHandlers_["ConfigSave"] = [this]() {
        if (showConfig_) {
            LOG_DEBUG("Config save triggered");
            soundManager_->playSound("config_save");
            Settings& mutableSettings = const_cast<Settings&>(configManager_->getSettings());
            mutableSettings = configEditor_->tempSettings_;
            configManager_->saveConfig();
            // Reload font and assets...
            const Settings& newSettings = configManager_->getSettings();
            font_.reset(TTF_OpenFont(newSettings.fontPath.c_str(), newSettings.fontSize));
            if (!font_) {
                std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
                LOG_DEBUG("Font load failed");
            } else {
                LOG_DEBUG("Font reloaded with size " << newSettings.fontSize);
            }
            assets_->setFont(font_.get());
            configManager_->notifyConfigChanged(*assets_, currentIndex_, tables_);
            showConfig_ = false;
        }
    };

    actionHandlers_["ConfigClose"] = [this]() {
        if (showConfig_) {
            LOG_DEBUG("Config close triggered");
            soundManager_->playSound("config_close");
            showConfig_ = false;
        }
    };
}

void App::handleConfigEvents(const SDL_Event& event) {
    configEditor_->handleEvent(event);
    if (configEditor_->isCapturingKey()) return;

    const auto& keybindManager = configManager_->getKeybindManager();
    if (event.type == SDL_KEYDOWN) {
        SDL_KeyboardEvent keyEvent = event.key;
        if (keybindManager.isAction(keyEvent, "ConfigSave")) {
            actionHandlers_["ConfigSave"]();
        } else if (keybindManager.isAction(keyEvent, "ConfigClose")) {
            actionHandlers_["ConfigClose"]();
        }
    } else if (event.type == SDL_JOYBUTTONDOWN) {
        if (keybindManager.isJoystickAction(event.jbutton, "ConfigSave")) {
            actionHandlers_["ConfigSave"]();
        } else if (keybindManager.isJoystickAction(event.jbutton, "ConfigClose")) {
            actionHandlers_["ConfigClose"]();
        }
    } else if (event.type == SDL_JOYHATMOTION) {
        if (keybindManager.isJoystickHatAction(event.jhat, "ConfigSave")) {
            actionHandlers_["ConfigSave"]();
        } else if (keybindManager.isJoystickHatAction(event.jhat, "ConfigClose")) {
            actionHandlers_["ConfigClose"]();
        }
    } else if (event.type == SDL_JOYAXISMOTION) {
        if (keybindManager.isJoystickAxisAction(event.jaxis, "ConfigSave")) {
            actionHandlers_["ConfigSave"]();
        } else if (keybindManager.isJoystickAxisAction(event.jaxis, "ConfigClose")) {
            actionHandlers_["ConfigClose"]();
        }
    }
}

void App::handleRegularEvents(const SDL_Event& event) {
    const auto& keybindManager = configManager_->getKeybindManager();
    for (const auto& action : keybindManager.getActions()) {
        if (event.type == SDL_KEYDOWN) {
            SDL_KeyboardEvent keyEvent = event.key;
            if (keybindManager.isAction(keyEvent, action)) {
                auto it = actionHandlers_.find(action);
                if (it != actionHandlers_.end()) it->second();
            }
        } else if (event.type == SDL_JOYBUTTONDOWN && keybindManager.isJoystickAction(event.jbutton, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
        } else if (event.type == SDL_JOYHATMOTION && keybindManager.isJoystickHatAction(event.jhat, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
        } else if (event.type == SDL_JOYAXISMOTION && keybindManager.isJoystickAxisAction(event.jaxis, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
        }
    }
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            quit_ = true;
            LOG_DEBUG("SDL_QUIT received");
            return;
        }

        if (event.type == SDL_JOYDEVICEADDED) {
            SDL_Joystick* joystick = SDL_JoystickOpen(event.jdevice.which);
            if (joystick) {
                joysticks_.push_back(joystick);
                LOG_DEBUG("Joystick connected: " << SDL_JoystickName(joystick));
            }
        } else if (event.type == SDL_JOYDEVICEREMOVED) {
            for (auto it = joysticks_.begin(); it != joysticks_.end(); ++it) {
                if (SDL_JoystickInstanceID(*it) == event.jdevice.which) {
                    SDL_JoystickClose(*it);
                    joysticks_.erase(it);
                    LOG_DEBUG("Joystick disconnected: ID " << event.jdevice.which);
                    break;
                }
            }
        }

        // Always handle ToggleConfig
        const auto& keybindManager = configManager_->getKeybindManager();
        if (event.type == SDL_KEYDOWN) {
            SDL_KeyboardEvent keyEvent = event.key;
            if (keybindManager.isAction(keyEvent, "ToggleConfig")) {
                actionHandlers_["ToggleConfig"]();
                continue; // Skip further processing for this event
            }
        } else if (event.type == SDL_JOYBUTTONDOWN) {
            if (keybindManager.isJoystickAction(event.jbutton, "ToggleConfig")) {
                actionHandlers_["ToggleConfig"]();
                continue;
            }
        } else if (event.type == SDL_JOYHATMOTION) {
            if (keybindManager.isJoystickHatAction(event.jhat, "ToggleConfig")) {
                actionHandlers_["ToggleConfig"]();
                continue;
            }
        } else if (event.type == SDL_JOYAXISMOTION) {
            if (keybindManager.isJoystickAxisAction(event.jaxis, "ToggleConfig")) {
                actionHandlers_["ToggleConfig"]();
                continue;
            }
        }

        if (showConfig_) {
            handleConfigEvents(event);
        } else {
            handleRegularEvents(event);
        }
    }
}

void App::update() {
    // Update game state
    assets_->clearOldVideoPlayers();
    configManager_->applyConfigChanges(windowManager_->getPrimaryWindow(), windowManager_->getSecondaryWindow());
}

void App::render() {
    // Render the frame
    if (renderer_ && assets_) {
        SDL_SetRenderDrawColor(windowManager_->getPrimaryRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getPrimaryRenderer());
        SDL_SetRenderDrawColor(windowManager_->getSecondaryRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getSecondaryRenderer());

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        renderer_->render(*assets_, showConfig_, *configEditor_);

        if (showConfig_) {
            configEditor_->drawGUI();
        }

        ImGui::Render();
        if (ImGui::GetDrawData()) {
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), windowManager_->getPrimaryRenderer());
        }

        SDL_RenderPresent(windowManager_->getPrimaryRenderer());
        SDL_RenderPresent(windowManager_->getSecondaryRenderer());
    }
}

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

    for (auto joystick : joysticks_) {
        if (joystick) SDL_JoystickClose(joystick);
    }
    joysticks_.clear();

    if (ImGui::GetCurrentContext()) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    font_.reset();
    // No manual SDL cleanup needed - guards handle SDL_Quit, Mix_Quit, TTF_Quit, IMG_Quit
    LOG_DEBUG("Cleanup complete");
}