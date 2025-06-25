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
            LOG_ERROR("CommandUtils: Failed to open pipe for command: " << full_command_with_cd);
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
            LOG_ERROR("CommandUtils: Command '" << command << "' terminated abnormally (raw exit code: " << exit_code_raw << ")");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: VPXTool terminated abnormally.");
            }
            return false;
        }

        if (exit_code != 0) {
            LOG_DEBUG("CommandUtils: Command '" << command << "' failed with exit code " << exit_code);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: VPXTool failed with exit code " + std::to_string(exit_code));
                if (!output_log.empty()) {
                    progress->logMessages.push_back("VPXTool output: " + output_log);
                }
            }
            return false;
        }

        LOG_INFO("CommandUtils: Command '" << command << "' executed successfully.");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("INFO: VPXTool command executed successfully.");
        }
        return true;
    }

    bool isExecutableFile(const std::string& path) {
        if (path.empty()) {
            return false;
        }
        std::error_code ec;
        if (!fs::is_regular_file(path, ec) || ec) {
            return false;
        }
        return (access(path.c_str(), X_OK) == 0);
    }
}

bool VPXToolScanner::scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    std::string jsonPath = settings.VPXTablesPath + "/" + settings.vpxtoolIndex;
    json vpxtoolJson;
    bool vpxtoolLoaded = false;
    std::string vpxtoolBinaryFullPath;
    std::string command_output_log;

    auto tryLoadJson = [&]() {
        vpxtoolLoaded = false;
        LOG_DEBUG("VPXToolScanner: Attempting to load vpxtool_index.json from: " << jsonPath);
        if (fs::exists(jsonPath)) {
            try {
                std::ifstream file(jsonPath);
                if (!file.is_open()) {
                    LOG_ERROR("VPXToolScanner: Could not open vpxtool_index.json for reading: " << jsonPath);
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->logMessages.push_back("ERROR: Cannot open vpxtool_index.json for reading.");
                    }
                    return false;
                }
                file >> vpxtoolJson;
                file.close();
                if (vpxtoolJson.contains("tables") && vpxtoolJson["tables"].is_array()) {
                    vpxtoolLoaded = true;
                    LOG_INFO("VPXToolScanner: Loaded vpxtool_index.json successfully from: " << jsonPath);
                } else {
                    LOG_ERROR("VPXToolScanner: Invalid vpxtool_index.json: 'tables' missing or not an array.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->logMessages.push_back("ERROR: vpxtool_index.json is invalid: 'tables' array missing or malformed.");
                    }
                }
            } catch (const json::exception& e) {
                LOG_ERROR("VPXToolScanner: Failed to parse vpxtool_index.json: " << e.what());
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->logMessages.push_back("ERROR: Failed to parse vpxtool_index.json: " + std::string(e.what()));
                }
            } catch (const std::exception& e) {
                LOG_ERROR("VPXToolScanner: Unexpected error loading vpxtool_index.json: " << e.what());
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->logMessages.push_back("ERROR: Unexpected error loading vpxtool_index.json: " + std::string(e.what()));
                }
            }
        } else {
            LOG_WARN("VPXToolScanner: vpxtool_index.json not found at: " << jsonPath);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("INFO: vpxtool_index.json not found. Attempting to generate.");
            }
        }
        return vpxtoolLoaded;
    };

    if (tryLoadJson()) {
        LOG_INFO("VPXToolScanner: Using existing vpxtool_index.json.");
    } else {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Attempting to generate VPXTool index...";
            progress->logMessages.push_back("INFO: Checking for VPXTool binary to generate index.");
        }

        if (!settings.vpxtoolBin.empty()) {
            if (CommandUtils::isExecutableFile(settings.vpxtoolBin)) {
                vpxtoolBinaryFullPath = settings.vpxtoolBin;
                LOG_INFO("VPXToolScanner: Found custom VPXTool binary: " << vpxtoolBinaryFullPath);
            } else {
                LOG_DEBUG("VPXToolScanner: Custom vpxtool path specified, but executable not found or not executable: " << settings.vpxtoolBin);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->logMessages.push_back("WARNING: Custom VPXTool path invalid: " + settings.vpxtoolBin);
                }
            }
        }

        if (vpxtoolBinaryFullPath.empty()) {
            vpxtoolBinaryFullPath = "vpxtool";
            LOG_WARN("VPXToolScanner: No specific VPXTool binary path found. Attempting to run 'vpxtool' from system PATH.");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("INFO: Attempting to run 'vpxtool' from system PATH.");
            }
        }

        if (!vpxtoolBinaryFullPath.empty()) {
            std::string command_to_run = "\"" + vpxtoolBinaryFullPath + "\" index \"" + settings.VPXTablesPath + "\"";
            LOG_DEBUG("VPXToolScanner: Executing VPXTool command: " << command_to_run);
            if (CommandUtils::runCommand(command_to_run, settings.VPXTablesPath, progress, command_output_log)) {
                LOG_INFO("VPXToolScanner: VPXTool executed successfully. Attempting to load newly created index.");
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->logMessages.push_back("INFO: VPXTool executed successfully. Loading new index.");
                }
                if (tryLoadJson()) {
                    LOG_INFO("VPXToolScanner: Successfully loaded newly generated vpxtool_index.json.");
                } else {
                    LOG_ERROR("VPXToolScanner: Failed to load newly generated vpxtool_index.json. Check logs for VPXTool output.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->logMessages.push_back("ERROR: Failed to load newly generated index after VPXTool run. Is output valid?");
                    }
                }
            } else {
                LOG_DEBUG("VPXToolScanner: VPXTool command failed to execute.");
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->logMessages.push_back("ERROR: VPXTool command failed to run. Check log for details.");
                }
            }
        } else {
            LOG_INFO("VPXToolScanner: No VPXTool binary identified (neither specified path nor on system PATH).");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: No VPXTool binary found or specified. Cannot generate index.");
            }
        }
    }

    if (!vpxtoolLoaded) {
        LOG_WARN("VPXToolScanner: All attempts to load or generate vpxtool_index.json failed. Skipping VPXTool.");
        return false;
    }

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
                    try {
                        it->get();
                    } catch (const std::exception& e) {
                        LOG_ERROR("VPXToolScanner: Thread exception during initial VPXTool processing: " << e.what());
                    }
                    it = futures.erase(it);
                } else {
                    ++it;
                }
            }
            std::this_thread::yield();
        }

        futures.push_back(std::async(std::launch::async, [&tableJson, &tables, progress, &processedVpxTool]() {
            try {
                if (!tableJson.is_object()) {
                    LOG_DEBUG("VPXToolScanner: Skipping non-object table entry from JSON.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numNoMatch++;
                    }
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTablesLoaded = ++processedVpxTool;
                        progress->currentTask = "VPXTool metadata: " + std::to_string(processedVpxTool) + " of " + std::to_string(progress->totalTablesToLoad) + " entries";
                    }
                    return;
                }
                std::string path = StringUtils::safeGetMetadataString(tableJson, "path");
                if (path.empty()) {
                    LOG_DEBUG("VPXToolScanner: Skipping table with empty path from JSON.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numNoMatch++;
                    }
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTablesLoaded = ++processedVpxTool;
                        progress->currentTask = "VPXTool metadata: " + std::to_string(processedVpxTool) + " of " + std::to_string(progress->totalTablesToLoad) + " entries";
                    }
                    return;
                }

                TableData* currentTable = nullptr;
                for (auto& table : tables) {
                    if (table.vpxFile == path) {
                        currentTable = &table;
                        break;
                    }
                }

                if (!currentTable) {
                    LOG_DEBUG("VPXToolScanner: Table file from vpxtool_index.json not found in initial scan list: " << path);
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numNoMatch++;
                    }
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTablesLoaded = ++processedVpxTool;
                        progress->currentTask = "VPXTool metadata: " + std::to_string(processedVpxTool) + " of " + std::to_string(progress->totalTablesToLoad) + " entries";
                    }
                    return;
                }

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
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numMatched++;
                }
            } catch (const json::exception& e) {
                LOG_ERROR("VPXToolScanner: JSON parsing error for table from vpxtool_index.json with path " << StringUtils::safeGetMetadataString(tableJson, "path", "N/A") << ": " << e.what());
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
                }
            } catch (...) {
                LOG_ERROR("VPXToolScanner: Unexpected error processing table from vpxtool_index.json.");
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
                }
            }
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = ++processedVpxTool;
                progress->currentTask = "VPXTool metadata: " + std::to_string(processedVpxTool) + " of " + std::to_string(progress->totalTablesToLoad) + " entries";
            }
        }));
    }

    for (auto& future : futures) {
        try {
            future.get();
        } catch (const std::exception& e) {
            LOG_ERROR("VPXToolScanner: Thread exception during initial VPXTool processing: " << e.what());
        }
    }

    LOG_INFO("VPXToolScanner: Completed processing vpxtool_index.json.");
    return true;
}