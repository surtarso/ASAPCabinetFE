#include "core/app.h"
#include "utils/logging.h"
#include "config/config_manager.h"
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
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;

App::App(const std::string& configPath) 
    : configPath_(configPath),
      primaryWindow_(nullptr, SDL_DestroyWindow),
      secondaryWindow_(nullptr, SDL_DestroyWindow),
      primaryRenderer_(nullptr, SDL_DestroyRenderer),
      secondaryRenderer_(nullptr, SDL_DestroyRenderer),
      font_(nullptr, TTF_CloseFont),
      tableChangeSound_(nullptr, Mix_FreeChunk),
      tableLoadSound_(nullptr, Mix_FreeChunk),
      configManager_(nullptr),
      configEditor_(nullptr),
      inputManager_(nullptr),
      renderer_(nullptr),
      assets_(nullptr),
      screenshotManager_(nullptr)  
{
    exeDir_ = getExecutableDir();
    LOG_DEBUG("Config path: " << configPath_);
    LOG_DEBUG("Exe dir set to: " << exeDir_);
}

App::~App() {
    cleanup();
    LOG_DEBUG("App destructor completed");
}

int App::initialize() {
    LOG_DEBUG("Initializing SDL");
    initializeSDL();
    configManager_ = std::make_unique<ConfigManager>(configPath_);
    configManager_->loadConfig();
    if (!isConfigValid()) {
        LOG_DEBUG("Config invalid, running initial config");
        runInitialConfig();
    }
    createWindowsAndRenderers();
    initializeImGui();
    loadResources();
    initializeActionHandlers();
    LOG_DEBUG("Initialization complete");
    return 0;
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
    if (lastSlash == std::string::npos) {
        return "./";
    }
    return fullPath.substr(0, lastSlash + 1);
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
    SDL_Window* configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                800, 500, SDL_WINDOW_SHOWN);
    SDL_Renderer* configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    IMGUI_CHECKVERSION();
    ImGuiContext* setupContext = ImGui::CreateContext();
    LOG_DEBUG("Created ImGui context for setup: " << (void*)setupContext);
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(configWindow, configRenderer);
    ImGui_ImplSDLRenderer2_Init(configRenderer);
    bool showConfig = true;
    IniEditor configEditor(configPath_, showConfig, configManager_.get(), &configManager_->getKeybindManager());
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
        if (!showConfig && isConfigValid()) {
            break;
        } else if (!showConfig) {
            std::cerr << "Config still invalid. Please fix VPX.ExecutableCmd and VPX.TablesPath." << std::endl;
            showConfig = true;
        }
    }
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(setupContext);
    LOG_DEBUG("Destroyed ImGui context for setup: " << (void*)setupContext);
    SDL_DestroyRenderer(configRenderer);
    SDL_DestroyWindow(configWindow);
}

void App::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        exit(1);
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
        exit(1);
    }
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
}

