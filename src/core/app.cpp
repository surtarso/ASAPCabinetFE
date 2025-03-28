#include "core/app.h"
#include "core/iwindow_manager.h"
#include "core/window_manager.h"
#include "config/settings_manager.h"
#include "sound/sound_manager.h"
#include "utils/sdl_guards.h"
#include "utils/logging.h"
#include "render/irenderer.h"
#include "render/renderer.h"
#include "keybinds/iinput_manager.h"
#include "keybinds/input_manager.h"

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

App::App(const std::string &configPath)
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
      screenshotManager_(nullptr),
      inputManager_(nullptr),
      prevShowConfig_(false)
{
    exeDir_ = getExecutableDir();
    LOG_DEBUG("Config path: " << configPath_);
    LOG_DEBUG("Exe dir set to: " << exeDir_);
}

App::~App()
{
    cleanup();
    LOG_DEBUG("App destructor completed");
}

void App::run()
{
    initializeDependencies();
    while (!inputManager_->shouldQuit())
    {
        handleEvents();
        update();
        render();
    }
}

void App::onConfigSaved()
{
    LOG_DEBUG("Config saved detected, forcing font reload");
    reloadFont();
    LOG_DEBUG("Font reload completed in onConfigSaved");
}

void App::reloadFont()
{
    const Settings& settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_)
    {
        std::cerr << "Failed to reload font: " << TTF_GetError() << std::endl;
    }
    else
    {
        assets_->setFont(font_.get());
        const TableLoader& table = tables_[currentIndex_];
        assets_->tableNameTexture.reset(assets_->renderText(
            assets_->getPrimaryRenderer(), font_.get(), table.tableName,
            settings.fontColor, assets_->tableNameRect));
        int texWidth = 0;
        if (assets_->tableNameTexture)
        {
            SDL_QueryTexture(assets_->tableNameTexture.get(), nullptr, nullptr, &texWidth, nullptr);
        }
        LOG_DEBUG("Font reloaded with size " << settings.fontSize << ", texture width: " << texWidth);
    }
}

std::string App::getExecutableDir()
{
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1)
    {
        LOG_DEBUG("Warning: Couldn't determine executable path, using './'");
        return "./";
    }
    path[count] = '\0';
    std::string fullPath = std::string(path);
    size_t lastSlash = fullPath.find_last_of('/');
    return (lastSlash == std::string::npos) ? "./" : fullPath.substr(0, lastSlash + 1);
}

bool App::isConfigValid()
{
    const Settings &settings = configManager_->getSettings();
    if (settings.vpxExecutableCmd.empty() || !fs::exists(settings.vpxExecutableCmd))
    {
        std::cerr << "Invalid VPX executable path: " << settings.vpxExecutableCmd << std::endl;
        return false;
    }
    if (settings.vpxTablesPath.empty() || !fs::exists(settings.vpxTablesPath))
    {
        std::cerr << "Invalid table path: " << settings.vpxTablesPath << std::endl;
        return false;
    }
    bool hasVpxFiles = false;
    for (const auto &entry : fs::recursive_directory_iterator(settings.vpxTablesPath))
    {
        if (entry.path().extension() == ".vpx")
        {
            hasVpxFiles = true;
            break;
        }
    }
    if (!hasVpxFiles)
    {
        std::cerr << "No .vpx files found in table path or subdirectories: " << settings.vpxTablesPath << std::endl;
        return false;
    }
    return true;
}

