/**
 * @file loading_screen.h
 * @brief Defines the LoadingScreen class for rendering a loading UI in ASAPCabinetFE.
 *
 * This header provides the LoadingScreen class, which manages an ImGui-based loading
 * interface displayed during the initialization of ASAPCabinetFE. The interface includes
 * multiple progress bars (overall, per-table, and match progress), a mini terminal for
 * log messages, and animated visual effects. It relies on LoadingProgress data for
 * real-time updates and supports customization of window size, colors, and layout
 * through internal logic that can be extended via configUI in the future.
 */

#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H // Header guard to prevent multiple inclusions

#include <memory>
#include <string>
#include <SDL2/SDL.h> // For SDL_DisplayMode, used indirectly via ImGui for window sizing
#include "imgui.h" // For ImGui rendering of the loading interface
#include "core/loading_progress.h" // Include the progress data structure for real-time updates

/**
 * @class LoadingScreen
 * @brief Manages the ImGui-based loading screen for ASAPCabinetFE initialization.
 *
 * This class renders a loading interface with multiple progress indicators (overall
 * progress with fade animation, per-table progress, and match progress) and a mini
 * terminal displaying recent log messages. The layout is dynamically sized based on
 * the display resolution, with a maximum cap for usability. Colors and animations
 * (e.g., deep blue for overall progress, purple for tables, pinkish-white for matches)
 * are hardcoded but can be made configurable via configUI by extending the design.
 * The screen updates in real-time using data from a LoadingProgress object.
 */
class LoadingScreen {
public:
    /**
     * @brief Constructs a LoadingScreen instance.
     *
     * Initializes the loading screen with a shared pointer to LoadingProgress, which
     * provides real-time data on loading stages, table counts, and log messages. The
     * constructor sets up the internal state for rendering, including dynamic window
     * sizing (max 600x400 pixels or 50% width/70% height of the display, whichever
     * is smaller).
     *
     * @param progress Shared pointer to the LoadingProgress data structure, which
     *                 contains loading statistics (e.g., currentStage, totalStages,
     *                 currentTablesLoaded, numMatched) and log messages.
     */
    LoadingScreen(std::shared_ptr<LoadingProgress> progress);

    /**
     * @brief Renders the loading screen with progress and stats.
     *
     * This method should be called within the ImGui rendering loop to display the
     * loading interface. It includes:
     * - An overall progress bar with a sinusoidal fade animation (deep blue, 0.7-1.0
     *   alpha, 3 rad/s) showing the ratio of currentStage to totalStages.
     * - A per-table progress bar (purple) based on currentTablesLoaded vs. totalTablesToLoad.
     * - A match progress bar (pinkish-white) showing numMatched vs. totalTablesToLoad.
     * - A mini terminal displaying recent log messages (faded yellow text), auto-scrolling
     *   to the bottom unless manually scrolled.
     * - Stats text for total matched and unmatched tables.
     * The window is centered, non-resizable, and locks LoadingProgress data with a mutex
     * to ensure thread safety during updates.
     */
    void render();

private:
    std::shared_ptr<LoadingProgress> loadingProgress_; ///< Shared pointer to the LoadingProgress object, providing real-time loading data and log messages, accessed thread-safely via a mutex.
};

#endif // LOADING_SCREEN_H