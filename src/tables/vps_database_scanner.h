#ifndef VPS_DATABASE_SCANNER_H
#define VPS_DATABASE_SCANNER_H

#include "core/ui/loading_progress.h"
#include "config/settings.h"
#include "data/table_data.h"
#include "utils/string_utils.h"
#include <nlohmann/json.hpp>
#include <string>

/**
 * @class VpsDataScanner
 * @brief Matches VPX table metadata to VPS database entries in ASAPCabinetFE.
 *
 * This class enriches TableData objects by matching their metadata (primarily filename-derived title,
 * manufacturer, and year) against vpsdb.json entries. It prioritizes filename-derived fields to
 * handle unreliable internal metadata, uses simplified scoring for accuracy, and optimizes performance.
 */
class VpsDataScanner {
public:
    /**
     * @brief Constructs a VpsDataScanner instance with a loaded VPS database.
     *
     * @param vpsDb Reference to the parsed vpsdb.json data.
     */
    VpsDataScanner(const nlohmann::json& vpsDb, const Settings& settings);

    /**
     * @brief Matches a VPX table's metadata to a VPS database entry.
     *
     * Compares vpxTable JSON and TableData against vpsdb.json, prioritizing filename_title.
     * Populates TableData's VPS fields (e.g., vpsId, vpsName) for high-confidence matches.
     * Updates progress and logs mismatches.
     *
     * @param vpxTable JSON data from a VPX table (e.g., from vpin_scanner).
     * @param tableData Reference to the TableData object to match.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if a match is found and applied, false otherwise.
     */
    bool matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress = nullptr) const;

private:
    void populateFromVpsEntry(const nlohmann::json& entry, TableData& tableData, float confidence) const;

    const nlohmann::json& vpsDb_; ///< Reference to the loaded VPS database.
    StringUtils utils_; ///< Utility functions for string processing.
    const Settings& settings_; ///< For weight management
};

#endif // VPS_DATABASE_SCANNER_H
