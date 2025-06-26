/**
 * @file first_run.cpp
 * @brief Implementation of the initial configuration setup for ASAPCabinetFE.
 */

#include "core/first_run.h"
#include "log/logging.h"
#include <SDL2/SDL.h>
#include <memory>

bool runInitialConfig(IConfigService* configService, IKeybindProvider* keybindProvider, const std::string& configPath) {
    //LOG_INFO("Config Path: " + configPath);
    SDL_Window* configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                800, 500, SDL_WINDOW_SHOWN);

    LOG_DEBUG("Running initial config with path: " + configPath);
    if (!configWindow) {
        LOG_ERROR("Failed to create config window: " + std::string(SDL_GetError()));
        return false;
    }

    SDL_Renderer* configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!configRenderer) {
        LOG_ERROR("Failed to create config renderer: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(configWindow);
        return false;
    }

    auto guiManager = std::make_unique<ImGuiManager>(configWindow, configRenderer, configService);
    guiManager->initialize();

    bool showConfig = true;
    ConfigUI configEditor(configService, keybindProvider, 
                          nullptr, nullptr, nullptr, nullptr, showConfig, true);

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            guiManager->processEvent(event);
            configEditor.handleEvent(event);
            if (event.type == SDL_QUIT) {
                LOG_ERROR("Config window closed without saving. Exiting...");
                return false;
            }
        }
        guiManager->newFrame();
        configEditor.drawGUI();
        guiManager->render(configRenderer);
        SDL_RenderPresent(configRenderer);

        if (!showConfig && configService->isConfigValid()) break;
        else if (!showConfig) {
            LOG_DEBUG("Configuration invalid.");
            showConfig = true;
        }
    }

    guiManager.reset();
    SDL_DestroyRenderer(configRenderer);
    SDL_DestroyWindow(configWindow);
    LOG_INFO("Initial config completed");
    return true;
}