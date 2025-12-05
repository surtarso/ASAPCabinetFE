/**
 * @file table_loader.cpp
 * @brief Implements the TableLoader class for loading and managing table data in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableLoader class, which loads table data
 * through a multi-stage process: scanning VPX files, merging with existing index, matching
 * metadata (if enabled), fetching VPSDB (if enabled), saving the index, patching tables,
 * applying overrides, and sorting. It supports incremental updates using
 * AsapIndexManager::mergeTables and progress tracking via LoadingProgress. The process is
 * configurable via Settings (e.g., titleSource, sortBy). If settings.ignoreScanners is true,
 * it skips all scanning and processing, loading only from asapcab_index.json for faster startup.
 */

#include "table_loader.h"
#include "file_scanner.h"
#include "vpin_scanner.h"
#include "vpxtool_scanner.h"
#include "table_patcher.h"
#include "vpinmdb/vpinmdb_scanner.h"
#include "launchboxdb/lbdb_scanner.h"
#include "data/asapcab/asapcab_index_manager.h"
#include "data/vpsdb/vps_database_client.h"
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

    // Fast startup: Load only from index if ignoreScanners is true and tables index is not empty/missing
    bool fastPathFailed = false;
    if (settings.ignoreScanners) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Loading from index only...";
            progress->currentStage = 1;
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = 0;
        }
        if (indexManager.load(settings, tables, progress)) {
            LOG_INFO("Fast Startup loaded " + std::to_string(tables.size()) + " tables from asapcab_index.json");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Loaded from index";
                progress->currentTablesLoaded = tables.size();
                progress->totalTablesToLoad = tables.size();
            }
        } else {
            LOG_WARN("Failed to load asapcab_index.json, scanning files...");
            fastPathFailed = true; // continue to full path
        }

        if (fastPathFailed) {
            LOG_INFO("Proceeding with file scan due to missing index.");
        } else {
            // Stage 10: Sorting and Indexing
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Sorting and indexing tables...";
                progress->currentStage = 10;
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = tables.empty() ? 0 : tables.size();
            }
            sortTables(tables, settings.titleSortBy, progress);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Loading complete";
                progress->currentTablesLoaded = tables.size();
                progress->totalTablesToLoad = tables.size();
                progress->currentStage = 11;
            }
            return tables;
        }
    }

    // Full path: Proceed with all stages
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
        LOG_INFO("Loaded " + std::to_string(tables.size()) + " tables from asapcab_index.json");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Loaded from index";
            progress->currentTablesLoaded = tables.size();
            progress->totalTablesToLoad = tables.size();
        }
    } else {
        LOG_INFO("No index loaded (forceRebuildMetadata=" + std::to_string(settings.forceRebuildMetadata) + ")");
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
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->totalTablesToLoad = scannedTables.size();
        progress->currentTablesLoaded = scannedTables.size();
        progress->currentTask = "Scanning complete";
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
                    LOG_INFO("VPXTool skipped or failed. Proceeding with VPin File Scanner.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "Scanning metadata with VPin...";
                    }
                    VPinScanner::scanFiles(tablesToScan, progress);
                }
            } else {
                LOG_INFO("useVpxtool is false, using VPin File Scanner.");
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
            }
        } else {
            LOG_INFO("No tables need metadata scanning.");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Metadata scanning skipped";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
                progress->numMatched = 0;
                progress->numNoMatch = 0;
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
        VpsDatabaseClient vpsdbClient(settings);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Checking for VPSDB updates...";
        }
        bool fetchSuccess = vpsdbClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress);
        if (fetchSuccess) {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Loading VPSDB into memory...";
            }
            if (vpsdbClient.load(progress)) {
                std::vector<TableData> tablesForVpsdb;
                std::unordered_map<std::string, TableData> scannedTableMap;
                for (const auto& newTable : scannedTables) {
                    if (!newTable.vpxFile.empty()) {
                        scannedTableMap[newTable.vpxFile] = newTable;
                    }
                }

                // consider vpsID if added by override_manager.
                for (auto& table : tables) {
                    if (settings.forceRebuildMetadata && !table.isManualVpsId) {
                        table.vpsId.clear();
                        table.jsonOwner.clear();
                        table.matchConfidence = 0.0f;
                    }
                    tablesForVpsdb.push_back(table);
                }

                LOG_INFO("Processing " + std::to_string(tablesForVpsdb.size()) + " tables for VPSDB matching");

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

                        futures.push_back(std::async(std::launch::async, [&table, &vpsdbClient, progress, &processedVps, &settings]() {
                            nlohmann::json tempJson;
                            // file scan fileds:
                            tempJson["path"] = table.vpxFile;   // Full path to the vpx table file
                            tempJson["rom"] = table.romName;    // actual rom file from user (user could have wrong rom...)
                            // we need to extract 'expected rom' from table script to validate this (tho script could be wrong too lol)
                            // Table file metadata fields:
                            tempJson["table_info"] = {
                                {"table_name", table.tableName},
                                {"author_name", table.tableAuthor},
                                {"table_description", table.tableDescription}, // big text
                                {"table_version", table.tableVersion},         // many formats
                                {"table_save_date", table.tableSaveDate},      // many formats
                                {"release_date", table.tableReleaseDate},      // many formats
                                {"table_save_rev", table.tableRevision},
                                {"table_blurb", table.tableBlurb},             // big text
                                {"table_rules", table.tableRules},             // big text
                                {"author_email", table.tableAuthorEmail},
                                {"author_website", table.tableAuthorWebsite}
                            };
                            tempJson["properties"] = {
                                {"manufacturer", table.tableManufacturer},
                                {"year", table.tableYear},      // many formats
                                {"TableType", table.tableType}  // EM, SS.. Eletro Mechanical, eletromechanical...
                            };
                            // file heuristics fields:
                            tempJson["filename_title"] = table.bestTitle;
                            tempJson["filename_manufacturer"] = table.bestManufacturer; // extracted from filename using manufacturers.h list
                            tempJson["filename_year"] = table.bestYear;

                            // send to vpsdb using good fields only
                            vpsdbClient.matchMetadata(tempJson, table, progress);
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
                            LOG_ERROR("VPSDB thread exception: " + std::string(e.what()));
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
                    LOG_INFO("No tables need VPSDB scanning.");
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
                LOG_ERROR("Failed to load VPSDB, skipping VPSDB matching.");
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
            LOG_ERROR("Failed to fetch VPSDB, skipping VPSDB matching.");
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

    // Stage 6: Media download
    if (settings.fetchMediaOnline) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Downloading table media...";
            progress->currentStage = 6;
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = tables.size();
            progress->numMatched = 0;
            progress->numNoMatch = 0;
        }

        // NON PARALLEL VERSION:
        // TODO: for now keep those, but later integrate with the new database in one pass
        {
            VpinMdbScanner vpdownloader(settings, progress);
            vpdownloader.scanForMedia(tables);
        }
        {
            LbdbScanner lbdownloader(settings, progress);
            lbdownloader.scanForMedia(tables);
        }

        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "All media downloaded";
            progress->currentTablesLoaded = tables.size();
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
            }
        } else {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Index saving skipped";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
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
            }
        } else {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Index saving skipped";
                progress->currentTablesLoaded = 0;
                progress->totalTablesToLoad = 0;
            }
        }
    }

    // Stage 9: Apply per-table overrides for sorting (vpsdbID was already used and will remain)
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
        progress->currentTask = "Loading complete";
        progress->currentTablesLoaded = tables.size();
        progress->totalTablesToLoad = tables.size();
        progress->currentStage = 11;
    }
    return tables;
}

