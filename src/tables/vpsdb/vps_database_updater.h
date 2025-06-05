/**
 * @file vps_database_updater.h
 * @brief Defines the VpsDatabaseUpdater class for updating the VPS database in ASAPCabinetFE.
 *
 * This header provides the VpsDatabaseUpdater class, which manages the process of
 * checking and downloading updates for the VPS database (vpsdb.json) from a remote
 * source. The class uses CURL for HTTP requests, supports progress tracking via
 * LoadingProgress, and is configurable via settings like update frequency and paths.
 * The update process can be extended with configUI for custom URLs or frequency in
 * the future.
 */

#ifndef VPS_DATABASE_UPDATER_H
#define VPS_DATABASE_UPDATER_H // Header guard to prevent multiple inclusions

#include <string> // For std::string to handle paths and URLs
#include "core/loading_progress.h" // Structure for tracking update progress

/**
 * @class VpsDatabaseUpdater
 * @brief Updates the VPS database file in ASAPCabinetFE.
 *
 * This class handles the updating of the VPS database (vpsdb.json) by checking a
 * remote lastUpdated.json file for the latest timestamp and downloading the database
 * if needed. It uses CURL for HTTP requests, supports multiple fallback URLs, and
 * tracks progress with LoadingProgress. The update frequency and paths are configurable
 * via input parameters, with potential for configUI enhancements (e.g., custom URLs).
 */
class VpsDatabaseUpdater {
public:
    /**
     * @brief Constructs a VpsDatabaseUpdater instance.
     *
     * Initializes the updater with the path where the VPS database file will be stored.
     * This path is used as the target for downloaded updates.
     *
     * @param vpsDbPath The file path for storing the VPS database.
     */
    VpsDatabaseUpdater(const std::string& vpsDbPath);

    /**
     * @brief Fetches the VPS database if an update is needed.
     *
     * Checks the last updated timestamp against the local copy (from lastUpdatedPath)
     * and downloads vpsdb.json from a predefined URL if the remote version is newer
     * or the local file is missing. The update frequency (e.g., "startup") determines
     * if the check occurs. Progress is tracked via LoadingProgress if provided.
     *
     * @param lastUpdatedPath The path to the local lastUpdated.json file.
     * @param updateFrequency The frequency setting (e.g., "startup") to control updates.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if the database is valid (exists or updated), false on critical failure.
     */
    bool fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress = nullptr);

private:
    /**
     * @brief Downloads the VPS database from a given URL.
     *
     * Uses CURL to download vpsdb.json from the specified URL, validates the response
     * (e.g., HTTP 200, JSON content-type), and saves it to vpsDbPath_. Progress is
     * tracked via LoadingProgress if provided.
     *
     * @param url The URL to download the VPS database from.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if the download and save succeed, false otherwise.
     */
    bool downloadVpsDb(const std::string& url, LoadingProgress* progress);

    std::string vpsDbPath_; ///< The file path where the VPS database is stored.
};

#endif // VPS_DATABASE_UPDATER_H