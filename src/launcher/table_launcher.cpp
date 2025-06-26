/**
 * @file table_launcher.cpp
 * @brief Implementation of the TableLauncher class.
 */

#include "launcher/table_launcher.h"
#include "log/logging.h"
#include <chrono>
#include <string>
#include <cstdlib>

TableLauncher::TableLauncher(IConfigService* configService)
    : configService_(configService) {
    LOG_INFO("TableLauncher Initialized");
}

std::pair<int, float> TableLauncher::launchTable(const TableData& table) {
    LOG_DEBUG("Launching table: " + table.title);

    // Get settings
    const auto& settings = configService_->getSettings();

    // Build command
    std::string command = settings.vpxStartArgs + " " + settings.VPinballXPath + " " +
                         settings.vpxSubCmd + " \"" + table.vpxFile + "\" " +
                         settings.vpxEndArgs;
    LOG_DEBUG("Command: " + command);

    // Record start time
    auto start = std::chrono::system_clock::now();
    LOG_INFO("Launching " + table.title);

    // Execute command (Linux-specific for now)
    int result = std::system((command + " > /dev/null 2>&1").c_str());

    // Calculate playtime
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<float> duration = end - start;
    float timePlayed = duration.count();

    // Format playtime as H:M:S
    int totalSeconds = static_cast<int>(timePlayed);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    std::string timeFormatted = std::to_string(hours) + ":" +
                               (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                               (seconds < 10 ? "0" : "") + std::to_string(seconds);
    LOG_INFO("Welcome back to ASAPCabinetFE.");
    LOG_INFO("You Played " + table.title + " for " + timeFormatted);

    return {result, timePlayed};
}