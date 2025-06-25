/**
 * @file table_launcher.cpp
 * @brief Implementation of the TableLauncher class.
 */
#include "table_launcher.h"
#include "log/logging.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdlib>

TableLauncher::TableLauncher(IConfigService* configService)
    : configService_(configService) {
    LOG_INFO("TableLauncher: Initialized");
}

std::pair<int, float> TableLauncher::launchTable(const TableData& table) {
    LOG_DEBUG("TableLauncher: Launching table: " << table.title);

    // Get settings
    const auto& settings = configService_->getSettings();
    
    // Build command
    std::string command = settings.vpxStartArgs + " " + settings.VPinballXPath + " " +
                         settings.vpxSubCmd + " \"" + table.vpxFile + "\" " +
                         settings.vpxEndArgs;
    LOG_DEBUG("TableLauncher: Command: " << command);

    // Record start time
    auto start = std::chrono::system_clock::now();
    LOG_INFO("TableLauncher: Launching " << table.title);

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
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;
    std::string timeFormatted = ss.str();
    LOG_INFO("Welcome back to ASAPCabinetFE.");
    LOG_INFO("You Played " << table.title << " for " << timeFormatted);

    return {result, timePlayed};
}