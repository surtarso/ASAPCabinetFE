/**
 * @file data_enricher.cpp
 * @brief Implements the DataEnricher class for enriching table data in ASAPCabinetFE.
 *
 * This file provides the implementation of the DataEnricher class, which enriches
 * TableData objects with metadata from vpxtool_index.json and optionally VPSDB. It
 * includes utility methods for cleaning strings and safely extracting JSON values,
 * supports progress tracking via LoadingProgress, and is configurable via Settings
 * (e.g., VPXTablesPath, fetchVPSdb). The process can be extended with configUI for
 * custom metadata sources or cleaning rules in the future.
 */

#include "tables/data_enricher.h"
#include "tables/vpsdb/vps_database_client.h"
#include "utils/logging.h"
#include <filesystem>
#include <fstream>
#include <json.hpp>

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations
using json = nlohmann::json; // Alias for nlohmann::json to simplify JSON usage

std::string DataEnricher::cleanString(const std::string& input) {
    std::string result = input;
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end()); // Remove carriage returns
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end()); // Remove newlines
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return std::iscntrl(c); }), result.end()); // Remove control characters
    size_t first = result.find_first_not_of(" \t");
    size_t last = result.find_last_not_of(" \t");
    if (first == std::string::npos) return ""; // Return empty if all whitespace
    return result.substr(first, last - first + 1); // Trim leading/trailing whitespace
}

std::string DataEnricher::safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue) {
    if (j.contains(key)) {
        if (j[key].is_string()) {
            return j[key].get<std::string>();
        } else if (j[key].is_number()) {
            return std::to_string(j[key].get<double>()); // Convert number to string
        } else {
            LOG_DEBUG("Field " << key << " is not a string or number, type: " << j[key].type_name());
            return defaultValue;
        }
    }
    return defaultValue; // Return default if key is missing
}

void DataEnricher::enrich(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    std::string jsonPath = settings.VPXTablesPath + settings.vpxtoolIndex;
    if (!fs::exists(jsonPath)) {
        LOG_INFO("DataEnricher: vpxtool_index.json not found at: " << jsonPath);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch += tables.size(); // Mark all tables as unmatched
        }
        return;
    }

    json vpxtoolJson;
    try {
        std::ifstream file(jsonPath);
        file >> vpxtoolJson; // Parse vpxtool_index.json
        file.close();
    } catch (const std::exception& e) {
        LOG_ERROR("DataEnricher: Failed to parse vpxtool_index.json: " << e.what());
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch += tables.size(); // Mark all tables as unmatched
        }
        return;
    }

    // Load VPSDB if enabled
    VpsDatabaseClient vpsClient(settings.vpsDbPath);
    bool vpsLoaded = false;
    if (settings.fetchVPSdb && vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress) && vpsClient.load(progress)) {
        vpsLoaded = true;
    } else if (settings.fetchVPSdb) {
        LOG_ERROR("DataEnricher: Failed to load vpsdb.json, using vpxtool only");
    }

    if (!vpxtoolJson.contains("tables") || !vpxtoolJson["tables"].is_array()) {
        LOG_ERROR("DataEnricher: Invalid vpxtool_index.json: 'tables' missing or not an array");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch += tables.size(); // Mark all tables as unmatched
        }
        return;
    }

    int processed = 0;
    for (const auto& tableJson : vpxtoolJson["tables"]) {
        try {
            if (!tableJson.is_object()) {
                LOG_DEBUG("DataEnricher: Skipping non-object table entry");
                continue;
            }
            std::string path = safeGetString(tableJson, "path");
            if (path.empty()) {
                LOG_DEBUG("DataEnricher: Skipping table with empty path");
                continue;
            }

            bool found = false;
            for (auto& table : tables) {
                if (table.vpxFile != path) continue;

                found = true;
                if (tableJson.contains("table_info") && tableJson["table_info"].is_object()) {
                    const auto& tableInfo = tableJson["table_info"];
                    table.tableName = cleanString(safeGetString(tableInfo, "table_name", table.title));
                    table.authorName = cleanString(safeGetString(tableInfo, "author_name"));
                    table.tableDescription = cleanString(safeGetString(tableInfo, "table_description"));
                    table.tableSaveDate = safeGetString(tableInfo, "table_save_date");
                    table.releaseDate = safeGetString(tableInfo, "release_date");
                    table.tableVersion = safeGetString(tableInfo, "table_version");
                    table.tableRevision = safeGetString(tableInfo, "table_save_rev");
                }
                table.gameName = cleanString(safeGetString(tableJson, "game_name"));
                table.romPath = safeGetString(tableJson, "rom_path");
                table.lastModified = safeGetString(tableJson, "last_modified");

                fs::path filePath(path);
                std::string filename = filePath.stem().string();
                table.title = table.tableName.empty() ? cleanString(filename) : table.tableName; // Fallback to filename if tableName is empty

                if (vpsLoaded) {
                    vpsClient.enrichTableData(tableJson, table, progress); // Enrich with VPSDB if loaded
                }
                break;
            }
            processed++;
            if (progress && !found) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->numNoMatch++; // Increment unmatched count if no match found
            }
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = processed;
            }
        } catch (const json::exception& e) {
            LOG_DEBUG("DataEnricher: JSON parsing error for table with path " << safeGetString(tableJson, "path", "N/A") << ": " << e.what());
            continue;
        }
    }
}