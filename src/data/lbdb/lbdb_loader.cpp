#include "lbdb_loader.h"
#include "log/logging.h"
#include <fstream>

namespace data::lbdb {
using json = nlohmann::json;
namespace fs = std::filesystem;

void LbdbLoader::push(const std::string& msg) {
    if (!progress_) return;
    std::lock_guard<std::mutex> lock(progress_->mutex);
    progress_->logMessages.push_back(msg);
}

json LbdbLoader::load() {
    fs::path jsonPath = settings_.lbdbPath;

    std::ifstream f(jsonPath);
    if (!f.is_open()) {
        LOG_ERROR("Failed to open launchbox pinball DB: " + jsonPath.string());
        push("Failed to open launchbox DB");
        return json{};
    }

    try {
        json db;
        f >> db;
        LOG_INFO("Loaded LaunchBox DB (" + std::to_string(db.size()) + " entries)");
        push("Loaded LaunchBox DB");
        return db;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Invalid JSON in launchbox DB: " + std::string(e.what()));
        push("Invalid JSON in launchbox DB");
        return json{};
    }
}

} // namespace data::lbdb