void App::runInitialConfig()
{
    SDL_Window *configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                800, 500, SDL_WINDOW_SHOWN);
    SDL_Renderer *configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    IMGUI_CHECKVERSION();
    ImGuiContext *setupContext = ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(configWindow, configRenderer);
    ImGui_ImplSDLRenderer2_Init(configRenderer);

    bool showConfig = true;
    SetupEditor configEditor(configPath_, showConfig, configManager_.get(), &configManager_->getKeybindManager());
    configEditor.setFillParentWindow(true);

    while (true)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            configEditor.handleEvent(event);
            if (event.type == SDL_QUIT)
            {
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

        if (!showConfig && isConfigValid())
            break;
        else if (!showConfig)
        {
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

void App::initializeSDL()
{
    if (!sdlGuard_.success)
    {
        std::cerr << "SDL initialization failed" << std::endl;
        exit(1);
    }
    if (!mixerGuard_.success)
    {
        std::cerr << "SDL_mixer initialization failed" << std::endl;
        exit(1);
    }
    if (!ttfGuard_.success)
    {
        std::cerr << "TTF initialization failed" << std::endl;
        exit(1);
    }
    if (!imgGuard_.flags)
    {
        std::cerr << "IMG initialization failed" << std::endl;
        exit(1);
    }

    if (Mix_Init(MIX_INIT_MP3) != MIX_INIT_MP3)
    {
        std::cerr << "Mix_Init Error: " << Mix_GetError() << std::endl;
        exit(1);
    }

    LOG_DEBUG("SDL initialized successfully with RAII guards");
}

void App::initializeJoysticks()
{
    int numJoysticks = SDL_NumJoysticks();
    LOG_DEBUG("Found " << numJoysticks << " joysticks");
    for (int i = 0; i < numJoysticks; ++i)
    {
        SDL_Joystick *joystick = SDL_JoystickOpen(i);
        if (joystick)
        {
            joysticks_.push_back(joystick);
            LOG_DEBUG("Opened joystick " << i << ": " << SDL_JoystickName(joystick));
        }
        else
        {
            LOG_DEBUG("Failed to open joystick " << i << ": " << SDL_GetError());
        }
    }
}

void App::loadFont()
{
    const Settings &settings = configManager_->getSettings();
    font_.reset(TTF_OpenFont(settings.fontPath.c_str(), settings.fontSize));
    if (!font_)
    {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
}

void App::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(windowManager_->getPrimaryWindow(), windowManager_->getPrimaryRenderer());
    ImGui_ImplSDLRenderer2_Init(windowManager_->getPrimaryRenderer());
}

void App::initializeDependencies()
{
    initializeSDL();
    initializeJoysticks();

    configManager_ = std::make_unique<SettingsManager>(configPath_);
    configManager_->loadConfig();

    if (!isConfigValid())
    {
        LOG_DEBUG("Config invalid, running initial config");
        runInitialConfig();
    }

    windowManager_ = std::make_unique<WindowManager>(configManager_->getSettings());
    initializeImGui();

    soundManager_ = std::make_unique<SoundManager>(exeDir_, configManager_->getSettings());
    soundManager_->loadSounds();

    loadFont();
    tables_ = loadTableList(configManager_->getSettings());
    if (tables_.empty())
    {
        std::cerr << "Edit config.ini, no .vpx files found in " << configManager_->getSettings().vpxTablesPath << std::endl;
        exit(1);
    }

    assets_ = std::make_unique<AssetManager>(windowManager_->getPrimaryRenderer(),
                                             windowManager_->getSecondaryRenderer(), font_.get());
    assets_->setSettingsManager(configManager_.get());
    screenshotManager_ = std::make_unique<ScreenshotManager>(exeDir_, configManager_.get(),
                                                             &configManager_->getKeybindManager(),
                                                             soundManager_.get());
    configEditor_ = std::make_unique<RuntimeEditor>(configPath_, showConfig_, configManager_.get(),
                                                    &configManager_->getKeybindManager(), assets_.get(),
                                                    &currentIndex_, &tables_, this);
    renderer_ = std::make_unique<Renderer>(windowManager_->getPrimaryRenderer(),
                                           windowManager_->getSecondaryRenderer());
    inputManager_ = std::make_unique<InputManager>(&configManager_->getKeybindManager());
    inputManager_->setDependencies(assets_.get(), soundManager_.get(), configManager_.get(),
                                   currentIndex_, tables_, showConfig_, getExecutableDir());
    inputManager_->setRuntimeEditor(configEditor_.get());
    inputManager_->registerActions();

    assets_->loadTableAssets(currentIndex_, tables_);

    LOG_DEBUG("Initialization complete");
}

void App::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        inputManager_->handleEvent(event);
        if (showConfig_)
        {
            configEditor_->handleEvent(event);
        }

        if (event.type == SDL_JOYDEVICEADDED)
        {
            SDL_Joystick *joystick = SDL_JoystickOpen(event.jdevice.which);
            if (joystick)
            {
                joysticks_.push_back(joystick);
                LOG_DEBUG("Joystick connected: " << SDL_JoystickName(joystick));
            }
        }
        else if (event.type == SDL_JOYDEVICEREMOVED)
        {
            for (auto it = joysticks_.begin(); it != joysticks_.end(); ++it)
            {
                if (SDL_JoystickInstanceID(*it) == event.jdevice.which)
                {
                    SDL_JoystickClose(*it);
                    joysticks_.erase(it);
                    LOG_DEBUG("Joystick disconnected: ID " << event.jdevice.which);
                    break;
                }
            }
        }
    }
}

void App::update()
{
    assets_->clearOldVideoPlayers();
    configManager_->applyConfigChanges(windowManager_->getPrimaryWindow(), windowManager_->getSecondaryWindow());
    prevShowConfig_ = inputManager_->isConfigActive();
}

void App::render()
{
    if (renderer_ && assets_)
    {
        SDL_SetRenderDrawColor(windowManager_->getPrimaryRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getPrimaryRenderer());
        SDL_SetRenderDrawColor(windowManager_->getSecondaryRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(windowManager_->getSecondaryRenderer());

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        renderer_->render(*assets_);

        if (showConfig_)
        {
            configEditor_->drawGUI();
        }

        ImGui::Render();
        if (ImGui::GetDrawData())
        {
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), windowManager_->getPrimaryRenderer());
        }

        SDL_RenderPresent(windowManager_->getPrimaryRenderer());
        SDL_RenderPresent(windowManager_->getSecondaryRenderer());
    }
}

void App::cleanup()
{
    LOG_DEBUG("Cleaning up");
    if (assets_)
    {
        VideoContext *tablePlayer = assets_->getTableVideoPlayer();
        if (tablePlayer && tablePlayer->player)
        {
            libvlc_media_player_stop(tablePlayer->player);
            libvlc_media_player_release(tablePlayer->player);
            tablePlayer->player = nullptr;
        }
        VideoContext *backglassPlayer = assets_->getBackglassVideoPlayer();
        if (backglassPlayer && backglassPlayer->player)
        {
            libvlc_media_player_stop(backglassPlayer->player);
            libvlc_media_player_release(backglassPlayer->player);
            backglassPlayer->player = nullptr;
        }
        VideoContext *dmdPlayer = assets_->getDmdVideoPlayer();
        if (dmdPlayer && dmdPlayer->player)
        {
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

    for (auto joystick : joysticks_)
    {
        if (joystick)
            SDL_JoystickClose(joystick);
    }
    joysticks_.clear();

    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    font_.reset();
    LOG_DEBUG("Cleanup complete");
}