/**
 * @file table_override_manager.cpp
 * @brief Implements the TableOverrideManager class for handling per-table JSON overrides in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableOverrideManager class, which loads
 * override JSON files (<table_name>.json) from the same directory as VPX files and merges
 * user-specified TableData fields (e.g., title, playfieldVideo). It ensures minimal impact
 * on existing components and includes placeholders for future dynamic reloading and saving.
 */

#include "tables/table_override_manager.h"
#include "log/logging.h"
#include <filesystem>
#include <fstream>
#include <map>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string TableOverrideManager::getOverrideFilePath(const TableData& table) const {
    if (table.vpxFile.empty()) {
        LOG_ERROR("TableOverrideManager: Invalid vpxFile path for table: " << table.title);
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
    std::string overridePath = getOverrideFilePath(table);
    if (overridePath.empty()) {
        return;
    }

    if (!fs::exists(overridePath)) {
        LOG_DEBUG("TableOverrideManager: No override file found at: " << overridePath);
        return;
    }

    try {
        std::ifstream file(overridePath);
        if (!file.is_open()) {
            LOG_ERROR("TableOverrideManager: Failed to open override file: " << overridePath);
            return;
        }

        json overrides;
        file >> overrides;
        file.close();

        // Merge overrideable fields
        if (overrides.contains("title") && overrides["title"].is_string()) {
            table.title = overrides["title"].get<std::string>();
        }
        if (overrides.contains("manufacturer") && overrides["manufacturer"].is_string()) {
            table.manufacturer = overrides["manufacturer"].get<std::string>();
        }
        if (overrides.contains("year") && overrides["year"].is_string()) {
            table.year = overrides["year"].get<std::string>();
        }
        if (overrides.contains("playfieldImage") && overrides["playfieldImage"].is_string()) {
            table.playfieldImage = overrides["playfieldImage"].get<std::string>();
        }
        if (overrides.contains("wheelImage") && overrides["wheelImage"].is_string()) {
            table.wheelImage = overrides["wheelImage"].get<std::string>();
        }
        if (overrides.contains("backglassImage") && overrides["backglassImage"].is_string()) {
            table.backglassImage = overrides["backglassImage"].get<std::string>();
        }
        if (overrides.contains("dmdImage") && overrides["dmdImage"].is_string()) {
            table.dmdImage = overrides["dmdImage"].get<std::string>();
        }
        if (overrides.contains("topperImage") && overrides["topperImage"].is_string()) {
            table.topperImage = overrides["topperImage"].get<std::string>();
        }
        if (overrides.contains("playfieldVideo") && overrides["playfieldVideo"].is_string()) {
            table.playfieldVideo = overrides["playfieldVideo"].get<std::string>();
        }
        if (overrides.contains("backglassVideo") && overrides["backglassVideo"].is_string()) {
            table.backglassVideo = overrides["backglassVideo"].get<std::string>();
        }
        if (overrides.contains("dmdVideo") && overrides["dmdVideo"].is_string()) {
            table.dmdVideo = overrides["dmdVideo"].get<std::string>();
        }
        if (overrides.contains("topperVideo") && overrides["topperVideo"].is_string()) {
            table.topperVideo = overrides["topperVideo"].get<std::string>();
        }
        if (overrides.contains("music") && overrides["music"].is_string()) {
            table.music = overrides["music"].get<std::string>();
        }
        if (overrides.contains("launchAudio") && overrides["launchAudio"].is_string()) {
            table.launchAudio = overrides["launchAudio"].get<std::string>();
        }
        if (overrides.contains("tableName") && overrides["tableName"].is_string()) {
            table.tableName = overrides["tableName"].get<std::string>();
        }
        if (overrides.contains("tableAuthor") && overrides["tableAuthor"].is_string()) {
            table.tableAuthor = overrides["tableAuthor"].get<std::string>();
        }
        if (overrides.contains("tableDescription") && overrides["tableDescription"].is_string()) {
            table.tableDescription = overrides["tableDescription"].get<std::string>();
        }
        if (overrides.contains("tableSaveDate") && overrides["tableSaveDate"].is_string()) {
            table.tableSaveDate = overrides["tableSaveDate"].get<std::string>();
        }
        if (overrides.contains("tableLastModified") && overrides["tableLastModified"].is_string()) {
            table.tableLastModified = overrides["tableLastModified"].get<std::string>();
        }
        if (overrides.contains("tableReleaseDate") && overrides["tableReleaseDate"].is_string()) {
            table.tableReleaseDate = overrides["tableReleaseDate"].get<std::string>();
        }
        if (overrides.contains("tableVersion") && overrides["tableVersion"].is_string()) {
            table.tableVersion = overrides["tableVersion"].get<std::string>();
        }
        if (overrides.contains("tableRevision") && overrides["tableRevision"].is_string()) {
            table.tableRevision = overrides["tableRevision"].get<std::string>();
        }
        if (overrides.contains("tableBlurb") && overrides["tableBlurb"].is_string()) {
            table.tableBlurb = overrides["tableBlurb"].get<std::string>();
        }
        if (overrides.contains("tableRules") && overrides["tableRules"].is_string()) {
            table.tableRules = overrides["tableRules"].get<std::string>();
        }
        if (overrides.contains("tableAuthorEmail") && overrides["tableAuthorEmail"].is_string()) {
            table.tableAuthorEmail = overrides["tableAuthorEmail"].get<std::string>();
        }
        if (overrides.contains("tableAuthorWebsite") && overrides["tableAuthorWebsite"].is_string()) {
            table.tableAuthorWebsite = overrides["tableAuthorWebsite"].get<std::string>();
        }
        if (overrides.contains("tableType") && overrides["tableType"].is_string()) {
            table.tableType = overrides["tableType"].get<std::string>();
        }
        if (overrides.contains("tableManufacturer") && overrides["tableManufacturer"].is_string()) {
            table.tableManufacturer = overrides["tableManufacturer"].get<std::string>();
        }
        if (overrides.contains("tableYear") && overrides["tableYear"].is_string()) {
            table.tableYear = overrides["tableYear"].get<std::string>();
        }

        LOG_INFO("TableOverrideManager: Applied overrides for table: " << table.title << " from: " << overridePath);
    } catch (const json::exception& e) {
        LOG_ERROR("TableOverrideManager: JSON parsing error in override file: " << overridePath << ": " << e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("TableOverrideManager: Failed to load override file: " << overridePath << ": " << e.what());
    }
}

void TableOverrideManager::reloadOverrides(TableData& table) const {
    LOG_DEBUG("TableOverrideManager: reloadOverrides called for table: " << table.title << " (not implemented)");
}

void TableOverrideManager::saveOverride(const TableData& table, const std::map<std::string, std::string>& overrides) const {
    std::string overridePath = getOverrideFilePath(table);
    if (overridePath.empty()) {
        LOG_ERROR("TableOverrideManager: Cannot save override, invalid path for table: " << table.title);
        return;
    }

    try {
        // Load existing JSON to preserve unedited fields
        json overrideJson;
        if (fs::exists(overridePath)) {
            std::ifstream inFile(overridePath);
            if (!inFile.is_open()) {
                LOG_ERROR("TableOverrideManager: Failed to open override file for reading: " << overridePath);
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
            LOG_DEBUG("TableOverrideManager: No overrides to save, deleted file for table: " << table.title);
            return;
        }

        // Ensure parent directories exist
        fs::create_directories(fs::path(overridePath).parent_path());

        // Write updated JSON
        std::ofstream outFile(overridePath);
        if (!outFile.is_open()) {
            LOG_ERROR("TableOverrideManager: Failed to open override file for writing: " << overridePath);
            return;
        }
        outFile << overrideJson.dump(4);
        outFile.close();

        LOG_INFO("TableOverrideManager: Saved overrides for table: " << table.title << " to: " << overridePath);
    } catch (const json::exception& e) {
        LOG_ERROR("TableOverrideManager: JSON error while saving override file: " << overridePath << ": " << e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("TableOverrideManager: Failed to save override file: " << overridePath << ": " << e.what());
    }
}

void TableOverrideManager::deleteOverride(const TableData& table) const {
    std::string overridePath = getOverrideFilePath(table);
    if (overridePath.empty()) {
        LOG_ERROR("TableOverrideManager: Cannot delete override, invalid path for table: " << table.title);
        return;
    }

    if (fs::exists(overridePath)) {
        std::error_code ec;
        if (fs::remove(overridePath, ec)) {
            LOG_DEBUG("TableOverrideManager: Deleted override file: " << overridePath);
        } else {
            LOG_ERROR("TableOverrideManager: Failed to delete override file: " << overridePath << ": " << ec.message());
        }
    } else {
        LOG_DEBUG("TableOverrideManager: No override file to delete: " << overridePath);
    }
}