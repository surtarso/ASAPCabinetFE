/**
 * @file data_enricher.h
 * @brief Defines the VPinScanner class for matchmaking table data in ASAPCabinetFE.
 *
 * This header provides the VPinScanner class, which contains static methods to match
 * TableData objects with metadata from vpxtool_index.json and VPSDB. The class supports
 * progress tracking via LoadingProgress, string cleaning for metadata fields, and safe
 * JSON value extraction. The matchmaking process is configurable via Settings (e.g.,
 * VPXTablesPath, fetchVPSdb), with potential for configUI integration to customize
 * metadata sources or cleaning rules in the future.
 */

#ifndef VPIN_SCANNER_H
#define VPIN_SCANNER_H // Header guard to prevent multiple inclusions

#include "vpsdb/vps_database_client.h"
#include "table_data.h" // Structure for storing table data
#include "config/settings.h" // Configuration settings for paths and VPSDB options
#include "core/ui/loading_progress.h" // Structure for tracking matchmaking progress
#include <nlohmann/json.hpp> // For nlohmann::json to handle JSON parsing
#include <vector> // For passing vectors of TableData

/**
 * @class VPinScanner
 * @brief Enriches table data with metadata in ASAPCabinetFE.
 *
 * This class provides static methods to match TableData objects by extracting metadata
 * from a vpxtool_index.json file and optionally a VPSDB database. It includes utility
 * methods for cleaning strings and safely extracting JSON values. The matchmaking process
 * updates fields like tableName, tableAuthor, and romName, tracks progress with
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
     * optionally matchmaking further with VPSDB data if enabled. Updates fields like
     * tableName, tableAuthor, and title, and tracks progress via LoadingProgress,
     * including numNoMatch for unmatched tables. The method is configurable via
     * Settings and supports future configUI customization.
     *
     * @param settings The application settings controlling the matchmaking process.
     * @param tables Reference to the vector of TableData to match.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     */
    // static void scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress);
    static void scanFiles(std::vector<TableData>& tables, LoadingProgress* progress);
};

#endif // VPIN_SCANNER_H