void App::createWindowsAndRenderers() {
    const Settings& settings = configManager_->getSettings();
    primaryWindow_.reset(SDL_CreateWindow("Playfield",
                                          SDL_WINDOWPOS_CENTERED_DISPLAY(settings.mainWindowMonitor),
                                          SDL_WINDOWPOS_CENTERED,
                                          settings.mainWindowWidth,
                                          settings.mainWindowHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!primaryWindow_) {
        std::cerr << "Failed to create primary window: " << SDL_GetError() << std::endl;
        exit(1);
    }
    primaryRenderer_.reset(SDL_CreateRenderer(primaryWindow_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!primaryRenderer_) {
        std::cerr << "Failed to create primary renderer: " << SDL_GetError() << std::endl;
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(primaryRenderer_.get(), SDL_BLENDMODE_BLEND);

    secondaryWindow_.reset(SDL_CreateWindow("Backglass",
                                            SDL_WINDOWPOS_CENTERED_DISPLAY(settings.secondWindowMonitor),
                                            SDL_WINDOWPOS_CENTERED,
                                            settings.secondWindowWidth,
                                            settings.secondWindowHeight,
                                            SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!secondaryWindow_) {
        std::cerr << "Failed to create secondary window: " << SDL_GetError() << std::endl;
        exit(1);
    }
    secondaryRenderer_.reset(SDL_CreateRenderer(secondaryWindow_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!secondaryRenderer_) {
        std::cerr << "Failed to create secondary renderer: " << SDL_GetError() << std::endl;
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(secondaryRenderer_.get(), SDL_BLENDMODE_BLEND);
}

void App::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGuiContext* mainContext = ImGui::CreateContext();
    LOG_DEBUG("Created ImGui context for main app: " << (void*)mainContext);
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(primaryWindow_.get(), primaryRenderer_.get());
    ImGui_ImplSDLRenderer2_Init(primaryRenderer_.get());
}

void App::loadResources() {
    LOG_DEBUG("Loading resources");
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        LOG_DEBUG("Current working directory: " << cwd);
    } else {
        LOG_DEBUG("Failed to get CWD: " << strerror(errno));
    }

    const Settings& settings = configManager_->getSettings();
    std::string trimmedChange = settings.tableChangeSound;
    std::string trimmedLoad = settings.tableLoadSound;
    trimmedChange.erase(std::remove_if(trimmedChange.begin(), trimmedChange.end(), isspace), trimmedChange.end());
    trimmedLoad.erase(std::remove_if(trimmedLoad.begin(), trimmedLoad.end(), isspace), trimmedLoad.end());
    
    std::ostringstream changeHex, loadHex;
    for (char c : settings.tableChangeSound) changeHex << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c << " ";
    for (char c : settings.tableLoadSound) loadHex << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c << " ";
    LOG_DEBUG("Raw TableChangeSound bytes: " << changeHex.str());
    LOG_DEBUG("Raw TableLoadSound bytes: " << loadHex.str());
    LOG_DEBUG("Trimmed TableChangeSound: '" << trimmedChange << "'");
    LOG_DEBUG("Trimmed TableLoadSound: '" << trimmedLoad << "'");

    std::string tableLoadSoundPath = exeDir_ + trimmedLoad;
    LOG_DEBUG("Loading table load sound from: " << tableLoadSoundPath);
    std::ifstream testFile(tableLoadSoundPath, std::ios::binary);
    if (!testFile.good()) {
        LOG_DEBUG("Error: Cannot open table load sound file for reading (errno: " << errno << " - " << strerror(errno) << ")");
    } else {
        LOG_DEBUG("Table load sound file exists and is readable");
        testFile.close();
    }
    tableLoadSound_.reset(Mix_LoadWAV(tableLoadSoundPath.c_str()));
    if (!tableLoadSound_) {
        std::cerr << "Mix_LoadWAV Error at " << tableLoadSoundPath << ": " << Mix_GetError() << std::endl;
    } else {
        LOG_DEBUG("Table load sound loaded successfully");
    }

    std::string tableChangeSoundPath = exeDir_ + trimmedChange;
    LOG_DEBUG("Loading table change sound from: " << tableChangeSoundPath);
    tableChangeSound_.reset(Mix_LoadWAV(tableChangeSoundPath.c_str()));
    if (!tableChangeSound_) {
        std::cerr << "Mix_LoadWAV Error at " << tableChangeSoundPath << ": " << Mix_GetError() << std::endl;
    } else {
        LOG_DEBUG("Table change sound loaded successfully");
    }

    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    tables_ = loadTableList(settings);
    if (tables_.empty()) {
        std::cerr << "Edit config.ini, no .vpx files found in " << settings.vpxTablesPath << std::endl;
        exit(1);
    }

    // Populate letterIndex for JumpNextLetter/JumpPrevLetter
    for (size_t i = 0; i < tables_.size(); ++i) {
        if (!tables_[i].tableName.empty()) {
            char firstChar = tables_[i].tableName[0];
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = i;
            }
        }
    }

    assets_ = std::make_unique<AssetManager>(primaryRenderer_.get(), secondaryRenderer_.get(), font_.get());
    assets_->setConfigManager(configManager_.get());
    inputManager_ = std::make_unique<InputManager>(configManager_->getKeybindManager()); // Moved up
    screenshotManager_ = std::make_unique<ScreenshotManager>(exeDir_, configManager_.get(), inputManager_.get()); // Now uses initialized inputManager_
    configEditor_ = std::make_unique<IniEditor>(configPath_, showConfig_, configManager_.get(),
                                                &configManager_->getKeybindManager(), assets_.get(),
                                                &currentIndex_, &tables_);
    renderer_ = std::make_unique<Renderer>(primaryRenderer_.get(), secondaryRenderer_.get());

    LOG_DEBUG("Loading initial table assets");
    assets_->loadTableAssets(currentIndex_, tables_);
    LOG_DEBUG("Resources loaded");
}

void App::initializeActionHandlers() {
    actionHandlers_["PreviousTable"] = [this]() {
        LOG_DEBUG("Previous table triggered");
        size_t newIndex = (currentIndex_ + tables_.size() - 1) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
        }
    };

    actionHandlers_["NextTable"] = [this]() {
        LOG_DEBUG("Next table triggered");
        size_t newIndex = (currentIndex_ + 1) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
        }
    };

    actionHandlers_["FastPrevTable"] = [this]() {
        LOG_DEBUG("Fast previous table triggered");
        size_t newIndex = (currentIndex_ + tables_.size() - 10) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
        }
    };

    actionHandlers_["FastNextTable"] = [this]() {
        LOG_DEBUG("Fast next table triggered");
        size_t newIndex = (currentIndex_ + 10) % tables_.size();
        if (newIndex != currentIndex_) {
            assets_->loadTableAssets(newIndex, tables_);
            currentIndex_ = newIndex;
            if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
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
                if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
            }
        } else {
            // Wrap to last entry (e.g., Z or 9)
            auto lastIt = std::prev(letterIndex.end());
            size_t newIndex = lastIt->second;
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
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
                if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
            }
        } else {
            // Wrap to first entry (e.g., 0 or A)
            size_t newIndex = letterIndex.begin()->second;
            if (newIndex != currentIndex_) {
                assets_->loadTableAssets(newIndex, tables_);
                currentIndex_ = newIndex;
                if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
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
                if (tableChangeSound_) Mix_PlayChannel(-1, tableChangeSound_.get(), 0);
            }
        }
    };

    actionHandlers_["LaunchTable"] = [this]() {
        LOG_DEBUG("Launch table triggered");
        if (tableLoadSound_) {
            LOG_DEBUG("Playing table load sound");
            Mix_PlayChannel(-1, tableLoadSound_.get(), 0);
        }
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
        if (tableLoadSound_) {
            LOG_DEBUG("Playing table load sound for screenshot mode");
            Mix_PlayChannel(-1, tableLoadSound_.get(), 0);
        }
        screenshotManager_->launchScreenshotMode(tables_[currentIndex_].vpxFile);
    };

    actionHandlers_["ToggleConfig"] = [this]() {
        showConfig_ = !showConfig_;
        LOG_DEBUG("Toggled showConfig to: " << (showConfig_ ? 1 : 0));
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("Quit triggered");
        quit_ = true;
    };

    actionHandlers_["ConfigSave"] = [this]() {
        if (showConfig_) {
            LOG_DEBUG("Config save triggered");
            Settings& mutableSettings = const_cast<Settings&>(configManager_->getSettings());
            mutableSettings = configEditor_->tempSettings_;
            configManager_->saveConfig();
            configManager_->notifyConfigChanged(*assets_, currentIndex_, tables_);
            showConfig_ = false;
        }
    };

    actionHandlers_["ConfigClose"] = [this]() {
        if (showConfig_) {
            LOG_DEBUG("Config close triggered");
            showConfig_ = false;
        }
    };
}

