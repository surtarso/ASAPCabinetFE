/**
 * @file itable_launcher.h
 * @brief Defines the ITableLauncher interface for launching VPX tables.
 *
 * This header provides the interface for executing Visual Pinball X (VPX) table processes.
 */

#ifndef ITABLE_LAUNCHER_H
#define ITABLE_LAUNCHER_H

#include "tables/table_data.h"
#include <string>
#include <utility>

/**
 * @class ITableLauncher
 * @brief Interface for launching VPX tables.
 *
 * Defines methods for executing VPX table processes.
 */
class ITableLauncher {
public:
    virtual ~ITableLauncher() = default;

    /**
     * @brief Launches a VPX table.
     *
     * Constructs and executes a VPX command for the specified table.
     *
     * @param table The table data containing the VPX file path and metadata.
     * @return A pair containing the exit code (0 for success) and formatted playtime (H:M:S).
     */
    virtual std::pair<int, float> launchTable(const TableData& table) = 0;
};

#endif // ITABLE_LAUNCHER_H