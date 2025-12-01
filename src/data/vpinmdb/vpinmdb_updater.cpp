/**
 * vpinmdb_updater.cpp
 *
 * Implementation of VpinMdbUpdater.
 *
 * This is the download/directory logic, if the local path doesn't exist it
 * attempts to create parent directories and download the file using vpinmdb::downloadFile.
 */

#include "vpinmdb_updater.h"
#include <iostream>
#include <system_error>
#include "log/logging.h"

namespace data::vpinmdb {
namespace fs = std::filesystem;

void VpinMdbUpdater::pushProgressMessage(const std::string &msg) {
    if (!progress_) return;
    std::lock_guard<std::mutex> lock(progress_->mutex);
    progress_->logMessages.push_back(msg);
}

bool VpinMdbUpdater::ensureAvailable() {
    fs::path dbPath = settings_.vpinmdbPath;
    const std::string url = settings_.vpinmdbUrl;

    if (fs::exists(dbPath)) {
        LOG_INFO("VPin Media DB already present at " + dbPath.string());
        return true;
    }

    // Ensure parent directory
    if (!fs::exists(dbPath.parent_path())) {
        try {
            fs::create_directories(dbPath.parent_path());
            LOG_INFO("Created directory " + dbPath.parent_path().string());
            pushProgressMessage("Created directory " + dbPath.parent_path().string());
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR("Failed to create directory " + dbPath.parent_path().string() + ": " + std::string(e.what()));
            pushProgressMessage("Failed to create directory for vpinmdb.json: " + std::string(e.what()));
            return false;
        }
    }

    // Download using existing helper
    if (filedownloader::downloadFile(url, dbPath)) {
        LOG_INFO("Downloaded VPin Media Database to " + dbPath.string());
        pushProgressMessage("Downloaded vpinmdb.json to " + dbPath.string());
        return true;
    } else {
        LOG_ERROR("Failed to download vpinmdb.json from " + url);
        pushProgressMessage("Failed to download vpinmdb.json");
        return false;
    }
}
} // namespace data::vpinmdb
