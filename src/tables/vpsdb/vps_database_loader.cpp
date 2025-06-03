#include "vps_database_loader.h"
#include <fstream>
#include "utils/logging.h"

VpsDatabaseLoader::VpsDatabaseLoader(const std::string& vpsDbPath) : vpsDbPath_(vpsDbPath) {}

bool VpsDatabaseLoader::load() {
    try {
        std::ifstream file(vpsDbPath_);
        if (!file.is_open()) {
            LOG_ERROR("VpsDatabaseLoader: Failed to open vpsdb.json at: " << vpsDbPath_);
            return false;
        }
        vpsDb_ = nlohmann::json::parse(file);
        if (vpsDb_.is_array()) {
            LOG_INFO("VpsDatabaseLoader: Loaded vpsdb.json with " << vpsDb_.size() << " entries");
            return true;
        } else if (vpsDb_.is_object() && vpsDb_.contains("tables") && vpsDb_["tables"].is_array()) {
            vpsDb_ = vpsDb_["tables"];
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