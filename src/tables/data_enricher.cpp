#include "tables/data_enricher.h"
#include "tables/vps_database_client.h"
#include "utils/logging.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

void DataEnricher::enrich(const Settings& settings, std::vector<TableData>& tables) {
    std::string jsonPath = settings.VPXTablesPath + settings.vpxtoolIndex;
    if (!fs::exists(jsonPath)) {
        LOG_INFO("DataEnricher: vpxtool_index.json not found at: " << jsonPath);
        return;
    }

    try {
        std::ifstream file(jsonPath);
        json vpxtoolJson;
        file >> vpxtoolJson;
        file.close();

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
                if (!tableJson.is_object()) continue;
                std::string path = tableJson.contains("path") && tableJson["path"].is_string() ? tableJson["path"].get<std::string>() : "";
                if (path.empty()) continue;

                for (auto& table : tables) {
                    if (table.vpxFile != path) continue;

                    if (tableJson.contains("table_info") && tableJson["table_info"].is_object()) {
                        const auto& tableInfo = tableJson["table_info"];
                        if (tableInfo.contains("table_name") && tableInfo["table_name"].is_string()) {
                            table.tableName = tableInfo["table_name"].get<std::string>();
                        } else {
                            table.tableName = table.title;
                        }
                        if (table.tableName.empty()) {
                            table.tableName = table.title;
                        }
                        if (tableInfo.contains("author_name") && tableInfo["author_name"].is_string()) {
                            table.authorName = tableInfo["author_name"].get<std::string>();
                        }
                        if (tableInfo.contains("table_description") && tableInfo["table_description"].is_string()) {
                            table.tableDescription = tableInfo["table_description"].get<std::string>();
                        }
                        if (tableInfo.contains("table_save_date") && tableInfo["table_save_date"].is_string()) {
                            table.tableSaveDate = tableInfo["table_save_date"].get<std::string>();
                        }
                        if (tableInfo.contains("release_date") && tableInfo["release_date"].is_string()) {
                            table.releaseDate = tableInfo["release_date"].get<std::string>();
                        }
                        if (tableInfo.contains("table_version")) {
                            if (tableInfo["table_version"].is_string()) {
                                table.tableVersion = tableInfo["table_version"].get<std::string>();
                            } else if (tableInfo["table_version"].is_number()) {
                                table.tableVersion = std::to_string(tableInfo["table_version"].get<double>());
                            }
                        }
                        if (tableInfo.contains("table_save_rev") && tableInfo["table_save_rev"].is_string()) {
                            table.tableRevision = tableInfo["table_save_rev"].get<std::string>();
                        }
                    }
                    if (tableJson.contains("game_name") && tableJson["game_name"].is_string()) {
                        table.gameName = tableJson["game_name"].get<std::string>();
                    }
                    if (tableJson.contains("rom_path") && tableJson["rom_path"].is_string()) {
                        table.romPath = tableJson["rom_path"].get<std::string>();
                    }
                    if (tableJson.contains("last_modified") && tableJson["last_modified"].is_string()) {
                        table.lastModified = tableJson["last_modified"].get<std::string>();
                    }

                    if (table.year.empty() && !table.releaseDate.empty()) {
                        std::regex dateRegex_YYYY("\\d{4}");
                        std::regex dateRegex_DDMMYYYY("\\d{2}\\.\\d{2}\\.(\\d{4})");
                        std::smatch match;
                        if (std::regex_search(table.releaseDate, match, dateRegex_DDMMYYYY)) {
                            table.year = match[1].str();
                        } else if (std::regex_search(table.releaseDate, match, dateRegex_YYYY)) {
                            table.year = match[0].str();
                        }
                    }
                    if (table.year.empty() && !table.tableName.empty()) {
                        std::regex yearFromTableNameRegex("\\((\\d{4})\\)");
                        std::smatch match;
                        if (std::regex_search(table.tableName, match, yearFromTableNameRegex)) {
                            table.year = match[1].str();
                        }
                    }
                    if (table.manufacturer.empty() && !table.tableName.empty()) {
                        std::regex manufFromTableNameRegex("\\(([^)]+?)(?:\\s+\\d{4})?\\)");
                        std::smatch match;
                        if (std::regex_search(table.tableName, match, manufFromTableNameRegex)) {
                            table.manufacturer = match[1].str();
                        }
                    }

                    if (vpsLoaded) {
                        vpsClient.enrichTableData(tableJson, table);
                    }
                    break;
                }
            } catch (const nlohmann::json::exception& e) {
                LOG_DEBUG("DataEnricher: JSON parsing error processing table");// with path " << path);// << ": " << e.what());
                continue;
            } catch (const std::exception& e) {
                LOG_DEBUG("DataEnricher: General error processing table");// with path " << path);// << ": " << e.what());
                continue;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("DataEnricher: Failed to parse vpxtool_index.json: " << e.what());
    }
}