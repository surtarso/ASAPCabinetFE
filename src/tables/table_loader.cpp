/**
 * @file table_loader.cpp
 * @brief Implements the TableLoader class for loading and managing table data in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableLoader class, which loads table data
 * through a multi-stage process: scanning VPX files, merging with existing index, matching
 * metadata (if enabled), fetching VPSDB (if enabled), saving the index, patching tables,
 * applying overrides, and sorting. It supports incremental updates using
 * AsapIndexManager::mergeTables and progress tracking via LoadingProgress. The process is
 * configurable via Settings (e.g., titleSource, sortBy).
 */

#include "table_loader.h"
#include "asap_index_manager.h"
#include "file_scanner.h"
#include "vpin_scanner.h"
#include "vpxtool_scanner.h"
#include "vpinmdb/vpinmdb_client.h"
#include "vpsdb/vps_database_client.h"
#include "table_patcher.h"
#include "log/logging.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>
#include <future>

namespace fs = std::filesystem;

std::vector<TableData> TableLoader::loadTableList(const Settings& settings, LoadingProgress* progress) {
    std::vector<TableData> tables, scannedTables;
    AsapIndexManager indexManager(settings);

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Initializing table loading...";
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = 0;
        progress->currentStage = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
        progress->logMessages.clear();
    }

    // Stage 1: Load existing index (unless forceRebuildMetadata)
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Loading index...";
        progress->currentStage = 1;
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
    }
    if (!settings.forceRebuildMetadata && indexManager.load(settings, tables, progress)) {
        LOG_INFO("TableLoader: Loaded " << tables.size() << " tables from asapcab_index.json");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Loaded from index";
            progress->currentTablesLoaded = tables.size();
            progress->totalTablesToLoad = tables.size();
        }
    } else {
        LOG_INFO("TableLoader: No index loaded (forceRebuildMetadata=" << settings.forceRebuildMetadata << ")");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Index loading skipped";
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = 0;
        }
    }

    // Stage 2: Scan VPX files
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Scanning VPX files...";
        progress->currentStage = 2;
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
    }
    scannedTables = FileScanner::scan(settings, progress, settings.forceRebuildMetadata ? nullptr : &tables);
    if (settings.forceRebuildMetadata) {
        // Reset user fields for full rebuild
        for (auto& table : scannedTables) {
            table.playCount = 0;
            table.playTimeLast = 0.0f;
            table.playTimeTotal = 0.0f;
            table.isBroken = false;
        }
    }
    if (!progress) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->totalTablesToLoad = scannedTables.size();
            progress->currentTablesLoaded = scannedTables.size();
            progress->currentTask = "Scanning complete";
        }
    }

    // Stage 3: Merge scanned tables with existing index
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Merging tables with index...";
        progress->currentStage = 3;
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
    }
    tables = indexManager.mergeTables(settings, scannedTables, progress);
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = tables.size();
        progress->currentTask = "Merge complete";
    }

    // Stage 4: Metadata scanning for new/updated tables (only if titleSource=metadata)
    if (settings.titleSource == "metadata") {
        std::vector<TableData> tablesToScan;
        std::unordered_map<std::string, TableData> scannedTableMap;
        for (const auto& newTable : scannedTables) {
            if (!newTable.vpxFile.empty()) {
                scannedTableMap[newTable.vpxFile] = newTable;
            }
        }

        for (auto& table : tables) {
            if (table.jsonOwner == "System File Scan") {
                tablesToScan.push_back(table);
            } else {
                auto it = scannedTableMap.find(table.vpxFile);
                if (it != scannedTableMap.end() && it->second.fileLastModified > table.fileLastModified) {
                    tablesToScan.push_back(table);
                }
            }
        }

        if (!tablesToScan.empty()) {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = settings.useVpxtool ? "Scanning metadata with VPXTool..." : "Scanning metadata with VPin...";
                progress->currentStage = 4;
                progress->totalTablesToLoad = tablesToScan.size();
                progress->currentTablesLoaded = 0;
                progress->numMatched = 0;
                progress->numNoMatch = 0;
            }
            if (settings.useVpxtool) {
                if (!VPXToolScanner::scanFiles(settings, tablesToScan, progress)) {
                    LOG_INFO("TableLoader: VPXTool skipped or failed. Proceeding with VPin File Scanner.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "Scanning metadata with VPin...";
                    }
                    VPinScanner::scanFiles(tablesToScan, progress);
                }
            } else {
                LOG_INFO("TableLoader: useVpxtool is false, using VPin File Scanner.");
                VPinScanner::scanFiles(tablesToScan, progress);
            }

            // Update tables with scanned metadata
            scannedTableMap.clear();
            for (const auto& scannedTable : tablesToScan) {
                if (!scannedTable.vpxFile.empty()) {
                    scannedTableMap[scannedTable.vpxFile] = scannedTable;
                }
            }
            for (auto& table : tables) {
                auto it = scannedTableMap.find(table.vpxFile);
                if (it != scannedTableMap.end()) {
                    table = it->second;
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTablesLoaded++;
                    }
                }
            }
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Metadata scanning complete";
                // Do not advance currentStage here; wait until Stage 5 starts
            }
        } else {
            LOG_INFO("TableLoader: No tables need metadata scanning.");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Metadata scanning skipped";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
                progress->numMatched = 0;
                progress->numNoMatch = 0;
                // Do not advance currentStage here; wait until Stage 5 starts
            }
        }
    }

    // Stage 5: VPSDB scanning for tables without VPSDB data
    if (settings.fetchVPSdb) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Preparing VPSDB...";
            progress->currentStage = 5;
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = 0;
            progress->numMatched = 0;
            progress->numNoMatch = 0;
        }
        VpsDatabaseClient vpsClient(settings.vpsDbPath);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Checking for VPSDB updates...";
        }
        bool fetchSuccess = vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress);
        if (fetchSuccess) {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Loading VPSDB into memory...";
            }
            if (vpsClient.load(progress)) {
                std::vector<TableData> tablesForVpsdb;
                std::unordered_map<std::string, TableData> scannedTableMap;
                for (const auto& newTable : scannedTables) {
                    if (!newTable.vpxFile.empty()) {
                        scannedTableMap[newTable.vpxFile] = newTable;
                    }
                }

                for (auto& table : tables) {
                    if (table.vpsId.empty() || table.jsonOwner == "System File Scan" || 
                        (scannedTableMap.find(table.vpxFile) != scannedTableMap.end() && 
                         scannedTableMap[table.vpxFile].fileLastModified > table.fileLastModified)) {
                        tablesForVpsdb.push_back(table);
                    }
                }
                LOG_INFO("TableLoader: Processing " << tablesForVpsdb.size() << " tables for VPSDB matching");

                if (!tablesForVpsdb.empty()) {
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "Matching tables to VPSDB...";
                        progress->totalTablesToLoad = tablesForVpsdb.size();
                        progress->currentTablesLoaded = 0;
                        progress->numMatched = 0;
                        progress->numNoMatch = 0;
                    }
                    std::vector<std::future<void>> futures;
                    std::atomic<int> processedVps(0);
                    const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency());

                    for (auto& table : tablesForVpsdb) {
                        while (futures.size() >= maxThreads) {
                            for (auto it = futures.begin(); it != futures.end();) {
                                if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                                    it->get();
                                    it = futures.erase(it);
                                } else {
                                    ++it;
                                }
                            }
                            std::this_thread::yield();
                        }

                        futures.push_back(std::async(std::launch::async, [&table, &vpsClient, progress, &processedVps]() {
                            nlohmann::json tempJson;
                            tempJson["path"] = table.vpxFile;
                            tempJson["rom"] = table.romName;
                            tempJson["table_info"] = {
                                {"table_name", table.tableName},
                                {"author_name", table.tableAuthor},
                                {"table_description", table.tableDescription},
                                {"table_version", table.tableVersion},
                                {"table_save_date", table.tableSaveDate},
                                {"release_date", table.tableReleaseDate},
                                {"table_save_rev", table.tableRevision},
                                {"table_blurb", table.tableBlurb},
                                {"table_rules", table.tableRules},
                                {"author_email", table.tableAuthorEmail},
                                {"author_website", table.tableAuthorWebsite}
                            };
                            tempJson["properties"] = {
                                {"manufacturer", table.tableManufacturer},
                                {"year", table.tableYear},
                                {"TableType", table.tableType}
                            };
                            tempJson["filename_title"] = table.title;
                            tempJson["filename_manufacturer"] = table.manufacturer;
                            tempJson["filename_year"] = table.year;

                            vpsClient.matchMetadata(tempJson, table, progress);
                            if (!table.vpsId.empty()) {
                                table.jsonOwner = "Virtual Pinball Spreadsheet Database";
                            }

                            if (progress) {
                                std::lock_guard<std::mutex> lock(progress->mutex);
                                progress->currentTablesLoaded = ++processedVps;
                                progress->currentTask = "Matching " + std::to_string(processedVps) + " of " + std::to_string(progress->totalTablesToLoad) + " tables to VPSDB";
                            }
                        }));
                    }

                    // Wait for all futures to complete
                    for (auto& future : futures) {
                        try {
                            future.get();
                        } catch (const std::exception& e) {
                            LOG_ERROR("TableLoader: VPSDB thread exception: " << e.what());
                        }
                    }

                    // Update tables and count matches in a single-threaded loop
                    std::unordered_map<std::string, TableData> vpsdbTableMap;
                    int numMatched = 0, numNoMatch = 0;
                    for (const auto& vpsTable : tablesForVpsdb) {
                        if (!vpsTable.vpxFile.empty()) {
                            vpsdbTableMap[vpsTable.vpxFile] = vpsTable;
                            if (!vpsTable.vpsId.empty()) {
                                numMatched++;
                            } else {
                                numNoMatch++;
                            }
                        }
                    }
                    for (auto& table : tables) {
                        auto it = vpsdbTableMap.find(table.vpxFile);
                        if (it != vpsdbTableMap.end()) {
                            table = it->second;
                        }
                    }

                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "VPSDB matching complete";
                        progress->numMatched = numMatched;
                        progress->numNoMatch = numNoMatch;
                    }
                } else {
                    LOG_INFO("TableLoader: No tables need VPSDB scanning.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "VPSDB matching skipped";
                        progress->currentTablesLoaded = 0;
                        progress->totalTablesToLoad = 0;
                        progress->numMatched = 0;
                        progress->numNoMatch = 0;
                    }
                }
            } else {
                LOG_ERROR("TableLoader: Failed to load VPSDB, skipping VPSDB matching.");
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTask = "VPSDB loading failed";
                    progress->currentTablesLoaded = 0;
                    progress->totalTablesToLoad = 0;
                    progress->numMatched = 0;
                    progress->numNoMatch = 0;
                }
            }
        } else {
            LOG_ERROR("TableLoader: Failed to fetch VPSDB, skipping VPSDB matching.");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "VPSDB fetch failed";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
                progress->numMatched = 0;
                progress->numNoMatch = 0;
            }
        }
    }

    // Stage 6: Download media
    if (settings.fetchVpinMediaDb) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Downloading table media...";
            progress->currentStage = 6;
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = tables.size();
            progress->numMatched = 0;
            progress->numNoMatch = 0;
        }
        VpinMdbClient downloader(settings, progress);
        bool downloaded = downloader.downloadMedia(tables);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = downloaded ? "Media downloading complete" : "No media downloaded";
            progress->currentTablesLoaded = tables.size(); // All tables processed
        }
    }

    // Stage 7: Save updated index after metadata and VPSDB scanning
    if (!settings.autoPatchTables) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Saving metadata to index...";
            progress->currentStage = 7;
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = tables.empty() ? 0 : tables.size();
            progress->numMatched = 0;
            progress->numNoMatch = 0;
        }
        if (!tables.empty()) {
            indexManager.save(settings, tables, progress);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Index saving complete";
                progress->currentTablesLoaded = tables.size();
                // Do not advance currentStage here; wait until Stage 7 starts
            }
        } else {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Index saving skipped";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
                // Do not advance currentStage here; wait until Stage 7 starts
            }
        }
    }

    // Stage 8: Auto patching
    if (settings.autoPatchTables) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Patching tables...";
            progress->currentStage = 8;
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = tables.empty() ? 0 : tables.size();
            progress->numMatched = 0;
            progress->numNoMatch = 0;
        }
        TablePatcher patcher;
        patcher.patchTables(settings, tables, progress);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Saving updated metadata after patching...";
            progress->currentTablesLoaded = tables.size();
        }
        if (!tables.empty()) {
            indexManager.save(settings, tables, progress);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Index saving complete";
                progress->currentTablesLoaded = tables.size();
                // Do not advance currentStage here; wait until Stage 8 starts
            }
        } else {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Index saving skipped";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
                // Do not advance currentStage here; wait until Stage 8 starts
            }
        }
    }

    // Stage 9: Apply per-table overrides
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Applying table overrides...";
        progress->currentStage = 9;
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = tables.empty() ? 0 : tables.size();
        progress->numMatched = 0;
        progress->numNoMatch = 0;
    }
    TableOverrideManager overrideManager;
    for (auto& table : tables) {
        overrideManager.applyOverrides(table);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded++;
        }
    }
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Overrides applied";
        // Do not advance currentStage here; wait until Stage 9 starts
    }

    // Stage 10: Sorting and Indexing
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting and indexing tables...";
        progress->currentStage = 10;
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = tables.empty() ? 0 : tables.size();
        progress->numMatched = 0;
        progress->numNoMatch = 0;
    }
    sortTables(tables, settings.titleSortBy, progress);
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting complete";
        progress->currentTablesLoaded = tables.size();
        // Do not advance currentStage here; wait until final step
    }

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Loading complete";
        progress->currentTablesLoaded = tables.size();
        progress->totalTablesToLoad = tables.size();
        progress->currentStage = 11;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
    }
    return tables;
}

