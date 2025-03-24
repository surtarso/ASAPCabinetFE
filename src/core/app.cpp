#include "core/app.h"
#include "utils/logging.h"
#include "version.h"
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>

App::App()
    : sdlInit_(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO),
      imgInit_(IMG_INIT_PNG | IMG_INIT_JPG),
      ttfInit_(),
      mixerGuard_(44100, MIX_DEFAULT_FORMAT, 2, 2048),
      primaryWindow_(nullptr, SDL_DestroyWindow),
      primaryRenderer_(nullptr, SDL_DestroyRenderer),
      secondaryWindow_(nullptr, SDL_DestroyWindow),
      secondaryRenderer_(nullptr, SDL_DestroyRenderer),
      font_(nullptr, TTF_CloseFont),
      tableChangeSound_(nullptr, Mix_FreeChunk),
      tableLoadSound_(nullptr, Mix_FreeChunk),
      currentIndex_(0),
      assets_(nullptr, nullptr, nullptr), // Will be initialized later
      screenshotManager_(""),
      configManager_(nullptr), // Initialize as nullptr
      configEditor_(nullptr),  // Initialize as nullptr
      renderer_(nullptr, nullptr),
      showConfig_(false),
      quit_(false) {}

App::~App() {
    cleanup();
}

int App::initialize(int argc, char* argv[]) {
    // Handle command-line arguments
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
        return 0;
    }

    // Initialize configuration
    exeDir_ = getExecutableDir();
    configPath_ = exeDir_ + "config.ini";
    initialize_config(configPath_);

    // Check config validity
    if (!isConfigValid()) {
        std::cout << "Invalid config detected. Launching setup..." << std::endl;
        runInitialConfig();
    }

    // Initialize SDL and subsystems
    initializeSDL();

    // Create windows and renderers
    createWindowsAndRenderers();

    // Initialize ImGui
    initializeImGui();

    // Load resources and initialize managers
    loadResources();

    return 1; // Continue running
}

void App::run() {
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
    if (lastSlash == std::string::npos) {
        return "./";
    }
    return fullPath.substr(0, lastSlash + 1);
}

bool App::isConfigValid() {
    if (VPX_EXECUTABLE_CMD.empty() || !std::filesystem::exists(VPX_EXECUTABLE_CMD)) {
        std::cerr << "Invalid VPX executable path: " << VPX_EXECUTABLE_CMD << std::endl;
        return false;
    }
    if (VPX_TABLES_PATH.empty() || !std::filesystem::exists(VPX_TABLES_PATH)) {
        std::cerr << "Invalid table path: " << VPX_TABLES_PATH << std::endl;
        return false;
    }
    bool hasVpxFiles = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(VPX_TABLES_PATH)) {
        if (entry.path().extension() == ".vpx") {
            hasVpxFiles = true;
            break;
        }
    }
    if (!hasVpxFiles) {
        std::cerr << "No .vpx files found in table path or subdirectories: " << VPX_TABLES_PATH << std::endl;
        return false;
    }
    return true;
}

void App::runInitialConfig() {
    SDL_Window* configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 500, SDL_WINDOW_SHOWN);
    if (!configWindow) {
        LOG_DEBUG("Failed to create config window: " << SDL_GetError());
        exit(1);
    }

    SDL_Renderer* configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!configRenderer) {
        LOG_DEBUG("Failed to create config renderer: " << SDL_GetError());
        SDL_DestroyWindow(configWindow);
        exit(1);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(configWindow, configRenderer);
    ImGui_ImplSDLRenderer2_Init(configRenderer);

    int originalWidth = MAIN_WINDOW_WIDTH;
    int originalHeight = MAIN_WINDOW_HEIGHT;
    MAIN_WINDOW_WIDTH = 800;
    MAIN_WINDOW_HEIGHT = 500;

    bool showConfig = true;
    IniEditor configEditor(configPath_, showConfig, nullptr);
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

        if (!showConfig) {
            initialize_config(configPath_);
            if (isConfigValid()) {
                break;
            } else {
                std::cerr << "Config still invalid. Please fix VPX.ExecutableCmd and VPX.TablesPath." << std::endl;
                showConfig = true;
            }
        }
    }

    MAIN_WINDOW_WIDTH = originalWidth;
    MAIN_WINDOW_HEIGHT = originalHeight;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(configRenderer);
    SDL_DestroyWindow(configWindow);
}

