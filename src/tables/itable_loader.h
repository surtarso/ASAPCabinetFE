#ifndef ITABLE_LOADER_H
#define ITABLE_LOADER_H

#include <vector>
#include <map>
#include "table_data.h"
#include "config/settings.h"
#include "core/ui/loading_progress.h"

/**
 * @brief Interface for loading table data.
 *
 * Provides an abstract interface to load a list of table data and retrieve a mapping from characters to indices.
 */
class ITableLoader {
public:
    virtual ~ITableLoader() = default;

    /**
     * @brief Loads a list of table data entries.
     *
     * This pure virtual function should be implemented by derived classes to load a collection
     * of TableData objects based on the given settings. The provided settings parameter supplies
     * the necessary configuration for determining which table data to load.
     *
     * @param settings Configuration parameters used to load the table data.
     * @param progress Optional pointer to LoadingProgress for tracking loading status.
     * @return A vector containing the loaded TableData objects.
     */
    virtual std::vector<TableData> loadTableList(const Settings& settings, LoadingProgress* progress = nullptr) = 0;
};

#endif // ITABLE_LOADER_H
