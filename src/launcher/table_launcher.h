/**
 * @file table_launcher.h
 * @brief Defines the TableLauncher class for launching VPX tables.
 *
 * This header provides the implementation for launching Visual Pinball X (VPX) tables,
 * integrating with configuration settings.
 */

#ifndef TABLE_LAUNCHER_H
#define TABLE_LAUNCHER_H

#include "itable_launcher.h"
#include "config/iconfig_service.h"

/**
 * @class TableLauncher
 * @brief Implements ITableLauncher for launching VPX tables.
 *
 * Constructs and executes VPX commands, integrating with configuration settings.
 */
class TableLauncher : public ITableLauncher {
public:
    /**
     * @brief Constructs a TableLauncher instance.
     *
     * @param configService The configuration service for settings.
     */
    explicit TableLauncher(IConfigService* configService);
    
    /**
     * @brief Launches a VPX table.
     *
     * @param table The table data containing the VPX file path and metadata.
     * @return A pair containing the exit code (0 for success) and formatted playtime (H:M:S).
     */
    std::pair<int, float> launchTable(const TableData& table) override;

private:
    IConfigService* configService_; ///< Configuration service for settings.
};

#endif // TABLE_LAUNCHER_H