void App::initializeSDL() {
    if (!sdlInit_.success) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (!imgInit_.flags) {
        std::cerr << "IMG_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (!ttfInit_.success) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (!mixerGuard_.success) {
        std::cerr << "Mix_OpenAudio failed: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

void App::createWindowsAndRenderers() {
    primaryWindow_.reset(SDL_CreateWindow("Playfield",
        SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!primaryWindow_) {
        std::cerr << "Failed to create primary window: " << SDL_GetError() << std::endl;
        exit(1);
    }

    primaryRenderer_.reset(SDL_CreateRenderer(primaryWindow_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!primaryRenderer_) {
        std::cerr << "Failed to create primary renderer: " << SDL_GetError() << std::endl;
        exit(1);
    }

    secondaryWindow_.reset(SDL_CreateWindow("Backglass",
        SDL_WINDOWPOS_CENTERED_DISPLAY(SECOND_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        SECOND_WINDOW_WIDTH, SECOND_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS));
    if (!secondaryWindow_) {
        std::cerr << "Failed to create secondary window: " << SDL_GetError() << std::endl;
        exit(1);
    }

    secondaryRenderer_.reset(SDL_CreateRenderer(secondaryWindow_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!secondaryRenderer_) {
        std::cerr << "Failed to create secondary renderer: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

void App::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(primaryWindow_.get(), primaryRenderer_.get());
    ImGui_ImplSDLRenderer2_Init(primaryRenderer_.get());
}

void App::loadResources() {
    font_.reset(TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE));
    if (!font_) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    std::string tableChangeSoundPath = exeDir_ + TABLE_CHANGE_SOUND;
    tableChangeSound_.reset(Mix_LoadWAV(tableChangeSoundPath.c_str()));
    if (!tableChangeSound_) {
        std::cerr << "Mix_LoadWAV Error at " << tableChangeSoundPath << ": " << Mix_GetError() << std::endl;
    }

    std::string tableLoadSoundPath = exeDir_ + TABLE_LOAD_SOUND;
    tableLoadSound_.reset(Mix_LoadWAV(tableLoadSoundPath.c_str()));
    if (!tableLoadSound_) {
        std::cerr << "Mix_LoadWAV Error at " << tableLoadSoundPath << ": " << Mix_GetError() << std::endl;
    }

    tables_ = loadTableList();
    if (tables_.empty()) {
        std::cerr << "Edit config.ini, no .vpx files found in " << VPX_TABLES_PATH << std::endl;
        exit(1);
    }

    // Initialize assets_ now that renderers and font are ready
    assets_ = AssetManager(primaryRenderer_.get(), secondaryRenderer_.get(), font_.get());
    screenshotManager_ = ScreenshotManager(exeDir_);
    
    // Create configManager_ and configEditor_ using std::make_unique
    configManager_ = std::make_unique<ConfigManager>(configPath_, primaryWindow_.get(), primaryRenderer_.get(),
                                                     secondaryWindow_.get(), secondaryRenderer_.get(),
                                                     font_, assets_, currentIndex_, tables_);
    configEditor_ = std::make_unique<IniEditor>(configPath_, showConfig_, configManager_.get());
    
    renderer_ = Renderer(primaryRenderer_.get(), secondaryRenderer_.get());

    assets_.loadTableAssets(currentIndex_, tables_);
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            quit_ = true;
        }

        if (event.type == SDL_KEYDOWN) {
            if (inputManager_.isToggleConfig(event)) {
                showConfig_ = !showConfig_;
                LOG_DEBUG("Toggled showConfig to: " << (showConfig_ ? 1 : 0));
            }

            if (showConfig_) {
                configEditor_->handleEvent(event);
                if (!configEditor_->isCapturingKey()) {
                    continue;
                }
            }

            if (!showConfig_) {
                if (inputManager_.isPreviousTable(event)) {
                    size_t newIndex = (currentIndex_ + tables_.size() - 1) % tables_.size();
                    if (newIndex != currentIndex_) {
                        transitionManager_.startTransition(assets_.getTableVideoPlayer(), assets_.getBackglassVideoPlayer(),
                                                          assets_.getDmdVideoPlayer(), tableChangeSound_.get(),
                                                          primaryRenderer_.get(), secondaryRenderer_.get());
                        currentIndex_ = newIndex;
                    }
                }
                else if (inputManager_.isNextTable(event)) {
                    size_t newIndex = (currentIndex_ + 1) % tables_.size();
                    if (newIndex != currentIndex_) {
                        transitionManager_.startTransition(assets_.getTableVideoPlayer(), assets_.getBackglassVideoPlayer(),
                                                          assets_.getDmdVideoPlayer(), tableChangeSound_.get(),
                                                          primaryRenderer_.get(), secondaryRenderer_.get());
                        currentIndex_ = newIndex;
                    }
                }
                else if (inputManager_.isFastPrevTable(event)) {
                    size_t newIndex = (currentIndex_ + tables_.size() - 10) % tables_.size();
                    if (newIndex != currentIndex_) {
                        transitionManager_.startTransition(assets_.getTableVideoPlayer(), assets_.getBackglassVideoPlayer(),
                                                          assets_.getDmdVideoPlayer(), tableChangeSound_.get(),
                                                          primaryRenderer_.get(), secondaryRenderer_.get());
                        currentIndex_ = newIndex;
                    }
                }
                else if (inputManager_.isFastNextTable(event)) {
                    size_t newIndex = (currentIndex_ + 10) % tables_.size();
                    if (newIndex != currentIndex_) {
                        transitionManager_.startTransition(assets_.getTableVideoPlayer(), assets_.getBackglassVideoPlayer(),
                                                          assets_.getDmdVideoPlayer(), tableChangeSound_.get(),
                                                          primaryRenderer_.get(), secondaryRenderer_.get());
                        currentIndex_ = newIndex;
                    }
                }
                else if (inputManager_.isJumpNextLetter(event)) {
                    char currentLetter = toupper(tables_[currentIndex_].tableName[0]);
                    char nextLetter = currentLetter + 1;
                    bool found = false;
                    size_t newIndex = currentIndex_;
                    for (; nextLetter <= 'Z'; ++nextLetter) {
                        if (letterIndex.find(nextLetter) != letterIndex.end()) {
                            newIndex = letterIndex[nextLetter];
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        for (nextLetter = 'A'; nextLetter < currentLetter; ++nextLetter) {
                            if (letterIndex.find(nextLetter) != letterIndex.end()) {
                                newIndex = letterIndex[nextLetter];
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found && newIndex != currentIndex_) {
                        transitionManager_.startTransition(assets_.getTableVideoPlayer(), assets_.getBackglassVideoPlayer(),
                                                          assets_.getDmdVideoPlayer(), tableChangeSound_.get(),
                                                          primaryRenderer_.get(), secondaryRenderer_.get());
                        currentIndex_ = newIndex;
                    }
                }
                else if (inputManager_.isJumpPrevLetter(event)) {
                    char currentLetter = toupper(tables_[currentIndex_].tableName[0]);
                    char prevLetter = currentLetter - 1;
                    bool found = false;
                    size_t newIndex = currentIndex_;
                    for (; prevLetter >= 'A'; --prevLetter) {
                        if (letterIndex.find(prevLetter) != letterIndex.end()) {
                            newIndex = letterIndex[prevLetter];
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        for (prevLetter = 'Z'; prevLetter > currentLetter; --prevLetter) {
                            if (letterIndex.find(prevLetter) != letterIndex.end()) {
                                newIndex = letterIndex[prevLetter];
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found && newIndex != currentIndex_) {
                        transitionManager_.startTransition(assets_.getTableVideoPlayer(), assets_.getBackglassVideoPlayer(),
                                                          assets_.getDmdVideoPlayer(), tableChangeSound_.get(),
                                                          primaryRenderer_.get(), secondaryRenderer_.get());
                        currentIndex_ = newIndex;
                    }
                }
                else if (inputManager_.isLaunchTable(event)) {
                    if (tableLoadSound_) Mix_PlayChannel(-1, tableLoadSound_.get(), 0);
                    std::string command = VPX_START_ARGS + " " + VPX_EXECUTABLE_CMD + " " + VPX_SUB_CMD + " \"" + tables_[currentIndex_].vpxFile + "\" " + VPX_END_ARGS;
                    LOG_DEBUG("Launching: " << command);
                    int result = std::system(command.c_str());
                    if (result != 0) {
                        std::cerr << "Warning: VPX launch failed with exit code " << result << std::endl;
                    }
                }
                else if (inputManager_.isScreenshotMode(event)) {
                    LOG_DEBUG("Entering screenshot mode for: " << tables_[currentIndex_].vpxFile);
                    screenshotManager_.launchScreenshotMode(tables_[currentIndex_].vpxFile);
                }
                else if (inputManager_.isQuit(event)) {
                    quit_ = true;
                    LOG_DEBUG("Quit triggered via keybind");
                }
            }
        }
    }
}

void App::update() {
    Uint32 now = SDL_GetTicks();
    transitionManager_.updateTransition(now, assets_);
    transitionManager_.loadNewContent([&]() { assets_.loadTableAssets(currentIndex_, tables_); });

    if (!transitionManager_.isTransitionActive()) {
        assets_.clearOldVideoPlayers();
    }

    configManager_->applyConfigChanges();
}

void App::render() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderer_.render(assets_, transitionManager_, showConfig_, *configEditor_);

    ImGui::Render();
}

void App::cleanup() {
    cleanupVideoContext(assets_.getTableVideoPlayer());
    cleanupVideoContext(assets_.getBackglassVideoPlayer());
    cleanupVideoContext(assets_.getDmdVideoPlayer());
    assets_.clearOldVideoPlayers();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}