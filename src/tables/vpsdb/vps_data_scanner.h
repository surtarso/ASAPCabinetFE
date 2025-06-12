#ifndef VPS_DATA_SCANNER_H
#define VPS_DATA_SCANNER_H

#include <nlohmann/json.hpp>
#include <vector>
#include "tables/table_data.h"
#include "vps_utils.h"
#include "core/loading_progress.h"

/**
 * @file vps_data_enricher.h
 * @brief Defines the VpsDataScanner class for enriching table data with VPSDB information.
 *
 * This header provides the VpsDataScanner class, which enhances TableData objects with
 * metadata from a VPS database by matching VPX table data using string similarity and
 * metadata comparison. It integrates with LoadingProgress for progress tracking.
 */

/**
 * @class VpsDataScanner
 * @brief Enriches TableData objects with metadata from a VPS database.
 *
 * This class uses Levenshtein distance and metadata matching (e.g., game name, year,
 * manufacturer) to enrich VPX table data with VPSDB entries. It supports progress
 * reporting and handles mismatches by logging them.
 */
class VpsDataScanner {
public:
    /**
     * @brief Constructs a VpsDataScanner with a VPS database reference.
     *
     * Initializes the enricher with a constant reference to the VPS database JSON.
     *
     * @param vpsDb Reference to the VPS database JSON object.
     */
    VpsDataScanner(const nlohmann::json& vpsDb);

    /**
     * @brief Enriches a single TableData object with VPSDB metadata.
     *
     * Matches the VPX table data against the VPS database using name similarity,
     * game name, year, and manufacturer, updating the TableData object accordingly.
     * Reports progress if a LoadingProgress pointer is provided.
     *
     * @param vpxTable JSON object containing VPX table data.
     * @param tableData Reference to the TableData object to enrich.
     * @param progress Optional pointer to LoadingProgress for progress tracking (default: nullptr).
     * @return True if a VPSDB match was found, false otherwise.
     */
    bool matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress = nullptr) const;

private:
    const nlohmann::json& vpsDb_; ///< Constant reference to the VPS database JSON.
    VpsUtils utils_;              ///< Utility object for string normalization and version comparison.
    /**
     * @brief Computes the Levenshtein distance between two strings.
     *
     * Calculates the minimum number of single-character edits required to transform
     * one string into another.
     *
     * @param s1 First string to compare.
     * @param s2 Second string to compare.
     * @return The Levenshtein distance between s1 and s2.
     */
    size_t levenshteinDistance(const std::string& s1, const std::string& s2) const;
};

#endif // VPS_DATA_SCANNER_H