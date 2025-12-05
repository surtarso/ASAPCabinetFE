/**
 * @file table_override_manager.cpp
 * @brief Implements the TableOverrideManager class for handling per-table JSON overrides in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableOverrideManager class, which loads
 * override JSON files (<table_name>.json) from the same directory as VPX files and merges
 * user-specified TableData fields (e.g., title, playfieldVideo). It ensures minimal impact
 * on existing components and includes placeholders for future dynamic reloading and saving.
 */

#include "table_override_manager.h"
#include "log/logging.h"
#include <filesystem>
#include <fstream>
#include <map>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string TableOverrideManager::getOverrideFilePath(const TableData& table) const {
    if (table.vpxFile.empty()) {
        LOG_ERROR("Invalid vpxFile path for table: " + table.bestTitle);
        return "";
    }

    fs::path vpxPath = table.vpxFile;
    fs::path jsonPath = vpxPath.parent_path() / (vpxPath.stem().string() + ".json");

    return jsonPath.string();
}

bool TableOverrideManager::overrideFileExists(const TableData& table) const {
    std::string overridePath = getOverrideFilePath(table);
    return !overridePath.empty() && fs::exists(overridePath);
}

void TableOverrideManager::applyOverrides(TableData& table) const {
    std::string path = getOverrideFilePath(table);
    if (path.empty() || !fs::exists(path)){
        table.isManualVpsId = false;
        table.hasOverride = false;
        return;
    }

    try {
        json j = json::parse(std::ifstream(path));

        if (j.contains("bestTitle") && j["bestTitle"].is_string())
            table.bestTitle = j["bestTitle"];
        if (j.contains("bestManufacturer") && j["bestManufacturer"].is_string())
            table.bestManufacturer = j["bestManufacturer"];
        if (j.contains("bestYear") && j["bestYear"].is_string())
            table.bestYear = j["bestYear"];
        // ONLY if vpsId is present and non-empty → manual
        if (j.contains("vpsId") && j["vpsId"].is_string()) {
            std::string id = j["vpsId"];
            if (!id.empty()) {
                table.vpsId = id;
                table.isManualVpsId = true;
            } else {
                table.vpsId.clear();  // user explicitly cleared it
            }
        }

        LOG_INFO("Applied overrides for: " + table.bestTitle);
    } catch (...) {
        LOG_ERROR("Failed to parse override file: " + path);
        table.isManualVpsId = false;
        table.hasOverride = false;
    }
}

void TableOverrideManager::saveOverride(const TableData& table, const std::map<std::string, std::string>& overrides) const {
    std::string overridePath = getOverrideFilePath(table);
    if (overridePath.empty()) {
        LOG_ERROR("Cannot save override, invalid path for table: " + table.bestTitle);
        return;
    }

    try {
        // Load existing JSON to preserve unedited fields
        json overrideJson = json::object();
        if (fs::exists(overridePath)) {
            std::ifstream inFile(overridePath);
            if (!inFile.is_open()) {
                LOG_ERROR("Failed to open override file for reading: " + overridePath);
                return;
            }
            inFile >> overrideJson;
            inFile.close();
        }

        // Update only edited fields
        bool hasChanges = false;
        for (const auto& [key, value] : overrides) {
            if (!value.empty()) {
                overrideJson[key] = value;
                hasChanges = true;
            } else {
                overrideJson.erase(key);
            }
        }

        // If no changes and JSON is empty, delete the file
        if (!hasChanges && overrideJson.empty()) {
            deleteOverride(table);
            LOG_DEBUG("No overrides to save, deleted file for table: " + table.bestTitle);
            return;
        }

        // Ensure parent directories exist
        fs::create_directories(fs::path(overridePath).parent_path());

        // Write updated JSON
        std::ofstream outFile(overridePath);
        if (!outFile.is_open()) {
            LOG_ERROR("Failed to open override file for writing: " + overridePath);
            return;
        }
        outFile << overrideJson.dump(4);
        outFile.close();

        LOG_DEBUG("Saved overrides for table: " + table.bestTitle + " to: " + overridePath);
    } catch (const json::exception& e) {
        LOG_ERROR("JSON error while saving override file: " + overridePath + ": " + e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save override file: " + overridePath + ": " + e.what());
    }
}

void TableOverrideManager::deleteOverride(const TableData& table) const {
    std::string overridePath = getOverrideFilePath(table);
    if (overridePath.empty()) {
        LOG_ERROR("Cannot delete override, invalid path for table: " + table.bestTitle);
        return;
    }

    if (fs::exists(overridePath)) {
        std::error_code ec;
        if (fs::remove(overridePath, ec)) {
            LOG_DEBUG("Deleted override file: " + overridePath);
        } else {
            LOG_ERROR("Failed to delete override file: " + overridePath + ": " + ec.message());
        }
    } else {
        LOG_DEBUG("No override file to delete: " + overridePath);
    }
}
