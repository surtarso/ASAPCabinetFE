#include "ipdb_updater.h"
#include "log/logging.h"
#include "data/vpinmdb/vpinmdb_downloader.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace data::ipdb {

IpdbUpdater::IpdbUpdater(const Settings& settings, LoadingProgress* progress)
    : settings_(settings), progress_(progress) {}

// ---------------------------------------------------------
// ensureAvailable()
// ---------------------------------------------------------
bool IpdbUpdater::ensureAvailable() {
    fs::path path = settings_.ipdbPath;

    if (fs::exists(path)) {
        return true;
    }

    LOG_WARN("IPDB missing — downloading...");
    return download();
}

// ---------------------------------------------------------
// forceUpdate()
// ---------------------------------------------------------
bool IpdbUpdater::forceUpdate() {
    LOG_INFO("Forcing IPDB update...");
    return download();
}

// ---------------------------------------------------------
// download()
// ---------------------------------------------------------
bool IpdbUpdater::download() {
    std::string IPDB_URL = settings_.ipdbUrl;
    fs::path path = settings_.ipdbPath;

    fs::create_directories(path.parent_path());

    if (!data::filedownloader::downloadFile(IPDB_URL, path)) {
        LOG_ERROR("Failed to download IPDB → " + path.string());
        return false;
    }

    LOG_INFO("IPDB downloaded: " + path.string());
    return true;
}

} // namespace data::ipdb
