/**
 * @file vps_database_client.h
 * @brief Defines the VpsDatabaseClient class for enriching vpxtool metadata with vpsdb.json.
 *
 * This header provides the VpsDatabaseClient class, which loads vpsdb.json and enriches
 * vpxtool metadata with additional fields (e.g., themes, type) for Visual Pinball X (VPX)
 * tables in ASAPCabinetFE.
 */

#ifndef VPS_DATABASE_CLIENT_H
#define VPS_DATABASE_CLIENT_H

#include <string>
#include <json.hpp>
#include "table_data.h"

class VpsDatabaseClient {
public:
    /**
     * @brief Constructs a VpsDatabaseClient with the path to vpsdb.json.
     *
     * @param vpsDbPath Path to vpsdb.json.
     */
    VpsDatabaseClient(const std::string& vpsDbPath);

    /**
     * @brief Loads vpsdb.json into memory.
     *
     * @return True if loaded successfully, false otherwise.
     */
    bool load();

    /**
     * @brief Enriches TableData with vpsdb.json metadata.
     *
     * Matches vpxtool metadata (table_name, game_name, table_version) to vpsdb.json
     * entries and populates vpsdb-specific fields in TableData.
     *
     * @param vpxTable JSON object from vpxtool_index.json for the table.
     * @param tableData TableData instance to enrich.
     * @return True if a match was found and enriched, false otherwise.
     */
    bool enrichTableData(const nlohmann::json& vpxTable, TableData& tableData) const;

    /**
     * @brief Fetches vpsdb.json if needed based on update frequency.
     *
     * Checks lastUpdated.json to determine if vpsdb.json needs updating and downloads
     * it if necessary.
     *
     * @param lastUpdatedPath Path to vpsdb_last_updated.txt.
     * @param updateFrequency Update frequency ("startup", "daily", "manual").
     * @return True if vpsdb.json is available, false otherwise.
     */
    bool fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency);

private:
    /**
     * @brief Normalizes a string for matching.
     *
     * Converts to lowercase and removes spaces, underscores, parentheses, and special characters.
     *
     * @param input Input string.
     * @return Normalized string.
     */
    std::string normalizeString(const std::string& input) const;
    std::string normalizeStringLessAggressive(const std::string& input) const;
    std::string extractYearFromDate(const std::string& dateString) const;
    /**
     * @brief Downloads vpsdb.json from the specified URL.
     *
     * @param url URL to download vpsdb.json.
     * @return True if download and save were successful, false otherwise.
     */
    bool downloadVpsDb(const std::string& url);

    nlohmann::json vpsDb_;    ///< Parsed vpsdb.json content.
    std::string vpsDbPath_;   ///< Path to vpsdb.json.
};

#endif // VPS_DATABASE_CLIENT_H