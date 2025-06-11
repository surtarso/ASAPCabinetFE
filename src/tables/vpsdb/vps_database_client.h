/**
 * @file vps_database_client.h
 * @brief Defines the VpsDatabaseClient class for managing the VPS database in ASAPCabinetFE.
 *
 * This header provides the VpsDatabaseClient class, which serves as a unified interface
 * for loading, updating, and enriching table data using the VPS database (vpsdb.json).
 * The class integrates VpsDatabaseLoader, VpsDataScanner, and VpsDatabaseUpdater, and
 * supports progress tracking via LoadingProgress. The process is configurable via the
 * vpsDbPath, with potential for configUI enhancements (e.g., custom update schedules).
 */

#ifndef VPS_DATABASE_CLIENT_H
#define VPS_DATABASE_CLIENT_H // Header guard to prevent multiple inclusions

#include <string> // For std::string to handle file paths
#include "tables/table_data.h" // Structure for storing table data
#include "vps_database_loader.h" // Class for loading the VPS database
#include "vps_data_scanner.h" // Class for enriching table data with VPS data
#include "vps_database_updater.h" // Class for updating the VPS database
#include "core/loading_progress.h" // Structure for tracking progress

/**
 * @class VpsDatabaseClient
 * @brief Manages the VPS database and its integration with table data in ASAPCabinetFE.
 *
 * This class provides a high-level interface to load, update, and enrich TableData
 * objects using the VPS database (vpsdb.json). It delegates loading to VpsDatabaseLoader,
 * enrichment to VpsDataScanner, and updates to VpsDatabaseUpdater, all configured with
 * a single vpsDbPath. Progress is tracked via LoadingProgress, and the client can be
 * extended with configUI for custom settings (e.g., update frequency or enrichment rules).
 */
class VpsDatabaseClient {
public:
    /**
     * @brief Constructs a VpsDatabaseClient instance.
     *
     * Initializes the client with the path to the VPS database file, which is used
     * by the loader, enricher, and updater components. The enricher is initialized
     * with the loaded database data.
     *
     * @param vpsDbPath The file path to the VPS database (vpsdb.json).
     */
    VpsDatabaseClient(const std::string& vpsDbPath);

    /**
     * @brief Loads the VPS database from the specified file path.
     *
     * Delegates the loading process to the internal VpsDatabaseLoader, parsing
     * vpsdb.json into a JSON object. Progress is tracked via LoadingProgress if
     * provided.
     *
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if loading succeeds, false otherwise.
     */
    bool load(LoadingProgress* progress = nullptr);

    /**
     * @brief Enriches a TableData object with VPS database data.
     *
     * Delegates the enrichment process to the internal VpsDataScanner, matching
     * the vpxTable JSON with the tableData object. Progress is tracked via
     * LoadingProgress if provided.
     *
     * @param vpxTable The JSON data from a VPX table to use for enrichment.
     * @param tableData Reference to the TableData object to enrich.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if enrichment succeeds, false otherwise.
     */
    bool enrichTableData(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress = nullptr) const;

    /**
     * @brief Fetches the VPS database if an update is needed.
     *
     * Delegates the update process to the internal VpsDatabaseUpdater, checking
     * the last updated timestamp and downloading vpsdb.json if necessary. Progress
     * is tracked via LoadingProgress if provided.
     *
     * @param lastUpdatedPath The path to the local lastUpdated.json file.
     * @param updateFrequency The frequency setting (e.g., "startup") to control updates.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if the database is valid (exists or updated), false on critical failure.
     */
    bool fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress = nullptr);

private:
    std::string vpsDbPath_; ///< The file path to the VPS database (vpsdb.json).
    VpsDatabaseLoader loader_; ///< The loader component for parsing the VPS database.
    VpsDataScanner enricher_; ///< The enricher component for updating table data.
    VpsDatabaseUpdater updater_; ///< The updater component for fetching database updates.
};

#endif // VPS_DATABASE_CLIENT_H