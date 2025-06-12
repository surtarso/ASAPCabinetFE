/**
 * @file data_enricher.h
 * @brief Defines the VPinScanner class for enriching table data in ASAPCabinetFE.
 *
 * This header provides the VPinScanner class, which contains static methods to enrich
 * TableData objects with metadata from vpxtool_index.json and VPSDB. The class supports
 * progress tracking via LoadingProgress, string cleaning for metadata fields, and safe
 * JSON value extraction. The enrichment process is configurable via Settings (e.g.,
 * VPXTablesPath, fetchVPSdb), with potential for configUI integration to customize
 * metadata sources or cleaning rules in the future.
 */

#ifndef VPIN_SCANNER_H
#define VPIN_SCANNER_H // Header guard to prevent multiple inclusions

#include "tables/vpsdb/vps_database_client.h"
#include "tables/table_data.h" // Structure for storing table data
#include "config/settings.h" // Configuration settings for paths and VPSDB options
#include "core/loading_progress.h" // Structure for tracking enrichment progress
#include <nlohmann/json.hpp> // For nlohmann::json to handle JSON parsing
#include <vector> // For passing vectors of TableData

/**
 * @class VPinScanner
 * @brief Enriches table data with metadata in ASAPCabinetFE.
 *
 * This class provides static methods to enrich TableData objects by extracting metadata
 * from a vpxtool_index.json file and optionally a VPSDB database. It includes utility
 * methods for cleaning strings and safely extracting JSON values. The enrichment process
 * updates fields like tableName, authorName, and romName, tracks progress with
 * LoadingProgress, and can be configured via Settings, with potential for configUI
 * enhancements (e.g., custom metadata sources).
 */
class VPinScanner {
public:
    /**
     * @brief Enriches table data with metadata from vpxtool_index.json and VPSDB.
     *
     * Processes the provided tables vector by matching entries with metadata from
     * vpxtool_index.json (located at settings.VPXTablesPath + vpxtoolIndex) and
     * optionally enriching further with VPSDB data if enabled. Updates fields like
     * tableName, authorName, and title, and tracks progress via LoadingProgress,
     * including numNoMatch for unmatched tables. The method is configurable via
     * Settings and supports future configUI customization.
     *
     * @param settings The application settings controlling the enrichment process.
     * @param tables Reference to the vector of TableData to enrich.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     */
    static void scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress = nullptr);

    /**
     * @brief Cleans a string by removing control characters and trimming whitespace.
     *
     * Removes carriage returns, newlines, control characters, and leading/trailing
     * whitespace from the input string. Returns an empty string if the result is
     * all whitespace. This method is used to sanitize metadata fields during enrichment.
     *
     * @param input The input string to clean.
     * @return The cleaned string, or empty if invalid.
     */
    static std::string cleanString(const std::string& input);

    /**
     * @brief Safely extracts a string value from a JSON object.
     *
     * Retrieves the value associated with the specified key from the JSON object.
     * If the value is a string, it is returned directly; if it is a number, it is
     * converted to a string; otherwise, the defaultValue is returned with a debug log.
     * This method ensures robust JSON parsing during enrichment.
     *
     * @param j The JSON object to query.
     * @param key The key to look up in the JSON object.
     * @param defaultValue The fallback value if the key is missing or invalid.
     * @return The extracted string value, or defaultValue if extraction fails.
     */
    static std::string safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue = "");
};

#endif // VPIN_SCANNER_H