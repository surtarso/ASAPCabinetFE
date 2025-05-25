/**
 * @file main.cpp
 * @brief Entry point for ASAPCabinetFE.
 *
 * This file contains the main entry point for the ASAPCabinetFE application. It
 * initializes SDL subsystems, handles command-line arguments, and launches the App
 * class to run the frontend. The SDLBootstrap struct manages SDL initialization and
 * cleanup using RAII principles.
 */

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

/**
 * @struct SDLBootstrap
 * @brief RAII struct for initializing and cleaning up SDL subsystems.
 *
 * This struct initializes SDL, SDL_ttf, and SDL_image with required subsystems
 * (video, events, joystick, audio) and configures DPI awareness. It logs errors
 * using LOG_ERROR and throws exceptions on failure. Cleanup is performed
 * automatically on destruction.
 */
struct SDLBootstrap {
    std::string configPath; ///< Configuration file path (currently unused in struct).

    /**
     * @brief Constructs an SDLBootstrap and initializes SDL subsystems.
     *
     * Initializes SDL with video, events, joystick, and audio subsystems, retrieves
     * system DPI, sets DPI awareness hints, and initializes SDL_ttf and SDL_image.
     * Logs errors and throws runtime_error on failure.
     *
     * @throws std::runtime_error If SDL, SDL_ttf, or SDL_image initialization fails.
     */
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

    /**
     * @brief Destroys the SDLBootstrap and cleans up SDL subsystems.
     *
     * Cleans up SDL_image, SDL_ttf, and SDL subsystems, logging the cleanup process.
     */
    ~SDLBootstrap() {
        IMG_Quit();
        TTF_Quit();
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
        SDL_Quit();
        LOG_DEBUG("Main: SDL subsystems cleaned up");
    }
};

/**
 * @brief Main entry point for ASAPCabinetFE.
 *
 * Handles command-line arguments to display version information or launch the
 * application. Initializes SDL subsystems via SDLBootstrap, creates an App instance
 * with the configuration file path, and runs the application.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on successful execution or version display, non-zero on failure.
 */
int main(int argc, char* argv[]) {
    std::string configPath = "config.ini";

    // Handle --version argument
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << ASAPCABINETFE_VERSION_STRING << std::endl;
        std::cout << "Git Hash: " << ASAPCABINETFE_GIT_HASH << std::endl;
        std::cout << "Git Branch: " << ASAPCABINETFE_GIT_BRANCH << std::endl;
        if (std::string(ASAPCABINETFE_GIT_DIRTY) == "+dirty") {
            std::cout << " (Repository has uncommitted changes)" << std::endl;
        }
        return 0;
    }

    // Initialize SDL subsystems
    SDLBootstrap bootstrap;

    // Create and run the application
    App app(configPath);
    app.run();
    return 0;
}