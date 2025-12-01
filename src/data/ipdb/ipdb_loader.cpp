#include "ipdb_loader.h"
#include "log/logging.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace data::ipdb {

IpdbLoader::IpdbLoader(const Settings& settings, LoadingProgress* progress)
    : settings_(settings), progress_(progress) {}

nlohmann::json IpdbLoader::load() {
    fs::path path = settings_.ipdbPath;

    if (!fs::exists(path)) {
        LOG_ERROR("IPDB load failed: file missing");
        return {};
    }

    LOG_INFO("Loading IPDB JSON…");

    nlohmann::json db;
    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_ERROR("Cannot open IPDB file: " + path.string());
        return {};
    }

    try {
        f >> db;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("IPDB JSON parse error: ") + e.what());
        return {};
    }

    LOG_INFO("IPDB loaded successfully (" + std::to_string(db.size()) + " entries)");
    return db;
}

} // namespace data::ipdb
