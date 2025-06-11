#include "tables/data_enricher.h"
#include "tables/vpsdb/vps_database_client.h"
#include "tables/asap_index_manager.h"
#include "utils/logging.h"
#include "vpin_wrapper.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <future>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string DataEnricher::cleanString(const std::string& input) {
    std::string result = input;
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return std::iscntrl(c); }), result.end());
    size_t first = result.find_first_not_of(" \t");
    size_t last = result.find_last_not_of(" \t");
    if (first == std::string::npos) return "";
    return result.substr(first, last - first + 1);
}

std::string DataEnricher::safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue) {
    if (j.contains(key)) {
        if (j[key].is_string()) {
            return j[key].get<std::string>();
        } else if (j[key].is_number()) {
            return std::to_string(j[key].get<double>());
        } else if (j[key].is_null()) {
            return defaultValue;
        } else {
            LOG_DEBUG("Field " << key << " is not a string, number, or null, type: " << j[key].type_name());
            return defaultValue;
        }
    }
    return defaultValue;
}

void DataEnricher::enrich(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    std::string jsonPath = settings.VPXTablesPath + settings.vpxtoolIndex;
    bool vpxtoolLoaded = false;
    json vpxtoolJson;

    LOG_DEBUG("DataEnricher: Starting enrich with titleSource=" << settings.titleSource << ", fetchVPSdb=" << settings.fetchVPSdb << ", vpxtoolIndex=" << jsonPath);

    // Try loading vpxtool_index.json if it exists
    if (settings.titleSource == "metadata" && fs::exists(jsonPath)) {
        try {
            LOG_DEBUG("DataEnricher: Attempting to load vpxtool_index.json from: " << jsonPath);
            std::ifstream file(jsonPath);
            file >> vpxtoolJson;
            file.close();
            if (vpxtoolJson.contains("tables") && vpxtoolJson["tables"].is_array()) {
                vpxtoolLoaded = true;
                LOG_INFO("DataEnricher: Loaded vpxtool_index.json from: " << jsonPath);
            } else {
                LOG_ERROR("DataEnricher: Invalid vpxtool_index.json: 'tables' missing or not an array");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("DataEnricher: Failed to parse vpxtool_index.json: " << e.what());
        }
    }

    // Initialize progress
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = vpxtoolLoaded ? "Processing vpxtool_index.json..." : "Scanning VPX files with vpin...";
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
        progress->logMessages.push_back("DEBUG: Initialized enrichment for " + std::to_string(tables.size()) + " tables");
    }

    // Load VPSDB if enabled
    VpsDatabaseClient vpsClient(settings.vpsDbPath);
    bool vpsLoaded = false;
    if (settings.fetchVPSdb && vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress) && vpsClient.load(progress)) {
        vpsLoaded = true;
        LOG_INFO("DataEnricher: VPSDB loaded successfully");
    } else if (settings.fetchVPSdb) {
        LOG_ERROR("DataEnricher: Failed to load vpsdb.json");
    }

    // Process tables using vpxtool_index.json if loaded
    if (vpxtoolLoaded) {
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
                    table.title = table.tableName.empty() ? cleanString(filename) : table.tableName;

                    LOG_DEBUG("DataEnricher: vpxtool table before VPSDB: path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                    if (vpsLoaded) {
                        vpsClient.enrichTableData(tableJson, table, progress);
                    }

                    LOG_DEBUG("DataEnricher: vpxtool table after VPSDB: path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);
                    break;
                }
                processed++;
                if (progress && !found) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
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
    } else if (settings.titleSource == "metadata") {
        // Use vpin to scan VPX files
        std::vector<std::future<void>> futures;
        std::atomic<int> processed(0);
        const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency() / 2);

        for (auto& table : tables) {
            while (futures.size() >= maxThreads) {
                for (auto it = futures.begin(); it != futures.end();) {
                    if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                        try {
                            it->get();
                        } catch (const std::exception& e) {
                            LOG_ERROR("DataEnricher: Thread exception for " << table.vpxFile << ": " << e.what());
                        }
                        it = futures.erase(it);
                    } else {
                        ++it;
                    }
                }
                std::this_thread::yield();
            }

            futures.push_back(std::async(std::launch::async, [&table, progress, &processed, &tables]() {
                std::string vpxFile = table.vpxFile;
                LOG_DEBUG("DataEnricher: Processing VPX file with vpin: " << vpxFile);
                char* json_result = get_vpx_table_info_as_json(vpxFile.c_str());
                if (!json_result) {
                    LOG_ERROR("DataEnricher: Failed to get metadata for " << vpxFile);
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numNoMatch++;
                        progress->logMessages.push_back("DEBUG: Failed to process: " + vpxFile);
                    }
                    return;
                }

                try {
                    std::string json_str(json_result);
                    json vpinJson = json::parse(json_str);

                    table.tableName = cleanString(safeGetString(vpinJson, "table_name", table.title));
                    table.authorName = cleanString(safeGetString(vpinJson, "author_name", ""));
                    table.tableDescription = cleanString(safeGetString(vpinJson, "table_description", ""));
                    table.tableSaveDate = safeGetString(vpinJson, "table_save_date", "");
                    table.releaseDate = safeGetString(vpinJson, "release_date", "");
                    table.tableVersion = cleanString(safeGetString(vpinJson, "table_version", ""));
                    table.tableRevision = safeGetString(vpinJson, "table_save_rev", "");
                    table.title = table.tableName.empty() ? cleanString(fs::path(vpxFile).stem().string()) : table.tableName;

                    if (vpinJson.contains("properties") && vpinJson["properties"].is_object()) {
                        table.manufacturer = cleanString(safeGetString(vpinJson["properties"], "manufacturer", ""));
                        table.year = cleanString(safeGetString(vpinJson["properties"], "year", ""));
                    }

                    LOG_DEBUG("DataEnricher: vpin table after processing: path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numMatched++;
                        progress->logMessages.push_back("DEBUG: Processed: " + vpxFile);
                    }
                } catch (const json::exception& e) {
                    LOG_ERROR("DataEnricher: JSON parsing error for " << vpxFile << ": " << e.what());
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numNoMatch++;
                        progress->logMessages.push_back("DEBUG: JSON error: " + vpxFile);
                    }
                } catch (...) {
                    LOG_ERROR("DataEnricher: Unexpected error processing " << vpxFile);
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->numNoMatch++;
                        progress->logMessages.push_back("DEBUG: Unexpected error: " + vpxFile);
                    }
                }

                free_rust_string(json_result);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTablesLoaded = ++processed;
                    progress->currentTask = "Processed " + std::to_string(processed) + " of " + std::to_string(tables.size()) + " tables";
                }
            }));
        }

        // Wait for all vpin threads to complete
        for (auto& future : futures) {
            try {
                future.get();
            } catch (const std::exception& e) {
                LOG_ERROR("DataEnricher: Thread exception: " << e.what());
            }
        }

        // Save asapcab_index.json after vpin processing
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Saving vpin metadata to index...";
            progress->logMessages.push_back("DEBUG: Saving vpin metadata to asapcab_index.json");
        }
        LOG_DEBUG("DataEnricher: Saving asapcab_index.json after vpin for " << tables.size() << " tables");
        AsapIndexManager::save(settings, tables, progress);

        // Update progress before VPSDB enrichment
        if (progress && vpsLoaded) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Matching tables to VPSDB...";
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = tables.size();
            progress->numMatched = 0;
            progress->numNoMatch = 0;
            progress->logMessages.push_back("DEBUG: Starting VPSDB enrichment for " + std::to_string(tables.size()) + " tables");
        }

        // Enrich with VPSDB if loaded, using parallel processing
        if (vpsLoaded) {
            const size_t maxThreadsVps = std::max(1u, std::thread::hardware_concurrency() / 2);
            std::vector<std::future<void>> vpsFutures;
            std::atomic<int> processedVps(0);

            for (auto& table : tables) {
                while (vpsFutures.size() >= maxThreadsVps) {
                    for (auto it = vpsFutures.begin(); it != vpsFutures.end();) {
                        if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                            try {
                                it->get();
                            } catch (const std::exception& e) {
                                LOG_ERROR("DataEnricher: VPSDB thread exception for " << table.vpxFile << ": " << e.what());
                            }
                            it = vpsFutures.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    std::this_thread::yield();
                }

                vpsFutures.push_back(std::async(std::launch::async, [&table, &vpsClient, progress, &processedVps, &tables]() {
                    json tableJson;
                    tableJson["path"] = table.vpxFile;
                    tableJson["table_info"] = {
                        {"table_name", table.tableName},
                        {"author_name", table.authorName},
                        {"table_description", table.tableDescription},
                        {"table_version", table.tableVersion},
                        {"table_save_date", table.tableSaveDate},
                        {"release_date", table.releaseDate},
                        {"table_save_rev", table.tableRevision}
                    };
                    tableJson["game_name"] = table.gameName;
                    tableJson["rom_path"] = table.romPath;
                    tableJson["properties"] = {
                        {"manufacturer", table.manufacturer},
                        {"year", table.year}
                    };

                    LOG_DEBUG("DataEnricher: vpin table before VPSDB: path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                    vpsClient.enrichTableData(tableJson, table, progress);

                    LOG_DEBUG("DataEnricher: vpin table after VPSDB: path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTablesLoaded = ++processedVps;
                        progress->currentTask = "Matched " + std::to_string(processedVps) + " of " + std::to_string(tables.size()) + " tables to VPSDB";
                    }
                }));
            }

            // Wait for all VPSDB threads to complete
            for (auto& future : vpsFutures) {
                try {
                    future.get();
                } catch (const std::exception& e) {
                    LOG_ERROR("DataEnricher: VPSDB thread exception: " << e.what());
                }
            }
        }
    }
}