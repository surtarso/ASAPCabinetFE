/**
 * @file vps_database_client.cpp
 * @brief Implements the VpsDatabaseClient class for managing the VPS database in ASAPCabinetFE.
 *
 * This file provides the implementation of the VpsDatabaseClient class, which serves as
 * a unified interface for loading, updating, and matchmaking table data using the VPS
 * database (vpsdb.json). It delegates tasks to VpsDatabaseLoader, VpsDataScanner, and
 * VpsDatabaseUpdater, all initialized with a common vpsDbPath. The process supports
 * progress tracking via LoadingProgress and can be extended with configUI for custom
 * settings (e.g., update frequency or matchmaking rules) in the future.
 */

#include "vps_database_client.h"
#include "log/logging.h"

// VpsDatabaseClient::VpsDatabaseClient(const std::string& vpsDbPath, const Settings& settings)
VpsDatabaseClient::VpsDatabaseClient(const Settings& settings)
    : settings_(settings),
    //   vpsDbPath_(setting_.vpsDbPath), // Initialize with the provided VPS database path
      loader_(settings_.vpsDbPath), // Initialize loader with the same path
      matchmaker_(loader_.getVpsDb(), settings_), // Initialize matchmaker with the loaded database
      updater_(settings_.vpsDbPath) {} // Initialize updater with the same path

bool VpsDatabaseClient::load(LoadingProgress* progress) {
    return loader_.load(progress); // Delegate loading to the loader component
}

bool VpsDatabaseClient::matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    return matchmaker_.matchMetadata(vpxTable, tableData, progress); // Delegate matchmaking to the matchmaker component
}

bool VpsDatabaseClient::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress) {
    return updater_.fetchIfNeeded(lastUpdatedPath, updateFrequency, progress); // Delegate update check to the updater component
}

const nlohmann::json& VpsDatabaseClient::getLoadedVpsDb() const {
    return loader_.getVpsDb();
}