void TableLoader::sortTables(std::vector<TableData>& tables, const std::string& sortBy, LoadingProgress* progress) {
    if (tables.empty()) {
        LOG_DEBUG("No tables to sort");
        return;
    }

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting tables...";
    }

    // === YOUR ORIGINAL SORT — JUST EFFICIENT ===
    if (sortBy == "author") {
        std::sort(tables.begin(), tables.end(), [](const auto& a, const auto& b) {
            auto getAuthor = [](const auto& t) { return t.vpsAuthors.empty() ? t.tableAuthor : t.vpsAuthors; };
            return getAuthor(a) < getAuthor(b);
        });
    } else if (sortBy == "type") {
        std::sort(tables.begin(), tables.end(), [](const auto& a, const auto& b) {
            return a.vpsType < b.vpsType;
        });
    } else if (sortBy == "manufacturer") {
        std::sort(tables.begin(), tables.end(), [](const auto& a, const auto& b) {
            return a.bestManufacturer < b.bestManufacturer;
        });
    } else if (sortBy == "year") {
        std::sort(tables.begin(), tables.end(), [](const auto& a, const auto& b) {
            return a.bestYear > b.bestYear;
        });
    } else {
        std::sort(tables.begin(), tables.end(), [](const auto& a, const auto& b) {
            return a.bestTitle < b.bestTitle;
        });
    }

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting complete";
    }
}
