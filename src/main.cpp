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
#include "log/logging.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <gst/gst.h>
#include <glib.h>    // Include for GMainLoop
#include <iostream>
#include <string>
#include <filesystem>

// Global GStreamer event loop and thread
/**
 * @var g_main_loop_global
 * @brief Global GMainLoop instance for the GStreamer event loop.
 *
 * This pointer is used to manage the GStreamer event processing loop, initialized
 * during SDLBootstrap construction and cleaned up during destruction.
 */
static GMainLoop* g_main_loop_global = nullptr;

/**
 * @var g_main_loop_thread_global
 * @brief Global GThread instance for the GStreamer event loop thread.
 *
 * This pointer represents the thread running the GStreamer event loop, created
 * during SDLBootstrap construction and joined during destruction.
 */
static GThread* g_main_loop_thread_global = nullptr;

// Function to run the GStreamer event loop
/**
 * @brief Runs the GStreamer event loop in a separate thread.
 *
 * This function is executed in a dedicated thread to process GStreamer events.
 * It runs the global GMainLoop until quit is called.
 *
 * @param data Unused parameter (cast to void to suppress warnings).
 * @return NULL to indicate thread completion.
 */
static gpointer run_gstreamer_event_loop(gpointer data) {
    (void)data; // Cast to void to suppress unused parameter warning
    g_main_loop_run(g_main_loop_global);
    LOG_DEBUG("Main: GStreamer event loop stopped.");
    return NULL;
}

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
        // Create and start the global GStreamer event loop thread
        g_main_loop_global = g_main_loop_new(NULL, FALSE);
        if (!g_main_loop_global) {
            LOG_ERROR("Main: Failed to create global GMainLoop for GStreamer events.");
            gst_deinit(); // Deinit GStreamer if GMainLoop creation fails
            throw std::runtime_error("Main: GStreamer MainLoop initialization failed");
        }
        g_main_loop_thread_global = g_thread_new("gstreamer-event-thread", run_gstreamer_event_loop, nullptr);
        if (!g_main_loop_thread_global) {
            LOG_ERROR("Main: Failed to create global GStreamer event thread.");
            g_main_loop_unref(g_main_loop_global);
            g_main_loop_global = nullptr;
            gst_deinit();
            throw std::runtime_error("Main: GStreamer Thread initialization failed");
        }
        LOG_DEBUG("Main: Global GStreamer event loop thread started.");

        // Initialize GStreamer globally
        gst_init(nullptr, nullptr);
        LOG_DEBUG("Main: SDL subsystems initialized");
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

        // Quit and join the global GStreamer event loop thread gracefully
        if (g_main_loop_global && g_main_loop_is_running(g_main_loop_global)) {
            g_main_loop_quit(g_main_loop_global);
            LOG_DEBUG("Main: GStreamer global GMainLoop quit requested.");
        }
        if (g_main_loop_thread_global) {
            g_thread_join(g_main_loop_thread_global);
            g_thread_unref(g_main_loop_thread_global);
            g_main_loop_thread_global = nullptr;
            LOG_DEBUG("Main: GStreamer global event thread joined and unref'd.");
        }
        if (g_main_loop_global) {
            g_main_loop_unref(g_main_loop_global);
            g_main_loop_global = nullptr;
            LOG_DEBUG("Main: GStreamer global GMainLoop unref'd.");
        }
        gst_deinit();

        LOG_DEBUG("Main: Subsystems cleaned up");
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
    std::string configPath = "data/settings.json";
    // // Resolve exeDir
    // std::string exeDir;
    // char path[PATH_MAX];
    // ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
    // if (count != -1) {
    //     path[count] = '\0';
    //     exeDir = std::filesystem::path(path).parent_path().string() + "/";
    // } else {
    //     exeDir = std::filesystem::current_path().string() + "/";
    //     LOG_DEBUG("main: Failed to resolve executable path, using current directory: " << exeDir);
    // }

    // // Set configPath using exeDir
    // std::string configPath = exeDir + "data/settings.json";
    // std::filesystem::path configFilePath(configPath);
    // std::filesystem::path configDir = configFilePath.parent_path();
    // if (!configDir.empty()) {
    //     try {
    //         if (!std::filesystem::exists(configDir)) {
    //             std::filesystem::create_directories(configDir);
    //             LOG_DEBUG("main: Created directory " << configDir.string());
    //         }
    //     } catch (const std::filesystem::filesystem_error& e) {
    //         LOG_ERROR("Failed to create directory " << configDir.string() << ": " << e.what());
    //         LOG_ERROR("Live changes will not persist on restart, check writing permissions.");
    //     }
    // }

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