/**
 * @file vpxtool_scanner.cpp
 * @brief Implements the VPXToolScanner class for loading and managing table metadata from vpxtool_index.json.
 *
 * This file provides the implementation of the VPXToolScanner class, which attempts to load
 * table metadata from a pre-generated vpxtool_index.json. If the index is not found or is
 * invalid, it attempts to run the 'vpxtool' command-line utility to generate a new index,
 * either from a user-specified path or by looking for 'vpxtool' in the system's PATH.
 * After loading/generating the index, it processes table metadata.
 */

#include "vpxtool_scanner.h"
#include "log/logging.h"
#include "utils/string_utils.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <future>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

// --- Helper functions for command execution and file checks ---
namespace CommandUtils {
    bool runCommand(const std::string& command, const std::string& working_directory, LoadingProgress* progress, std::string& output_log) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("INFO: Attempting to run command: " + command);
            progress->currentTask = "Running VPXTool...";
        }

        output_log.clear();
        std::string full_command_with_cd = "cd \"" + working_directory + "\" && " + command;
        FILE* pipe = popen(full_command_with_cd.c_str(), "r");

        if (!pipe) {
            LOG_ERROR("Failed to open pipe for command: " + full_command_with_cd);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: Failed to open pipe for VPXTool. Check permissions.");
            }
            return false;
        }

        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output_log += buffer;
        }

        int exit_code_raw = pclose(pipe);
        int exit_code = 0;

        if (WIFEXITED(exit_code_raw)) {
            exit_code = WEXITSTATUS(exit_code_raw);
        } else {
            LOG_ERROR("Command '" + command + "' terminated abnormally (raw exit code: " + std::to_string(exit_code_raw) + ")");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: VPXTool terminated abnormally.");
            }
            return false;
        }

        if (exit_code != 0) {
            LOG_DEBUG("Command '" + command + "' failed with exit code " + std::to_string(exit_code));
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: VPXTool failed with exit code " + std::to_string(exit_code));
                if (!output_log.empty()) {
                    progress->logMessages.push_back("VPXTool output: " + output_log);
                }
            }
            return false;
        }

        LOG_INFO("Command '" + command + "' executed successfully.");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("INFO: VPXTool command executed successfully.");
        }
        return true;
    }

    bool isExecutableFile(const std::string& path) {
        if (path.empty()) return false;
        std::error_code ec;
        if (!fs::is_regular_file(path, ec) || ec) return false;
        return (access(path.c_str(), X_OK) == 0);
    }
} // namespace CommandUtils

