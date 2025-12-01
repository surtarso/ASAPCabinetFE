/**
 * vpinmdb_loader.cpp
 *
 * Implementation of VpinMdbLoader::load().
 * Reads the JSON file from disk, parses it with nlohmann::json and returns it.
 * Logging and progress messages; exceptions propagate for caller to handle.
 *
 */

#include "vpinmdb_loader.h"
#include <fstream>
#include <iostream>
#include "log/logging.h"

namespace data::vpinmdb {
using json = nlohmann::json;
namespace fs = std::filesystem;

void VpinMdbLoader::pushProgressMessage(const std::string &msg) {
    if (!progress_) return;
    std::lock_guard<std::mutex> lock(progress_->mutex);
    progress_->logMessages.push_back(msg);
}

json VpinMdbLoader::load() {
    fs::path dbPath = settings_.vpinmdbPath;

    std::ifstream file(dbPath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open " + dbPath.string());
        pushProgressMessage("Failed to open vpinmdb.json: " + dbPath.string());
        throw std::runtime_error("Failed to open vpinmdb.json: " + dbPath.string());
    }

    try {
        json mediaDb;
        file >> mediaDb;
        LOG_INFO("Loaded VPin Media Database from " + dbPath.string());
        pushProgressMessage("Loaded vpinmdb.json from " + dbPath.string());
        return mediaDb;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse vpinmdb.json: " + std::string(e.what()));
        pushProgressMessage("Failed to parse vpinmdb.json: " + std::string(e.what()));
        throw; // re-throw to match original early-return behavior
    }
}

} // namespace data::vpinmdb
