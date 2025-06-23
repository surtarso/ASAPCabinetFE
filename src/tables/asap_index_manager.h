/**
 * @file asap_index_manager.h
 * @brief Defines the AsapIndexManager class for managing ASAP index files in ASAPCabinetFE.
 *
 * This header provides the AsapIndexManager class, which implements static methods to
 * load and save table data from/to an ASAP index file (asapcab_index.json). The
 * manager handles JSON-based serialization and deserialization of TableData, supports
 * progress tracking via LoadingProgress, and is configurable via Settings (e.g., indexPath).
 * The process can be extended with configUI for custom index paths or format options in
 * the future.
 */

#ifndef ASAP_INDEX_MANAGER_H
#define ASAP_INDEX_MANAGER_H // Header guard to prevent multiple inclusions

#include "table_data.h" // Structure for storing table data
#include "itable_callbacks.h"
#include "config/settings.h" // Configuration settings for index path
#include "core/loading_progress.h" // Structure for tracking loading/saving progress
#include <vector> // For returning and passing vectors of TableData

/**
 * @class AsapIndexManager
 * @brief Manages loading and saving of ASAP index files for ASAPCabinetFE.
 *
 * This class provides static methods to load table data from an existing ASAP index
 * file and save table data to a new or updated index file. It uses JSON for data
 * serialization, supports progress tracking with LoadingProgress, and relies on
 * Settings for the index file path. The manager handles errors (e.g., invalid JSON)
 * and can be configured via Settings, with potential for configUI enhancements
 * (e.g., custom index locations).
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
     * Attempts to read and parse the asapcab_index.json
 file specified in
     * settings.indexPath, populating the tables vector with TableData objects. If
     * progress is provided, it updates currentTablesLoaded and totalTablesToLoad.
     * The method checks for file existence and JSON validity, returning false on
     * failure. Configurability via settings.indexPath allows for custom locations.
     *
     * @param settings The application settings containing the index file path.
     * @param tables Reference to the vector to store the loaded table data.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if loading succeeds, false otherwise.
     */
    bool load(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress = nullptr);

    /**
     * @brief Saves table data to the ASAP index file.
     *
     * Serializes the provided tables vector into a JSON array and writes it to
     * asapcab_index.json
 at settings.indexPath. If progress is provided, it
     * updates currentTablesLoaded and totalTablesToLoad. The method creates parent
     * directories if needed and returns false on I/O or serialization errors.
     * Configurability via settings.indexPath supports custom save locations.
     *
     * @param settings The application settings containing the index file path.
     * @param tables The vector of TableData to save.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if saving succeeds, false otherwise.
     */
    bool save(const Settings& settings, const std::vector<TableData>& tables, LoadingProgress* progress = nullptr);

private:
    Settings settings_; ///< Stored settings for index file path.
};

#endif // ASAP_INDEX_MANAGER_H