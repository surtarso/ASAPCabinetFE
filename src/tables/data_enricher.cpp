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

std::string safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue = "") {
    if (j.contains(key)) {
        if (j[key].is_string()) {
            return j[key].get<std::string>();
        } else if (j[key].is_number()) {
            return std::to_string(j[key].get<double>());
        } else {
            LOG_DEBUG("Field " << key << " is not a string or number, type: " << j[key].type_name());
            return defaultValue;
        }
    }
    return defaultValue;
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
            std::string path = safeGetString(tableJson, "path");
            if (path.empty()) {
                LOG_DEBUG("DataEnricher: Skipping table with empty path");
                continue;
            }

            for (auto& table : tables) {
                if (table.vpxFile != path) continue;

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
                table.title = table.tableName.empty() ? cleanString(filename) : table.tableName;

                if (vpsLoaded) {
                    vpsClient.enrichTableData(tableJson, table);
                }
                break;
            }
        } catch (const json::exception& e) {
            LOG_DEBUG("DataEnricher: JSON parsing error for table with path " << safeGetString(tableJson, "path", "N/A") << ": " << e.what());
            continue;
        }
    }
}