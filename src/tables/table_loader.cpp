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

#include "tables/table_loader.h"
#include "tables/asap_index_manager.h"
#include "tables/file_scanner.h"
#include "tables/vpin_scanner.h"
#include "tables/vpsdb/vps_database_client.h"
#include "utils/logging.h"
#include <algorithm>
#include <cctype>

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

    // Stage 1: Fetching VPSDB (if needed)
    if (settings.titleSource == "metadata" && settings.fetchVPSdb) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Fetching VPSDB...";
            progress->currentStage = 1;
        }
        VpsDatabaseClient vpsClient(settings.vpsDbPath);
        if (vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress) && vpsClient.load(progress)) {
            LOG_INFO("TableLoader: VPSDB fetched and loaded successfully");
        } else {
            LOG_ERROR("TableLoader: Failed to fetch/load VPSDB, proceeding without it");
        }
    } else {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Skipping VPSDB fetch...";
            progress->currentStage = 1;
        }
    }

    // Stage 2: Load from index or scan VPX files
    if (settings.titleSource == "metadata" && !settings.forceRebuildMetadata && AsapIndexManager::load(settings, tables, progress)) {
        LOG_INFO("TableLoader: Loaded " << tables.size() << " tables from ASAP index");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded = tables.size();
            progress->totalTablesToLoad = tables.size();
            progress->currentTask = "Loaded from index";
            progress->currentStage = 2;
        }
    } else {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Scanning VPX files...";
            progress->currentStage = 2;
        }
        tables = FileScanner::scan(settings, progress);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->totalTablesToLoad = tables.size();
            progress->currentTask = "Scanning complete";
        }
        if (settings.titleSource == "metadata") {
            // Stage 3: Enrich metadata and Stage 4: Save index (only when scanning)
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Enriching data...";
                progress->currentStage = 3;
            }
            VPinScanner::enrich(settings, tables, progress);
            if (progress) {
            // Stage 4: Save index
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Saving index...";
                progress->currentStage = 4;
            }
            AsapIndexManager::save(settings, tables, progress);
        }
    }

    // Stage 5: Sorting and Indexing
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Sorting and indexing tables...";
        progress->currentStage = 5;
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
            std::string aAuthor = a.vpsAuthors.empty() ? a.authorName : a.vpsAuthors;
            std::string bAuthor = b.vpsAuthors.empty() ? b.authorName : b.vpsAuthors;
            return aAuthor < bAuthor;
        });
    } else if (sortBy == "type") {
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            return a.type < b.type;
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