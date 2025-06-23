/**
 * @file table_loader.cpp
 * @brief Implements the TableLoader class for loading and managing table data in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableLoader class, which loads table data
 * through a five-stage process: fetching VPSDB (if enabled), scanning VPX files or loading
 * from an ASAP index, matching metadata, saving the index, and sorting tables. It supports
 * progress tracking via LoadingProgress and builds a letter index for navigation. The
 * process is configurable via Settings (e.g., titleSource, sortBy), with potential for
 * future customization via configUI.
 */

#include "table_loader.h"
#include "asap_index_manager.h"
#include "file_scanner.h"
#include "vpin_scanner.h"
#include "vpxtool_scanner.h"
#include "vpsdb/vps_database_client.h"
#include "table_patcher.h"
#include "log/logging.h"
#include <algorithm>
#include <cctype>

// Note: letterIndex is now a non-static member of TableLoader, so it's not initialized here.
// Each instance of TableLoader will have its own letterIndex.

//TODO: use table_data.jsonOwner to decide on incremental updates
std::vector<TableData> TableLoader::loadTableList(const Settings& settings, LoadingProgress* progress) {
    std::vector<TableData> tables;
    AsapIndexManager indexManager(settings);

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Initializing table loading...";
        progress->currentTablesLoaded = 0;
        progress->totalTablesToLoad = 0;
        progress->currentStage = 0;
        progress->numMatched = 0;    // Reset numMatched
        progress->numNoMatch = 0;    // Reset numNoMatch
        progress->logMessages.clear();// Clear log
    }
    // Stage 1: Load from index or scan VPX files
    // -------- ASAP INDEX FOUND - WE'RE DONE HERE ---------
    if (!settings.forceRebuildMetadata && indexManager.load(settings, tables, progress)) {
        LOG_INFO("TableLoader: Loaded " << tables.size() << " tables from asapcab_index.json");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded = tables.size();
            progress->totalTablesToLoad = tables.size();
            progress->currentTask = "Loaded from index";
            progress->currentStage = 1;
        }
    //---------------- NO ASAP INDEX FOUND ------------------
    } else {
        // ------------- FILE SCANNER -------------
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Scanning VPX files...";
            progress->currentStage = 1;
        }
        tables = FileScanner::scan(settings, progress);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->totalTablesToLoad = tables.size();
            progress->currentTask = "Scanning complete";
        }
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Saving metadata to index...";
            progress->currentStage = 1;
        }
        if (!tables.empty()) {
            indexManager.save(settings, tables, progress);
        }
        // ------------- METADATA SCANNERS --------------
        if (settings.titleSource == "metadata") {
            if (settings.useVpxtool) {
                // Stage 2: Attempt to scan with VPXToolScanner if useVpxtool is true
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTask = "Scanning metadata with VPXTool...";
                    progress->currentStage = 2;
                }
                if (!VPXToolScanner::scanFiles(settings, tables, progress)) {
                    // If VPXToolScanner fails (e.g., no vpxtool_index.json or invalid), fall back to VPinScanner
                    LOG_INFO("TableLoader: VPXTool skipped or failed. Proceeding with VPin File Scanner.");
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "Scanning metadata with VPin...";
                        progress->currentStage = 2; // Still stage 2 for metadata scanning
                    }
                    VPinScanner::scanFiles(settings, tables, progress);
                }
            } else {
                // Stage 2: Skip VPXToolScanner and use VPinScanner directly if useVpxtool is false
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTask = "Scanning metadata with VPin...";
                    progress->currentStage = 2;
                }
                LOG_INFO("TableLoader: useVpxtool is false, using VPin File Scanner.");
                VPinScanner::scanFiles(settings, tables, progress);
            }

            // Stage 3: Save asapcab_index.json after metadata matchmaking (from either scanner)
            if (!settings.autoPatchTables) { // Only save if not patching, as patching will save later
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTask = "Saving metadata to index...";
                    progress->currentStage = 3;
                }
                if (!tables.empty()) {
                    indexManager.save(settings, tables, progress);
                }
            }
        }
    }

    // ----------------- AUTO PATCHER -----------------
    if (settings.autoPatchTables) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Patching tables...";
            progress->currentStage = 4;
        }
        TablePatcher patcher;
        patcher.patchTables(settings, tables, progress);
        // Save updated metadata after patching
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Saving updated metadata after patching...";
            progress->currentStage = 4;
        }
        if (!tables.empty()) {
            indexManager.save(settings, tables, progress);
        }
    }

    // -------------- OVERRIDES AND SORTING  -------------
    // Stage 4: Apply per-table overrides
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Applying table overrides...";
        progress->currentStage = 5;
    }
    TableOverrideManager overrideManager;
    for (auto& table : tables) {
        overrideManager.applyOverrides(table);
    }

    // Stage 5: Sorting and Indexing
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting and indexing tables...";
        progress->currentStage = 6;
    }
    
    sortTables(tables, settings.titleSortBy, progress);

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Loading complete";
        progress->currentTablesLoaded = tables.size();
        progress->totalTablesToLoad = tables.size();
        progress->currentStage = 7;
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
    letterIndex.clear(); // Clear the member letterIndex
    for (size_t i = 0; i < tables.size(); ++i) {
        if (tables[i].title.empty()) {
            LOG_DEBUG("TableLoader: Empty title at index " << i);
            continue;
        }
        char firstChar = tables[i].title[0];
        if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = static_cast<int>(i); // Assign index of first occurrence
                // LOG_DEBUG("TableLoader: Added letter index: " << key << " -> " << i);
            }
        } else {
            LOG_DEBUG("TableLoader: Invalid first character in title: " << tables[i].title << " at index " << i);
        }
    }
    if (letterIndex.empty()) {
        LOG_ERROR("TableLoader: Letter index is empty after building");
    }
}