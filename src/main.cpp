#include "core/app.h"
#include "version.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>

struct SDLBootstrap {
    SDLBootstrap() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            exit(1);
        }
        if (TTF_Init() < 0) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
            SDL_Quit();
            exit(1);
        }
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            exit(1);
        }
        std::cout << "SDL subsystems initialized\n";
    }
    ~SDLBootstrap() {
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        std::cout << "SDL subsystems cleaned up\n";
    }
};

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
        return 0;
    }

    SDLBootstrap bootstrap;  // RAII for SDL/TTF/IMG init and cleanup
    App app("config.ini");
    app.run();
    return 0;
}