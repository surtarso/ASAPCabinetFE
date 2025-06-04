// core/loading_progress.h
#ifndef LOADING_PROGRESS_H
#define LOADING_PROGRESS_H

#include <string>
#include <vector>
#include <mutex>
#include <deque> // For log buffer

struct LoadingProgress {
    std::mutex mutex;
    int currentTablesLoaded = 0; // Per-table progress
    int totalTablesToLoad = 0;  // Total tables for per-table progress
    int currentStage = 0;       // Overall progress stage (fetching VPSDB, scanning, enriching, saving, sorting)
    int totalStages = 5;        // Total stages: fetching VPSDB, scanning/loading, enriching, saving, sorting/indexing
    std::string currentTask = "Initializing..."; // Current task description
    int numMatched = 0;         // Number of tables matched
    int numNoMatch = 0;         // Number of tables with no metadata
    // For mini terminal: store recent log messages
    std::deque<std::string> logMessages;
    size_t maxLogMessages = 10; // Keep last 10 messages for display

    void addLogMessage(const std::string& message) {
        logMessages.push_back(message);
        if (logMessages.size() > maxLogMessages) {
            logMessages.pop_front();
        }
    }
};

#endif // LOADING_PROGRESS_H