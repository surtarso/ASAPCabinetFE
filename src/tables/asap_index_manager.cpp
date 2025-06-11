/**
 * @file asap_index_manager.cpp
 * @brief Implements the AsapIndexManager class for managing ASAP index files in ASAPCabinetFE.
 *
 * This file provides the implementation of the AsapIndexManager class, which loads and
 * saves table data from/to an ASAP index file (asapcab_index.json) using JSON
 * serialization. The manager handles file I/O, validates JSON structure, and supports
 * progress tracking via LoadingProgress. It is configurable via Settings (e.g., indexPath),
 * with potential for future configUI enhancements (e.g., custom index paths or formatting).
 */

#include "tables/asap_index_manager.h"
#include "utils/logging.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations
using json = nlohmann::json; // Alias for nlohmann::json to simplify JSON usage

bool AsapIndexManager::load(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    if (!fs::exists(settings.indexPath)) {
        LOG_INFO("AsapIndexManager: asapcab_index.json not found at: " << settings.indexPath);
        return false;
    }

    try {
        std::ifstream indexFile(settings.indexPath);
        json asapIndex;
        indexFile >> asapIndex; // Parse JSON from file
        indexFile.close();

        if (!asapIndex.contains("tables") || !asapIndex["tables"].is_array()) {
            LOG_ERROR("AsapIndexManager: Invalid asapcab_index.json : 'tables' missing or not an array");
            return false;
        }

        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Loading tables from index...";
            progress->totalTablesToLoad = asapIndex["tables"].size();
            progress->currentTablesLoaded = 0;
        }

        tables.clear();
        for (const auto& table : asapIndex["tables"]) {
            TableData tableData;
            if (table.contains("vpxFile") && table["vpxFile"].is_string()) tableData.vpxFile = table["vpxFile"].get<std::string>();
            if (table.contains("folder") && table["folder"].is_string()) tableData.folder = table["folder"].get<std::string>();
            if (table.contains("title") && table["title"].is_string()) tableData.title = table["title"].get<std::string>();
            if (table.contains("manufacturer") && table["manufacturer"].is_string()) tableData.manufacturer = table["manufacturer"].get<std::string>();
            if (table.contains("year") && table["year"].is_string()) tableData.year = table["year"].get<std::string>();
            if (table.contains("tableDescription") && table["tableDescription"].is_string()) tableData.tableDescription = table["tableDescription"].get<std::string>();
            if (table.contains("vpsId") && table["vpsId"].is_string()) tableData.vpsId = table["vpsId"].get<std::string>();
            if (table.contains("vpsName") && table["vpsName"].is_string()) tableData.vpsName = table["vpsName"].get<std::string>();
            if (table.contains("type") && table["type"].is_string()) tableData.type = table["type"].get<std::string>();
            if (table.contains("themes") && table["themes"].is_string()) tableData.themes = table["themes"].get<std::string>();
            if (table.contains("designers") && table["designers"].is_string()) tableData.designers = table["designers"].get<std::string>();
            if (table.contains("players") && table["players"].is_string()) tableData.players = table["players"].get<std::string>();
            if (table.contains("ipdbUrl") && table["ipdbUrl"].is_string()) tableData.ipdbUrl = table["ipdbUrl"].get<std::string>();
            if (table.contains("vpsVersion") && table["vpsVersion"].is_string()) tableData.vpsVersion = table["vpsVersion"].get<std::string>();
            if (table.contains("vpsAuthors") && table["vpsAuthors"].is_string()) tableData.vpsAuthors = table["vpsAuthors"].get<std::string>();
            if (table.contains("features") && table["features"].is_string()) tableData.features = table["features"].get<std::string>();
            if (table.contains("vpsComment") && table["vpsComment"].is_string()) tableData.vpsComment = table["vpsComment"].get<std::string>();
            if (table.contains("tableName") && table["tableName"].is_string()) tableData.tableName = table["tableName"].get<std::string>();
            if (table.contains("authorName") && table["authorName"].is_string()) tableData.authorName = table["authorName"].get<std::string>();
            if (table.contains("gameName") && table["gameName"].is_string()) tableData.gameName = table["gameName"].get<std::string>();
            if (table.contains("romPath") && table["romPath"].is_string()) tableData.romPath = table["romPath"].get<std::string>();
            if (table.contains("tableSaveDate") && table["tableSaveDate"].is_string()) tableData.tableSaveDate = table["tableSaveDate"].get<std::string>();
            if (table.contains("lastModified") && table["lastModified"].is_string()) tableData.lastModified = table["lastModified"].get<std::string>();
            if (table.contains("releaseDate") && table["releaseDate"].is_string()) tableData.releaseDate = table["releaseDate"].get<std::string>();
            if (table.contains("tableVersion") && table["tableVersion"].is_string()) tableData.tableVersion = table["tableVersion"].get<std::string>();
            if (table.contains("tableRevision") && table["tableRevision"].is_string()) tableData.tableRevision = table["tableRevision"].get<std::string>();
            if (table.contains("music") && table["music"].is_string()) tableData.music = table["music"].get<std::string>();
            if (table.contains("launchAudio") && table["launchAudio"].is_string()) tableData.launchAudio = table["launchAudio"].get<std::string>();
            if (table.contains("playfieldImage") && table["playfieldImage"].is_string()) tableData.playfieldImage = table["playfieldImage"].get<std::string>();
            if (table.contains("wheelImage") && table["wheelImage"].is_string()) tableData.wheelImage = table["wheelImage"].get<std::string>();
            if (table.contains("backglassImage") && table["backglassImage"].is_string()) tableData.backglassImage = table["backglassImage"].get<std::string>();
            if (table.contains("dmdImage") && table["dmdImage"].is_string()) tableData.dmdImage = table["dmdImage"].get<std::string>();
            if (table.contains("topperImage") && table["topperImage"].is_string()) tableData.topperImage = table["topperImage"].get<std::string>();
            if (table.contains("playfieldVideo") && table["playfieldVideo"].is_string()) tableData.playfieldVideo = table["playfieldVideo"].get<std::string>();
            if (table.contains("backglassVideo") && table["backglassVideo"].is_string()) tableData.backglassVideo = table["backglassVideo"].get<std::string>();
            if (table.contains("dmdVideo") && table["dmdVideo"].is_string()) tableData.dmdVideo = table["dmdVideo"].get<std::string>();
            if (table.contains("topperVideo") && table["topperVideo"].is_string()) tableData.topperVideo = table["topperVideo"].get<std::string>();
            tables.push_back(tableData);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded++;
            }
        }
        LOG_INFO("AsapIndexManager: Loaded " << tables.size() << " tables from asapcab_index.json");
        return !tables.empty();
    } catch (const std::exception& e) {
        LOG_ERROR("AsapIndexManager: Failed to load asapcab_index.json: " << e.what());
        return false;
    }
}

