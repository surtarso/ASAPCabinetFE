/**
 * @file vpinmdb_client.h
 * @brief Defines the VpinMdbClient class for orchestrating table media downloads in ASAPCabinetFE.
 *
 * This header provides the VpinMdbClient class, which coordinates downloading media
 * from vpinmdb.json for tables with valid VPSDB IDs. It uses vpinmdb_downloader and vpinmdb_image
 * components, integrating with TableLoader, Settings, and LoadingProgress.
 */

#ifndef VPINMDB_CLIENT_H
#define VPINMDB_CLIENT_H

#include "config/settings.h"
#include "tables/table_data.h"
#include "core/loading_progress.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <mutex>

namespace fs = std::filesystem;

class VpinMdbClient {
public:
    /**
     * @brief Constructs a VpinMdbClient instance.
     * @param settings Application settings controlling download behavior.
     * @param progress Optional pointer to LoadingProgress for updates.
     * @param mediaDb Optional pre-loaded vpinmdb.json (if null, loads from data/vpinmdb.json).
     */
    VpinMdbClient(const Settings& settings, LoadingProgress* progress = nullptr, const nlohmann::json* mediaDb = nullptr);

    /**
     * @brief Downloads media for a list of tables and updates their media paths.
     * @param tables Vector of TableData to process (updated with new media paths).
     * @return True if any media was successfully downloaded.
     */
    bool downloadMedia(std::vector<TableData>& tables);

private:
    const Settings& settings_; ///< Reference to application settings.
    LoadingProgress* progress_; ///< Pointer to progress tracker (nullable).
    nlohmann::json mediaDb_; ///< Loaded vpinmdb.json.
    std::mutex mutex_; ///< Mutex for thread-safe progress updates.

    /**
     * @brief Determines the preferred resolution based on window sizes.
     * @return "4k" if any window is >=2560x1440, else "1k".
     */
    std::string selectResolution() const;
};

#endif // VPINMDB_CLIENT_H