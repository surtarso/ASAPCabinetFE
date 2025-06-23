/**
 * @file asap_index_manager.h
 * @brief Defines the AsapIndexManager class for managing ASAP index files in ASAPCabinetFE.
 *
 * This header provides the AsapIndexManager class, which implements methods to
 * load, save, and merge table data from/to an ASAP index file (asapcab_index.json).
 * The manager handles JSON-based serialization and deserialization of TableData,
 * supports progress tracking via LoadingProgress, and is configurable via Settings.
 */

#ifndef ASAP_INDEX_MANAGER_H
#define ASAP_INDEX_MANAGER_H

#include "table_data.h"
#include "itable_callbacks.h"
#include "config/settings.h"
#include "core/loading_progress.h"
#include <vector>

/**
 * @class AsapIndexManager
 * @brief Manages loading, saving, and merging of ASAP index files for ASAPCabinetFE.
 *
 * This class provides methods to load table data from an existing ASAP index
 * file, save table data to a new or updated index file, and merge new table data
 * with existing index entries. It uses JSON for data serialization, supports
 * progress tracking with LoadingProgress, and relies on Settings for configuration.
 */
class AsapIndexManager : public ITableCallbacks {
public:
    /**
     * @brief Constructs an AsapIndexManager with the given settings.
     *
     * @param settings The application settings containing the index file path.
     */
    AsapIndexManager(const Settings& settings);

    /**
     * @brief Loads table data from the ASAP index file.
     *
     * Attempts to read and parse the asapcab_index.json file specified in
     * settings.indexPath, populating the tables vector with TableData objects.
     *
     * @param settings The application settings containing the index file path.
     * @param tables Reference to the vector to store the loaded table data.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if loading succeeds, false otherwise.
     */
    bool load(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress = nullptr) override;

    /**
     * @brief Saves table data to the ASAP index file.
     *
     * Serializes the provided tables vector into a JSON array and writes it to
     * asapcab_index.json at settings.indexPath.
     *
     * @param settings The application settings containing the index file path.
     * @param tables The vector of TableData to save.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if saving succeeds, false otherwise.
     */
    bool save(const Settings& settings, const std::vector<TableData>& tables, LoadingProgress* progress = nullptr) override;

    /**
     * @brief Merges new table data with existing index data.
     *
     * Compares new table data with existing index entries, updating tables with higher-quality
     * metadata (based on jsonOwner priority), adding new tables, and removing deleted ones.
     * Preserves user fields like playCount. Returns the merged table data.
     *
     * @param settings The application settings containing the index file path.
     * @param newTables The new table data to merge.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return The merged vector of TableData.
     */
    std::vector<TableData> mergeTables(const Settings& settings, const std::vector<TableData>& newTables, LoadingProgress* progress = nullptr) override;

private:
    Settings settings_; ///< Stored settings for index file path.
};

#endif // ASAP_INDEX_MANAGER_H