bool AsapIndexManager::save(const Settings& settings, const std::vector<TableData>& tables, LoadingProgress* progress) {
    json asapIndex;
    asapIndex["tables"] = json::array();

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Saving tables to index...";
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = 0;
    }

    for (const auto& table : tables) {
        json tableJson;
        tableJson["vpxFile"] = table.vpxFile;
        tableJson["folder"] = table.folder;
        tableJson["title"] = table.title;
        tableJson["manufacturer"] = table.manufacturer;
        tableJson["year"] = table.year;
        tableJson["tableDescription"] = table.tableDescription;
        tableJson["vpsId"] = table.vpsId;
        tableJson["vpsName"] = table.vpsName;
        tableJson["type"] = table.type;
        tableJson["themes"] = table.themes;
        tableJson["designers"] = table.designers;
        tableJson["players"] = table.players;
        tableJson["ipdbUrl"] = table.ipdbUrl;
        tableJson["vpsVersion"] = table.vpsVersion;
        tableJson["vpsAuthors"] = table.vpsAuthors;
        tableJson["features"] = table.features;
        tableJson["vpsComment"] = table.vpsComment;
        tableJson["tableName"] = table.tableName;
        tableJson["authorName"] = table.authorName;
        tableJson["gameName"] = table.gameName;
        tableJson["romPath"] = table.romPath;
        tableJson["tableSaveDate"] = table.tableSaveDate;
        tableJson["lastModified"] = table.lastModified;
        tableJson["releaseDate"] = table.releaseDate;
        tableJson["tableVersion"] = table.tableVersion;
        tableJson["tableRevision"] = table.tableRevision;
        tableJson["music"] = table.music;
        tableJson["launchAudio"] = table.launchAudio;
        tableJson["playfieldImage"] = table.playfieldImage;
        tableJson["wheelImage"] = table.wheelImage;
        tableJson["backglassImage"] = table.backglassImage;
        tableJson["dmdImage"] = table.dmdImage;
        tableJson["topperImage"] = table.topperImage;
        tableJson["playfieldVideo"] = table.playfieldVideo;
        tableJson["backglassVideo"] = table.backglassVideo;
        tableJson["dmdVideo"] = table.dmdVideo;
        tableJson["topperVideo"] = table.topperVideo;
        asapIndex["tables"].push_back(tableJson);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded++;
        }
    }

    try {
        fs::create_directories(fs::path(settings.indexPath).parent_path()); // Ensure parent directories exist
        std::ofstream out(settings.indexPath);
        if (!out.is_open()) {
            LOG_ERROR("AsapIndexManager: Failed to open " << settings.indexPath << " for writing");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("DEBUG: Failed to open asapcab_index.json for writing");
            }
            return false;
        }
        out << asapIndex.dump(4); // Serialize with 4-space indentation
        out.close();
        LOG_INFO("AsapIndexManager: Saved " << tables.size() << " tables to asapcab_index.json");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("DEBUG: Saved " + std::to_string(tables.size()) + " tables to index");
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("AsapIndexManager: Failed to save asapcab_index.json: " << e.what());
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("DEBUG: Failed to save asapcab_index.json: " + std::string(e.what()));
        }
        return false;
    }
}