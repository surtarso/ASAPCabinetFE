/**
 * @file table_loader.cpp
 * @brief Implements the TableLoader class for loading and managing table data in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableLoader class, which loads table data
 * through a five-stage process: fetching VPSDB (if enabled), scanning VPX files or loading
 * from an ASAP index, enriching metadata, saving the index, and sorting tables. It supports
 * progress tracking via LoadingProgress and builds a letter index for navigation. The
 * process is configurable via Settings (e.g., titleSource, sortBy), with potential for
 * future customization via configUI.
 */

#include "table_loader.h"
#include "asap_index_manager.h"
#include "file_scanner.h"
#include "vpin_scanner.h" // Renamed from vpin_scanner.h
#include "vpxtool_scanner.h"   // New scanner
#include "vpsdb/vps_database_client.h" // Still needed for VPSDB client within scanners
#include "log/logging.h"
#include <algorithm>
#include <cctype>

// Note: letterIndex is now a non-static member of TableLoader, so it's not initialized here.
// Each instance of TableLoader will have its own letterIndex.

std::vector<TableData> TableLoader::loadTableList(const Settings& settings, LoadingProgress* progress) {
    std::vector<TableData> tables;

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
    if (!settings.forceRebuildMetadata && AsapIndexManager::load(settings, tables, progress)) {
        LOG_INFO("TableLoader: Loaded " << tables.size() << " tables from ASAP index");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded = tables.size();
            progress->totalTablesToLoad = tables.size();
            progress->currentTask = "Loaded from index";
            progress->currentStage = 1;
        }
    } else {
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

        if (settings.titleSource == "metadata") {
            // Stage 2: Attempt to scan with VPXToolScanner first
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Scanning metadata with VPXTool...";
                progress->currentStage = 2;
            }
            // If VPXToolScanner successfully loaded and processed vpxtool_index.json,
            // it will have updated the tables. We don't need to call VPinFileScanner.
            if (!VPXToolScanner::scanFiles(settings, tables, progress)) {
                // If VPXToolScanner skipped (e.g., no vpxtool_index.json or invalid),
                // then proceed with VPinFileScanner.
                LOG_INFO("TableLoader: VPXToolScanner skipped or failed. Proceeding with VPinFileScanner.");
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTask = "Scanning metadata with VPin...";
                    progress->currentStage = 2; // Still stage 2 for metadata scanning
                }
                VPinScanner::scanFiles(settings, tables, progress);
            }

            // Stage 3: Save asapcab_index.json after metadata enrichment (from either scanner)
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Saving metadata to index...";
                progress->currentStage = 3;
            }
            AsapIndexManager::save(settings, tables, progress);
        }
    }

    // Stage 4: Apply per-table overrides
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Applying table overrides...";
        progress->currentStage = 4;
    }
    TableOverrideManager overrideManager;
    for (auto& table : tables) {
        overrideManager.applyOverrides(table);
    }

    // Stage 5: Sorting and Indexing
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting and indexing tables...";
        progress->currentStage = 4;
    }
    
    sortTables(tables, settings.titleSortBy, progress);

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Loading complete";
        progress->currentTablesLoaded = tables.size();
        progress->totalTablesToLoad = tables.size();
        progress->currentStage = 5;
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
                LOG_DEBUG("TableLoader: Added letter index: " << key << " -> " << i);
            }
        } else {
            LOG_DEBUG("TableLoader: Invalid first character in title: " << tables[i].title << " at index " << i);
        }
    }
    if (letterIndex.empty()) {
        LOG_ERROR("TableLoader: Letter index is empty after building");
    }
}