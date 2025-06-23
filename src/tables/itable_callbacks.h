/**
 * @file itable_callbacks.h
 * @brief Defines the ITableCallbacks interface for managing ASAP index files in ASAPCabinetFE.
 *
 * This header provides the ITableCallbacks interface, which specifies methods for
 * loading and saving table data to/from an ASAP index file (asapcab_index.json).
 * It is designed to be implemented by classes like AsapIndexManager for handling
 * JSON-based serialization and deserialization of TableData.
 */

#ifndef ITABLE_CALLBACKS_H
#define ITABLE_CALLBACKS_H

#include "tables/table_data.h"
#include "config/settings.h"
#include "core/loading_progress.h"
#include <vector>

/**
 * @class ITableCallbacks
 * @brief Interface for managing ASAP index file operations.
 *
 * Defines methods for loading and saving table data, supporting progress tracking
 * and configuration via Settings. Implementers handle JSON serialization and file I/O.
 */
class ITableCallbacks {
public:
    virtual ~ITableCallbacks() = default;

    /**
     * @brief Loads table data from the ASAP index file.
     *
     * Reads and parses asapcab_index.json from the specified settings.indexPath,
     * populating the tables vector with TableData objects. Supports progress tracking.
     *
     * @param settings The application settings containing the index file path.
     * @param tables Reference to the vector to store the loaded table data.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if loading succeeds, false otherwise.
     */
    virtual bool load(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress = nullptr) = 0;

    /**
     * @brief Saves table data to the ASAP index file.
     *
     * Serializes the provided tables vector into a JSON array and writes it to
     * asapcab_index.json at settings.indexPath. Supports progress tracking.
     *
     * @param settings The application settings containing the index file path.
     * @param tables The vector of TableData to save.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if saving succeeds, false otherwise.
     */
    virtual bool save(const Settings& settings, const std::vector<TableData>& tables, LoadingProgress* progress = nullptr) = 0;
};

#endif // ITABLE_CALLBACKS_H