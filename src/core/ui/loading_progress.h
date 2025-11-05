/**
 * @file loading_progress.h
 * @brief Defines the LoadingProgress struct for tracking loading state in ASAPCabinetFE.
 *
 * This header provides the LoadingProgress struct, which holds the state of the loading
 * process for ASAPCabinetFE. It tracks overall progress (stages), per-table loading,
 * metadata matching, and log messages for display in a loading screen. The struct is
 * thread-safe, using a mutex to protect access to its members, and is designed to be
 * shared between the loading thread and the UI rendering thread (e.g., via LoadingScreen).
 * Fields like progress bars' colors or log message limits can be made configurable in
 * the future by exposing them through configUI.
 */

#ifndef LOADING_PROGRESS_H
#define LOADING_PROGRESS_H // Header guard to prevent multiple inclusions

#include <string> // For std::string in task descriptions and log messages
#include <vector> // Not used directly but included for potential future expansion
#include <mutex> // For thread-safe access to the struct's members
#include <deque> // For logMessages buffer with efficient front/back operations

/**
 * @struct LoadingProgress
 * @brief Tracks the loading progress state for ASAPCabinetFE.
 *
 * This struct stores the state of the loading process, including overall progress
 * (stages like fetching, scanning, matchmaking), per-table loading progress, metadata
 * matching statistics, and a log message buffer for UI display. It is designed to be
 * shared between a loading thread and a rendering thread (e.g., LoadingScreen), with
 * thread safety ensured via a mutex. The struct supports a mini terminal display in
 * the UI by maintaining a capped deque of recent log messages.
 */
struct LoadingProgress {
    std::mutex mutex; ///< Mutex for thread-safe access to all members, used by loading and rendering threads.

    size_t currentTablesLoaded = 0; ///< Number of tables currently loaded (used for per-table progress bar).
    size_t totalTablesToLoad = 0;   ///< Total number of tables to load (denominator for per-table progress bar).
    int currentStage = 0;           ///< Current overall progress stage (e.g., fetching VPSDB, scanning, matchmaking).
    int totalStages = 11;           ///< Total number of stages

    std::string currentTask = "Initializing..."; ///< Description of the current task (e.g., "Scanning Tables (5)").

    int numMatched = 0;          ///< Number of tables successfully matched with metadata (used for match progress bar).
    int numNoMatch = 0;          ///< Number of tables with no metadata match (displayed in UI stats).

    std::deque<std::string> logMessages; ///< Buffer of recent log messages for display in the loading screen's mini terminal.
    size_t maxLogMessages = 10; ///< Maximum number of log messages to store (keeps last 10, older messages are removed).

    /**
     * @brief Adds a log message to the buffer for UI display.
     *
     * Appends a new log message to the logMessages deque, maintaining a maximum size
     * of maxLogMessages by removing the oldest message if necessary. This method is
     * called by the loading thread to report progress or issues, and the messages are
     * displayed in the LoadingScreen's mini terminal.
     *
     * @param message The log message to add (e.g., "DEBUG: Loaded table X").
     * @note Thread safety must be ensured by locking the mutex before calling this method.
     */
    void addLogMessage(const std::string& message) {
        logMessages.push_back(message);
        if (logMessages.size() > maxLogMessages) {
            logMessages.pop_front();
        }
    }
};

#endif // LOADING_PROGRESS_H
