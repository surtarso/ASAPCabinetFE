// tables/vps_database_loader.cpp
#include "vps_database_loader.h"
#include <filesystem>
#include <fstream>
#include "utils/logging.h"

namespace fs = std::filesystem;

VpsDatabaseLoader::VpsDatabaseLoader(const std::string& vpsDbPath) : vpsDbPath_(vpsDbPath) {}

bool VpsDatabaseLoader::load(LoadingProgress* progress) {
    try {
        std::ifstream file(vpsDbPath_);
        if (!file.is_open()) {
            LOG_ERROR("VpsDatabaseLoader: Failed to open vpsdb.json at: " << vpsDbPath_);
            return false;
        }

        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Loading VPSDB JSON...";
            progress->currentTablesLoaded = 0;
            // Do not set totalTablesToLoad to preserve local table count
        }

        vpsDb_ = nlohmann::json::parse(file, nullptr, true, true);
        file.close();

        if (vpsDb_.is_array()) {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = vpsDb_.size();
                progress->currentTask = "VPSDB JSON loaded";
            }
            LOG_INFO("VpsDatabaseLoader: Loaded vpsdb.json with " << vpsDb_.size() << " entries");
            return true;
        } else if (vpsDb_.is_object() && vpsDb_.contains("tables") && vpsDb_["tables"].is_array()) {
            vpsDb_ = vpsDb_["tables"];
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = vpsDb_.size();
                progress->currentTask = "VPSDB JSON loaded";
            }
            LOG_INFO("VpsDatabaseLoader: Loaded vpsdb.json with " << vpsDb_.size() << " entries");
            return true;
        } else {
            LOG_ERROR("VpsDatabaseLoader: Invalid vpsdb.json: expected an array or an object with 'tables' array");
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("VpsDatabaseLoader: Failed to parse vpsdb.json: " << e.what());
        return false;
    }
}

const nlohmann::json& VpsDatabaseLoader::getVpsDb() const {
    return vpsDb_;
}