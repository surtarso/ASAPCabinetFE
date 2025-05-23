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
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0) {
            LOG_ERROR("Main: SDL_Init failed: " << SDL_GetError());
            throw std::runtime_error("Main: SDL initialization failed");
        }
        
        // Get system DPI
        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
            LOG_ERROR("Main: Warning: Could not get DPI: " << SDL_GetError());
            ddpi = 96.0f; // Default fallback DPI
        }
        
        // Enable DPI awareness
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

        if (TTF_Init() < 0) {
            LOG_ERROR("Main: TTF_Init failed: " << TTF_GetError());
            SDL_Quit();
            throw std::runtime_error("Main: TTF initialization failed");
        }
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            LOG_ERROR("Main: IMG_Init failed: " << IMG_GetError());
            TTF_Quit();
            SDL_Quit();
            throw std::runtime_error("Main: IMG initialization failed");
        }
        LOG_DEBUG("Main: SDL subsystems initialized");
    }
    ~SDLBootstrap() {
        IMG_Quit();
        TTF_Quit();
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
        SDL_Quit();
        LOG_DEBUG("Main: SDL subsystems cleaned up");
    }
};

int main(int argc, char* argv[]) {
    std::string configPath = "config.ini";
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << ASAPCABINETFE_VERSION_STRING << std::endl;
        std::cout << "Git Hash: " << ASAPCABINETFE_GIT_HASH << std::endl;
        std::cout << "Git Branch: " << ASAPCABINETFE_GIT_BRANCH << std::endl;
        if (std::string(ASAPCABINETFE_GIT_DIRTY) == "+dirty") {
            std::cout << " (Repository has uncommitted changes)" << std::endl;
        }
        return 0;
    }

    SDLBootstrap bootstrap; // Initialize SDL
    App app(configPath);    // App handles config validation and logger initialization
    app.run();
    return 0;
}