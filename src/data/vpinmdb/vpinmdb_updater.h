#pragma once
/**
 * vpinmdb_updater.h
 *
 * Responsible only for ensuring the local vpinmdb.json exists (and downloading it
 * if missing). Uses existing vpinmdb::downloadFile(url, path) call.
 *
 */

#include <string>
#include <filesystem>
#include <optional>
#include "vpinmdb_downloader.h"
#include "config/settings.h"
#include "core/ui/loading_progress.h"


// namespace filedownloader {
//     bool downloadFile(const std::string &url, const std::filesystem::path &dest); // existing
// }

namespace data::vpinmdb {

namespace fs = std::filesystem;

class VpinMdbUpdater {
public:
    VpinMdbUpdater(const Settings& settings, LoadingProgress* progress)
        : settings_(settings), progress_(progress) {}

    // Ensure the vpinmdb file exists locally. Returns true if file exists (either already present
    // or successfully downloaded). Returns false if download failed or directory creation failed.
    // Side-effects: logs into progress_->logMessages when progress_ != nullptr.
    bool ensureAvailable();

private:
    const Settings& settings_;
    LoadingProgress* progress_;

    void pushProgressMessage(const std::string &msg);
};

} // namespace data::vpinmdb
