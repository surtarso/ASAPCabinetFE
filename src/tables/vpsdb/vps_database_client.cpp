/**
 * @file vps_database_client.cpp
 * @brief Implements the VpsDatabaseClient class for managing the VPS database in ASAPCabinetFE.
 *
 * This file provides the implementation of the VpsDatabaseClient class, which serves as
 * a unified interface for loading, updating, and enriching table data using the VPS
 * database (vpsdb.json). It delegates tasks to VpsDatabaseLoader, VpsDataScanner, and
 * VpsDatabaseUpdater, all initialized with a common vpsDbPath. The process supports
 * progress tracking via LoadingProgress and can be extended with configUI for custom
 * settings (e.g., update frequency or enrichment rules) in the future.
 */

#include "vps_database_client.h"
#include "utils/logging.h"

VpsDatabaseClient::VpsDatabaseClient(const std::string& vpsDbPath)
    : vpsDbPath_(vpsDbPath), // Initialize with the provided VPS database path
      loader_(vpsDbPath), // Initialize loader with the same path
      enricher_(loader_.getVpsDb()), // Initialize enricher with the loaded database
      updater_(vpsDbPath) {} // Initialize updater with the same path

bool VpsDatabaseClient::load(LoadingProgress* progress) {
    return loader_.load(progress); // Delegate loading to the loader component
}

bool VpsDatabaseClient::matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    return enricher_.matchMetadata(vpxTable, tableData, progress); // Delegate enrichment to the enricher component
}

bool VpsDatabaseClient::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress) {
    return updater_.fetchIfNeeded(lastUpdatedPath, updateFrequency, progress); // Delegate update check to the updater component
}