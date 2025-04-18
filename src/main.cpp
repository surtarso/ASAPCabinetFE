#include "version.h"
#include "core/app.h"
#include "core/first_run.h"
#include "core/dependency_factory.h"
#include "utils/logging.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>
#include <filesystem>

struct SDLBootstrap {
    std::string configPath;
    SDLBootstrap() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0) {
            LOG_ERROR("SDL_Init failed: " << SDL_GetError());
            throw std::runtime_error("SDL initialization failed");
        }
        
        // Get system DPI
        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
            LOG_ERROR("Warning: Could not get DPI: " << SDL_GetError());
            ddpi = 96.0f; // Default fallback DPI
        }
        
        // Enable DPI awareness
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

        if (TTF_Init() < 0) {
            LOG_ERROR("TTF_Init failed: " << TTF_GetError());
            SDL_Quit();
            throw std::runtime_error("TTF initialization failed");
        }
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            LOG_ERROR("IMG_Init failed: " << IMG_GetError());
            TTF_Quit();
            SDL_Quit();
            throw std::runtime_error("IMG initialization failed");
        }
        LOG_INFO("SDL subsystems initialized");
    }
    ~SDLBootstrap() {
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        LOG_INFO("SDL subsystems cleaned up");
    }
};

int main(int argc, char* argv[]) {
    std::string configPath = "config.ini";
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
        return 0;
    }

    SDLBootstrap bootstrap; // Initialize SDL
    App app(configPath);    // App handles config validation and logger initialization
    app.run();
    return 0;
}