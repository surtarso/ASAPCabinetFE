#include "core/first_run.h"
#include "utils/logging.h"
#include <iostream>

bool runInitialConfig(IConfigService* configService, const std::string& configPath) {
    LOG_INFO("Config Path: " << configPath);
    SDL_Window* configWindow = SDL_CreateWindow("ASAPCabinetFE Setup",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                800, 500, SDL_WINDOW_SHOWN);
    
        LOG_DEBUG("Running initial config with path: " << configPath);
        if (!configWindow) {
            LOG_ERROR("Failed to create config window: " << SDL_GetError());
        return false;
    }

    SDL_Renderer* configRenderer = SDL_CreateRenderer(configWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!configRenderer) {
        LOG_ERROR("Failed to create config renderer: " << SDL_GetError());
        SDL_DestroyWindow(configWindow);
        return false;
    }

    auto guiManager = std::make_unique<GuiManager>(configWindow, configRenderer, configService);
    guiManager->initialize();

    bool showConfig = true;
    ConfigUI configEditor(configService, &configService->getKeybindManager(), 
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
            LOG_ERROR("Configuration invalid. Please fix VPX.ExecutableCmd and VPX.TablesPath.");
            showConfig = true;
        }
    }

    guiManager.reset();
    SDL_DestroyRenderer(configRenderer);
    SDL_DestroyWindow(configWindow);
    LOG_INFO("Initial config completed");
    return true;
}