void App::handleEvents() {
    LOG_DEBUG("Handling events");
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            quit_ = true;
            LOG_DEBUG("SDL_QUIT received");
            return;
        }
        if (event.type == SDL_KEYDOWN) {
            LOG_DEBUG("Key pressed: " << SDL_GetKeyName(event.key.keysym.sym));
            bool eventConsumed = false;
            if (showConfig_) {
                configEditor_->handleEvent(event);
                // If the config GUI is capturing a key, it consumes the event
                if (configEditor_->isCapturingKey()) {
                    eventConsumed = true;
                }
                // Allow ConfigSave and ConfigClose to be processed by App even when config GUI is open
                if (inputManager_->isAction(event, "ConfigSave")) {
                    auto it = actionHandlers_.find("ConfigSave");
                    if (it != actionHandlers_.end()) {
                        it->second();
                    }
                    eventConsumed = true;
                } else if (inputManager_->isAction(event, "ConfigClose")) {
                    auto it = actionHandlers_.find("ConfigClose");
                    if (it != actionHandlers_.end()) {
                        it->second();
                    }
                    eventConsumed = true;
                } else {
                    // If the config GUI is open but not capturing a key, assume it consumes the event
                    // to prevent other actions (like Quit) from being processed
                    eventConsumed = true;
                }
            }
            if (!eventConsumed) {
                for (const auto& action : configManager_->getKeybindManager().getActions()) {
                    if (inputManager_->isAction(event, action)) {
                        auto it = actionHandlers_.find(action);
                        if (it != actionHandlers_.end()) {
                            it->second();
                        }
                    }
                }
            }
        }
    }
    LOG_DEBUG("Events handled");
}

void App::update() {
    LOG_DEBUG("Updating");
    assets_->clearOldVideoPlayers();
    configManager_->applyConfigChanges(primaryWindow_.get(), secondaryWindow_.get());
    LOG_DEBUG("Update complete");
}

void App::render() {
    LOG_DEBUG("Rendering with showConfig_: " << (showConfig_ ? 1 : 0));
    if (renderer_ && assets_) {
        SDL_SetRenderDrawColor(primaryRenderer_.get(), 0, 0, 0, 255);
        SDL_RenderClear(primaryRenderer_.get());
        SDL_SetRenderDrawColor(secondaryRenderer_.get(), 0, 0, 0, 255);
        SDL_RenderClear(secondaryRenderer_.get());

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        renderer_->render(*assets_, showConfig_, *configEditor_);

        if (showConfig_) {
            LOG_DEBUG("Calling configEditor_->drawGUI() from App");
            configEditor_->drawGUI();
        }

        ImGui::Render();
        if (ImGui::GetDrawData()) {
            LOG_DEBUG("ImGui has draw data in App::render, CmdListsCount: " << ImGui::GetDrawData()->CmdListsCount);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), primaryRenderer_.get());
        } else {
            LOG_DEBUG("No ImGui draw data in App::render");
        }

        SDL_RenderPresent(primaryRenderer_.get());
        SDL_RenderPresent(secondaryRenderer_.get());
    }
    LOG_DEBUG("Render complete");
}

void App::run() {
    LOG_DEBUG("Entering main loop");
    while (!quit_) {
        handleEvents();
        update();
        render();
    }
    LOG_DEBUG("Exiting main loop");
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

    if (ImGui::GetCurrentContext()) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    font_.reset();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    LOG_DEBUG("Cleanup complete");
}