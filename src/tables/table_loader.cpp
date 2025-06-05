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
#include "tables/vpx_scanner.h"
#include "tables/data_enricher.h"
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
        tables = VpxScanner::scan(settings, progress);
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
            DataEnricher::enrich(settings, tables, progress);
            if (progress) {
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
    if (tables.empty()) return;

    // Sort based on the selected criterion
    if (sortBy == "author") {
        std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
            // Prefer vpsAuthors, fallback to authorName if empty
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
        char firstChar = tables[i].title[0];
        if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = static_cast<int>(i); // Assign index of first occurrence
            }
        }
    }
}