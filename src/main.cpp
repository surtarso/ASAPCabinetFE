#include "core/app.h"
#include "core/first_run.h"
#include "core/dependency_factory.h"
#include "utils/logging.h"
#include "version.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>

struct SDLBootstrap {
    SDLBootstrap() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0) {
            LOG_DEBUG("SDL_Init failed: " << SDL_GetError());
            throw std::runtime_error("SDL initialization failed");
        }
        
        // Get system DPI
        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
            LOG_DEBUG("Warning: Could not get DPI: " << SDL_GetError());
            ddpi = 96.0f; // Default fallback DPI
        }
        
        // Enable DPI awareness
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

        if (TTF_Init() < 0) {
            LOG_DEBUG("TTF_Init failed: " << TTF_GetError());
            SDL_Quit();
            throw std::runtime_error("TTF initialization failed");
        }
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            LOG_DEBUG("IMG_Init failed: " << IMG_GetError());
            TTF_Quit();
            SDL_Quit();
            throw std::runtime_error("IMG initialization failed");
        }
        LOG_DEBUG("SDL subsystems initialized");
    }
    ~SDLBootstrap() {
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        LOG_DEBUG("SDL subsystems cleaned up");
    }
};

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
        return 0;
    }

    SDLBootstrap bootstrap;
    std::string configPath = "config.ini";
    auto configService = DependencyFactory::createConfigService(configPath);

    if (!configService->isConfigValid()) {
        if (!runInitialConfig(configService.get(), configPath)) {
            LOG_DEBUG("Initial configuration failed or was aborted. Exiting...");
            return 1;
        }
        configService->loadConfig();  // Reload after first-run setup
    }

    App app(configPath);
    app.run();
    return 0;
}