void TableLoader::sortTables(std::vector<TableData>& tables, const std::string& sortBy, LoadingProgress* progress) {
    if (tables.empty()) {
        LOG_DEBUG("TableLoader: No tables to sort");
        return;
    }

    // Sort based on the selected criterion
    if (sortBy == "author") {
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            std::string aAuthor = a.vpsAuthors.empty() ? a.tableAuthor : a.vpsAuthors;
            std::string bAuthor = b.vpsAuthors.empty() ? b.tableAuthor : b.vpsAuthors;
            return aAuthor < bAuthor;
        });
    } else if (sortBy == "type") {
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            return a.vpsType < b.vpsType;
        });
    } else if (sortBy == "manufacturer") {
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            return a.manufacturer < b.manufacturer;
        });
    } else if (sortBy == "year") {
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            return a.year > b.year; // Descending order for year
        });
    } else { // Default to "title"
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            return a.title < b.title;
        });
    }

    // Rebuild letter index after sorting (always based on title)
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Building letter index...";
    }
    letterIndex.clear();
    for (size_t i = 0; i < tables.size(); ++i) {
        if (tables[i].title.empty()) {
            LOG_DEBUG("TableLoader: Empty title at index " << i);
            continue;
        }
        char firstChar = tables[i].title[0];
        if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = static_cast<int>(i);
            }
        } else {
            LOG_DEBUG("TableLoader: Invalid first character in title: " << tables[i].title << " at index " << i);
        }
    }
    if (letterIndex.empty()) {
        LOG_ERROR("TableLoader: Letter index is empty after building");
    }
}