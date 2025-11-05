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
#include "editor/editor.h"
#include <curl/curl.h>
#include "log/logging.h"
#include "log/logger.h"
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
 * This struct initializes GST, SDL, SDL_ttf, and SDL_image with required subsystems
 * (video, events, joystick, audio) and configures DPI awareness. It logs errors
 * using LOG_ERROR and throws exceptions on failure. Cleanup is performed
 * automatically on destruction.
 */
struct SDLBootstrap {
    std::string configPath; ///< Configuration file path

    /**
     * @brief Constructs an SDLBootstrap and initializes GST and SDL subsystems.
     *
     * Initializes SDL with video, events, joystick, and audio subsystems, retrieves
     * system DPI, sets DPI awareness hints, and initializes SDL_ttf, SDL_image, and
     * GStreamer. Logs errors and throws runtime_error on failure.
     *
     * @throws std::runtime_error If GST, SDL, SDL_ttf, or SDL_image initialization fails.
     */
    SDLBootstrap() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
            LOG_ERROR("Failed to initialize SDL: " + std::string(SDL_GetError()));
            throw std::runtime_error("Main: SDL initialization failed");
        }

        // Get system DPI
        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
            LOG_ERROR("Warning: Could not get DPI: " + std::string(SDL_GetError()));
            ddpi = 96.0f; // Default fallback DPI
        }

        // Enable DPI awareness
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

        if (TTF_Init() < 0) {
            LOG_ERROR("Failed to initialize TTF: " + std::string(TTF_GetError()));
            SDL_Quit();
            throw std::runtime_error("Main: TTF initialization failed");
        }
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            LOG_ERROR("Failed to initialize IMG: " + std::string(IMG_GetError()));
            TTF_Quit();
            SDL_Quit();
            throw std::runtime_error("Main: IMG initialization failed");
        }
        LOG_INFO("SDL Subsystems Initialized");
        // Initialize libcurl
        curl_global_init(CURL_GLOBAL_DEFAULT);
        LOG_DEBUG("Initialized libcurl");
    }

    /**
     * @brief Destroys the SDLBootstrap and cleans up SDL subsystems.
     *
     * Cleans up SDL_image, SDL_ttf, and SDL subsystems, logging the cleanup process.
     * Also quits and joins the GStreamer event loop thread gracefully.
     */
    ~SDLBootstrap() {
        IMG_Quit();
        TTF_Quit();
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
        SDL_Quit();
        LOG_DEBUG("Subsystems cleaned up");
        // Cleanup libcurl
        curl_global_cleanup();
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
    // Handle --version / -v argument
    if (argc > 1 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        std::cout << "ASAPCabinetFE version " << ASAPCABINETFE_VERSION_STRING << std::endl;
        std::cout << "Git Hash: " << ASAPCABINETFE_GIT_HASH << std::endl;
        std::cout << "Git Branch: " << ASAPCABINETFE_GIT_BRANCH << std::endl;
        if (std::string(ASAPCABINETFE_GIT_DIRTY) == "+dirty") {
            std::cout << " (Repository has uncommitted changes)" << std::endl;
        }
        return 0;
    }

    // Handle --help / -h argument
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "\n"
                << "=========================================================\n"
                << "          ASAPCabinetFE - Command Line Options\n"
                << "=========================================================\n"
                << "Version: " << ASAPCABINETFE_VERSION_STRING << "\n\n"
                << "Usage:\n"
                << "  ./ASAPCabinetFE [option]\n\n"
                << "Options:\n"
                << "  <no args>              Launch the main Front-End\n"
                << "  --software-renderer    Launch in software mode (debug)\n"
                << "  -e, --editor           Launch the Table Editor (not yet implemented)\n"
                << "  -v, --version          Display version information\n"
                << "  -h, --help             Show this help message\n\n"
                << "Example:\n"
                << "  ./ASAPCabinetFE --editor\n"
                << "=========================================================\n"
                << "User's Manual:\n"
                << "github.com/surtarso/ASAPCabinetFE/blob/main/UserManual.md\n"
                << "=========================================================\n"
                << std::endl;
        return 0;
    }

    // Resolve exeDir
    std::string exeDir;
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (count != -1) {
        path[count] = '\0';
        exeDir = std::filesystem::path(path).parent_path().string() + "/";
    } else {
        exeDir = std::filesystem::current_path().string() + "/";
        LOG_WARN("Failed to resolve executable path, using current directory: " + exeDir);
    }

    // Initialize logger
    #ifdef DEBUG_LOGGING
        std::string logFile = exeDir + "logs/debug.log";
        asap::logging::Logger::getInstance().initialize(logFile, true);
    #else
        std::string logFile = exeDir + "logs/asapcab.log";
        asap::logging::Logger::getInstance().initialize(logFile, false);
    #endif

    // Set configPath using exeDir
    std::string configPath = exeDir + "data/settings.json";
    std::filesystem::path configFilePath(configPath);
    std::filesystem::path configDir = configFilePath.parent_path();
    if (!configDir.empty()) {
        try {
            if (!std::filesystem::exists(configDir)) {
                std::filesystem::create_directories(configDir);
                LOG_DEBUG("Created directory " + configDir.string());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR("Failed to create directory " + configDir.string() + ": " + std::string(e.what()));
            LOG_ERROR("Live changes will not persist on restart, check writing permissions.");
        }
    }

    // Initialize SDL subsystems
    SDLBootstrap bootstrap;

    // Handle --editor / -e argument
    // Create and run the editor (vpxguitools port)
    if (argc > 1 && (std::string(argv[1]) == "--editor" || std::string(argv[1]) == "-e")) {
        Editor editor(configPath);
        editor.run();
        return 0;
    }

    // Parse --software-renderer flag
    bool forceSoftware = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--software-renderer") {
            forceSoftware = true;
            break;
        }
    }

    // Create and run the application
    App app(configPath, forceSoftware);
    app.run();

    return 0;
}
