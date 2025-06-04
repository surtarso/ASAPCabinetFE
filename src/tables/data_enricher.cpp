#include "tables/data_enricher.h"
#include "tables/vpsdb/vps_database_client.h"
#include "utils/logging.h"
#include <filesystem>
#include <fstream>
#include <json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string DataEnricher::cleanString(const std::string& input) {
    std::string result = input;
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    size_t first = result.find_first_not_of(" \t");
    size_t last = result.find_last_not_of(" \t");
    if (first == std::string::npos) return "";
    return result.substr(first, last - first + 1);
}

void DataEnricher::enrich(const Settings& settings, std::vector<TableData>& tables) {
    std::string jsonPath = settings.VPXTablesPath + settings.vpxtoolIndex;
    if (!fs::exists(jsonPath)) {
        LOG_INFO("DataEnricher: vpxtool_index.json not found at: " << jsonPath);
        return;
    }

    json vpxtoolJson;
    try {
        std::ifstream file(jsonPath);
        file >> vpxtoolJson;
        file.close();
    } catch (const std::exception& e) {
        LOG_ERROR("DataEnricher: Failed to parse vpxtool_index.json: " << e.what());
        return;
    }

    VpsDatabaseClient vpsClient(settings.vpsDbPath);
    bool vpsLoaded = false;
    if (settings.fetchVPSdb && vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency) && vpsClient.load()) {
        vpsLoaded = true;
    } else if (settings.fetchVPSdb) {
        LOG_ERROR("DataEnricher: Failed to load vpsdb.json, using vpxtool only");
    }

    if (!vpxtoolJson.contains("tables") || !vpxtoolJson["tables"].is_array()) {
        LOG_ERROR("DataEnricher: Invalid vpxtool_index.json: 'tables' missing or not an array");
        return;
    }

    for (const auto& tableJson : vpxtoolJson["tables"]) {
        try {
            if (!tableJson.is_object()) {
                LOG_DEBUG("DataEnricher: Skipping non-object table entry");
                continue;
            }
            std::string path = tableJson.value("path", "");
            if (path.empty()) {
                LOG_DEBUG("DataEnricher: Skipping table with empty path");
                continue;
            }

            for (auto& table : tables) {
                if (table.vpxFile != path) continue;

                // Extract JSON fields
                if (tableJson.contains("table_info") && tableJson["table_info"].is_object()) {
                    const auto& tableInfo = tableJson["table_info"];
                    try {
                        table.tableName = cleanString(tableInfo.value("table_name", table.title));
                        table.authorName = cleanString(tableInfo.value("author_name", ""));
                        table.tableDescription = cleanString(tableInfo.value("table_description", ""));
                        table.tableSaveDate = tableInfo.value("table_save_date", "");
                        table.releaseDate = tableInfo.value("release_date", "");
                        table.tableVersion = tableInfo["table_version"].is_string() ? tableInfo["table_version"].get<std::string>() :
                                             tableInfo["table_version"].is_number() ? std::to_string(tableInfo["table_version"].get<double>()) :
                                             tableInfo["table_version"].is_null() ? "" : "";
                        table.tableRevision = tableInfo.value("table_save_rev", "");
                    } catch (const json::exception& e) {
                        LOG_DEBUG("DataEnricher: Error parsing table_info for " << path << ": " << e.what());
                    }
                }
                table.gameName = cleanString(tableJson.value("game_name", ""));
                table.romPath = tableJson.value("rom_path", "");
                table.lastModified = tableJson.value("last_modified", "");

                // Set title from tableName or filename
                fs::path filePath(path);
                std::string filename = filePath.stem().string();
                table.title = table.tableName.empty() ? cleanString(filename) : table.tableName;

                if (vpsLoaded) {
                    vpsClient.enrichTableData(tableJson, table);
                }
                break;
            }
        } catch (const json::exception& e) {
            LOG_DEBUG("DataEnricher: JSON parsing error for table with path " << tableJson.value("path", "N/A") << ": " << e.what());
            continue;
        }
    }
}