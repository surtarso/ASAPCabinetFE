/**
 * @file vps_database_loader.cpp
 * @brief Implements the VpsDatabaseLoader class for loading the VPS database in ASAPCabinetFE.
 *
 * This file provides the implementation of the VpsDatabaseLoader class, which loads
 * the VPS database (vpsdb.json) from a specified file path into a nlohmann::json object.
 * It validates the JSON structure, supports progress tracking via LoadingProgress, and
 * provides access to the loaded data. The loading process can be extended with configUI
 * for custom validation rules (e.g., additional JSON structure checks) in the future.
 */

#include "vps_database_loader.h"
#include <filesystem>
#include <fstream>
#include "log/logging.h"

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations

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

        vpsDb_ = nlohmann::json::parse(file, nullptr, true, true); // Parse JSON with comments and error tolerance
        file.close();

        if (vpsDb_.is_array()) {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = vpsDb_.size(); // Update with number of entries
                progress->currentTask = "VPSDB JSON loaded";
            }
            LOG_INFO("VpsDatabaseLoader: Loaded vpsdb.json with " << vpsDb_.size() << " entries");
            return true;
        } else if (vpsDb_.is_object() && vpsDb_.contains("tables") && vpsDb_["tables"].is_array()) {
            vpsDb_ = vpsDb_["tables"]; // Adjust to store the 'tables' array directly
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = vpsDb_.size(); // Update with number of entries
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