bool VPXToolScanner::scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    std::string jsonPath = settings.VPXTablesPath + "/" + settings.vpxtoolIndex;
    json vpxtoolJson;
    bool vpxtoolLoaded = false;
    std::string vpxtoolBinaryFullPath;
    std::string command_output_log;

    // --- Check if index has at least one table matching actual files ---
    auto isVpxtoolIndexValid = [&](const json& j, const std::vector<TableData>& scanTables) -> bool {
        if (!j.contains("tables") || !j["tables"].is_array()) return false;
        int validCount = 0;
        for (const auto& entry : j["tables"]) {
            std::string path = StringUtils::safeGetMetadataString(entry, "path");
            if (path.empty()) continue;
            for (const auto& t : scanTables) {
                if (t.vpxFile == path) { validCount++; break; }
            }
        }
        return validCount > 0;
    };

    auto tryLoadJson = [&]() {
        vpxtoolLoaded = false;
        LOG_DEBUG("Attempting to load vpxtool_index.json from: " + jsonPath);
        if (fs::exists(jsonPath)) {
            try {
                std::ifstream file(jsonPath);
                if (!file.is_open()) {
                    LOG_ERROR("Could not open vpxtool_index.json for reading: " + jsonPath);
                    if (progress) { std::lock_guard lock(progress->mutex); progress->logMessages.push_back("ERROR: Cannot open vpxtool_index.json."); }
                    return false;
                }
                file >> vpxtoolJson;
                file.close();
                if (vpxtoolJson.contains("tables") && vpxtoolJson["tables"].is_array()) {
                    vpxtoolLoaded = true;
                    LOG_INFO("Loaded vpxtool_index.json successfully from: " + jsonPath);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse vpxtool_index.json: " + std::string(e.what()));
            }
        } else {
            LOG_WARN("vpxtool_index.json not found at: " + jsonPath);
        }
        return vpxtoolLoaded;
    };

    // --- VPXTool fallback logic START ---
    if (!tryLoadJson() || !isVpxtoolIndexValid(vpxtoolJson, tables)) {
        LOG_WARN("VPXTool index missing, invalid, or contains no matching tables. Attempting to generate new index...");

        if (!settings.vpxtoolBin.empty() && CommandUtils::isExecutableFile(settings.vpxtoolBin)) {
            vpxtoolBinaryFullPath = settings.vpxtoolBin;
        } else {
            vpxtoolBinaryFullPath = "vpxtool"; // fallback to system PATH
        }

        if (!vpxtoolBinaryFullPath.empty()) {
            std::string command_to_run = "\"" + vpxtoolBinaryFullPath + "\" index \"" + settings.VPXTablesPath + "\"";
            LOG_DEBUG("Executing VPXTool command: " + command_to_run);
            if (!CommandUtils::runCommand(command_to_run, settings.VPXTablesPath, progress, command_output_log) ||
                !tryLoadJson() || !isVpxtoolIndexValid(vpxtoolJson, tables)) {
                LOG_WARN("VPXTool failed or produced no valid tables. Falling back to VPin scan.");
                if (progress) { std::lock_guard lock(progress->mutex); progress->logMessages.push_back("WARN: VPXTool failed or empty. Using VPin scanner fallback."); }
                return false; // fallback to VPin
            }
        } else {
            LOG_WARN("No VPXTool binary found. Falling back to VPin scan.");
            if (progress) { std::lock_guard lock(progress->mutex); progress->logMessages.push_back("WARN: VPXTool binary missing. Using VPin scanner fallback."); }
            return false;
        }
    }
    // --- VPXTool fallback logic END ---

    // --- Process VPXTool JSON ---
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Processing vpxtool_index.json metadata...";
        progress->totalTablesToLoad = vpxtoolJson["tables"].size();
        progress->currentTablesLoaded = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
        progress->logMessages.push_back("INFO: Processing " + std::to_string(vpxtoolJson["tables"].size()) + " entries from vpxtool_index.json.");
    }

    std::vector<std::future<void>> futures;
    std::atomic<int> processedVpxTool(0);
    const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency());

    for (const auto& tableJson : vpxtoolJson["tables"]) {
        while (futures.size() >= maxThreads) {
            for (auto it = futures.begin(); it != futures.end();) {
                if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    try { it->get(); } catch (...) {}
                    it = futures.erase(it);
                } else { ++it; }
            }
            std::this_thread::yield();
        }

        futures.push_back(std::async(std::launch::async, [&tableJson, &tables, progress, &processedVpxTool]() {
            // --- Table processing START ---
            try {
                if (!tableJson.is_object()) {
                    if (progress) { std::lock_guard lock(progress->mutex); progress->numNoMatch++; }
                    if (progress) { std::lock_guard lock(progress->mutex); progress->currentTablesLoaded = ++processedVpxTool; }
                    return;
                }
                std::string path = StringUtils::safeGetMetadataString(tableJson, "path");
                if (path.empty()) { if (progress) { std::lock_guard lock(progress->mutex); progress->numNoMatch++; progress->currentTablesLoaded = ++processedVpxTool; } return; }

                TableData* currentTable = nullptr;
                for (auto& table : tables) {
                    if (table.vpxFile == path) { currentTable = &table; break; }
                }
                if (!currentTable) { if (progress) { std::lock_guard lock(progress->mutex); progress->numNoMatch++; progress->currentTablesLoaded = ++processedVpxTool; } return; }

                if (tableJson.contains("table_info") && tableJson["table_info"].is_object()) {
                    const auto& tableInfo = tableJson["table_info"];
                    currentTable->tableName = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(tableInfo, "table_name", currentTable->title));
                    currentTable->tableAuthor = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(tableInfo, "author_name"));
                    currentTable->tableDescription = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(tableInfo, "table_description"));
                    currentTable->tableSaveDate = StringUtils::safeGetMetadataString(tableInfo, "table_save_date");
                    currentTable->tableReleaseDate = StringUtils::safeGetMetadataString(tableInfo, "release_date");
                    currentTable->tableVersion = StringUtils::safeGetMetadataString(tableInfo, "table_version");
                    currentTable->tableRevision = StringUtils::safeGetMetadataString(tableInfo, "table_save_rev");
                }
                currentTable->romName = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(tableJson, "game_name"));
                currentTable->romPath = StringUtils::safeGetMetadataString(tableJson, "rom_path");
                currentTable->tableLastModified = StringUtils::safeGetMetadataString(tableJson, "last_modified");
                currentTable->jsonOwner = "VPXTool Index";

                fs::path filePath(path);
                std::string filename = filePath.stem().string();
                currentTable->title = currentTable->tableName.empty() ? StringUtils::cleanMetadataString(filename) : currentTable->tableName;

                if (tableJson.contains("properties") && tableJson["properties"].is_object()) {
                    currentTable->manufacturer = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(tableJson["properties"], "manufacturer", ""));
                    currentTable->year = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(tableJson["properties"], "year", ""));
                }
                if (progress) { std::lock_guard lock(progress->mutex); progress->numMatched++; }
            } catch (...) { if (progress) { std::lock_guard lock(progress->mutex); progress->numNoMatch++; } }

            if (progress) { std::lock_guard lock(progress->mutex); progress->currentTablesLoaded = ++processedVpxTool; }
            // --- Table processing END ---
        }));
    }

    for (auto& future : futures) {
        try { future.get(); } catch (const std::exception& e) {
            LOG_ERROR("Thread exception during initial VPXTool processing: " + std::string(e.what()));
        }
    }

    // --- Final sanity check: ensure VPXTool actually matched tables ---
    if (progress && progress->numMatched == 0) {
        LOG_WARN("VPXTool processing completed, but no tables were matched. Falling back to VPin scan.");
        progress->logMessages.push_back("WARN: VPXTool produced 0 matched tables. Using VPin scanner fallback.");
        return false; // force fallback
    }

    LOG_INFO("Completed processing vpxtool_index.json.");
    return true;
}
