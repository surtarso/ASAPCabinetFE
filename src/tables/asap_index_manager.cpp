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
#include "log/logging.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations
using json = nlohmann::json; // Alias for nlohmann::json to simplify JSON usage

bool AsapIndexManager::load(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    if (!fs::exists(settings.indexPath)) {
        LOG_INFO("AsapIndexManager: asapcab_index.json not found at: " << settings.indexPath << ". Will create a new one on save.");
        return false;
    }

    try {
        std::ifstream indexFile(settings.indexPath);
        if (!indexFile.is_open()) {
            LOG_ERROR("AsapIndexManager: Failed to open " << settings.indexPath << " for reading.");
            return false;
        }

        json asapIndex;
        indexFile >> asapIndex; // Parse JSON from file
        indexFile.close();

        if (!asapIndex.contains("tables") || !asapIndex["tables"].is_array()) {
            LOG_ERROR("AsapIndexManager: Invalid asapcab_index.json: 'tables' key missing or not an array. Attempting to clear and rebuild index.");
            // Consider clearing the file or backing it up if it's malformed to prevent constant errors
            return false; // Indicate failure to load
        }

        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Loading tables from index...";
            progress->totalTablesToLoad = asapIndex["tables"].size();
            progress->currentTablesLoaded = 0;
        }

        tables.clear(); // Clear existing tables before loading
        for (const auto& table : asapIndex["tables"]) {
            TableData tableData;
            // ----------------- BEST MATCHES --------------------
            if (table.contains("title") && table["title"].is_string()) tableData.title = table["title"].get<std::string>();
            if (table.contains("manufacturer") && table["manufacturer"].is_string()) tableData.manufacturer = table["manufacturer"].get<std::string>();
            if (table.contains("year") && table["year"].is_string()) tableData.year = table["year"].get<std::string>();
            
            // ------------------ FILE PATHS ------------------
            if (table.contains("vpxFile") && table["vpxFile"].is_string()) tableData.vpxFile = table["vpxFile"].get<std::string>();
            if (table.contains("folder") && table["folder"].is_string()) tableData.folder = table["folder"].get<std::string>();
            if (table.contains("romPath") && table["romPath"].is_string()) tableData.romPath = table["romPath"].get<std::string>();
            if (table.contains("romName") && table["romName"].is_string()) tableData.romName = table["romName"].get<std::string>();
            if (table.contains("playfieldImage") && table["playfieldImage"].is_string()) tableData.playfieldImage = table["playfieldImage"].get<std::string>();
            if (table.contains("wheelImage") && table["wheelImage"].is_string()) tableData.wheelImage = table["wheelImage"].get<std::string>();
            if (table.contains("backglassImage") && table["backglassImage"].is_string()) tableData.backglassImage = table["backglassImage"].get<std::string>();
            if (table.contains("dmdImage") && table["dmdImage"].is_string()) tableData.dmdImage = table["dmdImage"].get<std::string>();
            if (table.contains("topperImage") && table["topperImage"].is_string()) tableData.topperImage = table["topperImage"].get<std::string>();
            if (table.contains("playfieldVideo") && table["playfieldVideo"].is_string()) tableData.playfieldVideo = table["playfieldVideo"].get<std::string>();
            if (table.contains("backglassVideo") && table["backglassVideo"].is_string()) tableData.backglassVideo = table["backglassVideo"].get<std::string>();
            if (table.contains("dmdVideo") && table["dmdVideo"].is_string()) tableData.dmdVideo = table["dmdVideo"].get<std::string>();
            if (table.contains("topperVideo") && table["topperVideo"].is_string()) tableData.topperVideo = table["topperVideo"].get<std::string>();
            if (table.contains("music") && table["music"].is_string()) tableData.music = table["music"].get<std::string>();
            if (table.contains("launchAudio") && table["launchAudio"].is_string()) tableData.launchAudio = table["launchAudio"].get<std::string>();
            
            // Boolean flags (ensure they are boolean type in JSON)
            if (table.contains("altSound") && table["altSound"].is_boolean()) tableData.altSound = table["altSound"].get<bool>();
            if (table.contains("altColor") && table["altColor"].is_boolean()) tableData.altColor = table["altColor"].get<bool>();
            if (table.contains("hasPup") && table["hasPup"].is_boolean()) tableData.hasPup = table["hasPup"].get<bool>();
            if (table.contains("hasAltMusic") && table["hasAltMusic"].is_boolean()) tableData.hasAltMusic = table["hasAltMusic"].get<bool>();
            if (table.contains("hasUltraDMD") && table["hasUltraDMD"].is_boolean()) tableData.hasUltraDMD = table["hasUltraDMD"].get<bool>();


            // ------------ FILE METADATA (vpin/vpxtool) -----------
            if (table.contains("tableName") && table["tableName"].is_string()) tableData.tableName = table["tableName"].get<std::string>();
            if (table.contains("tableAuthor") && table["tableAuthor"].is_string()) tableData.tableAuthor = table["tableAuthor"].get<std::string>();
            if (table.contains("tableDescription") && table["tableDescription"].is_string()) tableData.tableDescription = table["tableDescription"].get<std::string>();
            if (table.contains("tableSaveDate") && table["tableSaveDate"].is_string()) tableData.tableSaveDate = table["tableSaveDate"].get<std::string>();
            if (table.contains("tableLastModified") && table["tableLastModified"].is_string()) tableData.tableLastModified = table["tableLastModified"].get<std::string>();
            if (table.contains("tableReleaseDate") && table["tableReleaseDate"].is_string()) tableData.tableReleaseDate = table["tableReleaseDate"].get<std::string>();
            if (table.contains("tableVersion") && table["tableVersion"].is_string()) tableData.tableVersion = table["tableVersion"].get<std::string>();
            if (table.contains("tableRevision") && table["tableRevision"].is_string()) tableData.tableRevision = table["tableRevision"].get<std::string>();
            if (table.contains("tableBlurb") && table["tableBlurb"].is_string()) tableData.tableBlurb = table["tableBlurb"].get<std::string>();
            if (table.contains("tableRules") && table["tableRules"].is_string()) tableData.tableRules = table["tableRules"].get<std::string>();
            if (table.contains("tableAuthorEmail") && table["tableAuthorEmail"].is_string()) tableData.tableAuthorEmail = table["tableAuthorEmail"].get<std::string>();
            if (table.contains("tableAuthorWebsite") && table["tableAuthorWebsite"].is_string()) tableData.tableAuthorWebsite = table["tableAuthorWebsite"].get<std::string>();
            if (table.contains("tableType") && table["tableType"].is_string()) tableData.tableType = table["tableType"].get<std::string>();
            if (table.contains("tableManufacturer") && table["tableManufacturer"].is_string()) tableData.tableManufacturer = table["tableManufacturer"].get<std::string>();
            if (table.contains("tableYear") && table["tableYear"].is_string()) tableData.tableYear = table["tableYear"].get<std::string>();


            // --------------- VPSDB METADATA -------------
            if (table.contains("vpsId") && table["vpsId"].is_string()) tableData.vpsId = table["vpsId"].get<std::string>();
            if (table.contains("vpsName") && table["vpsName"].is_string()) tableData.vpsName = table["vpsName"].get<std::string>();
            if (table.contains("vpsType") && table["vpsType"].is_string()) tableData.vpsType = table["vpsType"].get<std::string>();
            if (table.contains("vpsThemes") && table["vpsThemes"].is_string()) tableData.vpsThemes = table["vpsThemes"].get<std::string>();
            if (table.contains("vpsDesigners") && table["vpsDesigners"].is_string()) tableData.vpsDesigners = table["vpsDesigners"].get<std::string>();
            if (table.contains("vpsPlayers") && table["vpsPlayers"].is_string()) tableData.vpsPlayers = table["vpsPlayers"].get<std::string>();
            if (table.contains("vpsIpdbUrl") && table["vpsIpdbUrl"].is_string()) tableData.vpsIpdbUrl = table["vpsIpdbUrl"].get<std::string>();
            if (table.contains("vpsVersion") && table["vpsVersion"].is_string()) tableData.vpsVersion = table["vpsVersion"].get<std::string>();
            if (table.contains("vpsAuthors") && table["vpsAuthors"].is_string()) tableData.vpsAuthors = table["vpsAuthors"].get<std::string>();
            if (table.contains("vpsFeatures") && table["vpsFeatures"].is_string()) tableData.vpsFeatures = table["vpsFeatures"].get<std::string>();
            if (table.contains("vpsComment") && table["vpsComment"].is_string()) tableData.vpsComment = table["vpsComment"].get<std::string>();
            if (table.contains("vpsManufacturer") && table["vpsManufacturer"].is_string()) tableData.vpsManufacturer = table["vpsManufacturer"].get<std::string>();
            if (table.contains("vpsYear") && table["vpsYear"].is_string()) tableData.vpsYear = table["vpsYear"].get<std::string>();
            if (table.contains("vpsTableImgUrl") && table["vpsTableImgUrl"].is_string()) tableData.vpsTableImgUrl = table["vpsTableImgUrl"].get<std::string>();
            if (table.contains("vpsTableUrl") && table["vpsTableUrl"].is_string()) tableData.vpsTableUrl = table["vpsTableUrl"].get<std::string>();
            if (table.contains("vpsB2SImgUrl") && table["vpsB2SImgUrl"].is_string()) tableData.vpsB2SImgUrl = table["vpsB2SImgUrl"].get<std::string>();
            if (table.contains("vpsB2SUrl") && table["vpsB2SUrl"].is_string()) tableData.vpsB2SUrl = table["vpsB2SUrl"].get<std::string>();
            if (table.contains("vpsFormat") && table["vpsFormat"].is_string()) tableData.vpsFormat = table["vpsFormat"].get<std::string>();

            
            // --------------- OPERATIONAL TAGS ------------------
            // Check for float type for confidence/score
            if (table.contains("matchConfidence") && table["matchConfidence"].is_number_float()) tableData.matchConfidence = table["matchConfidence"].get<float>();
            if (table.contains("jsonOwner") && table["jsonOwner"].is_string()) tableData.jsonOwner = table["jsonOwner"].get<std::string>();
            if (table.contains("playCount") && table["playCount"].is_number_integer()) tableData.playCount = table["playCount"].get<int>(); // Assuming playCount is a string for now

            tables.push_back(tableData);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded++;
            }
        }
        LOG_DEBUG("AsapIndexManager: Loaded " << tables.size() << " tables from asapcab_index.json");
        return !tables.empty();
    } catch (const json::exception& e) {
        LOG_ERROR("AsapIndexManager: JSON parsing error while loading asapcab_index.json: " << e.what() << ". File might be corrupt.");
        // Potentially rename/backup the corrupt file here
        return false;
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
        progress->totalTablesToLoad = tables.size(); // Use tables.size() for total
        progress->currentTablesLoaded = 0;
    }

    for (const auto& table : tables) {
        json tableJson;
        // ----------------- BEST MATCHES --------------------
        tableJson["title"] = table.title;
        tableJson["manufacturer"] = table.manufacturer;
        tableJson["year"] = table.year;
        
        // ------------------ FILE PATHS ------------------
        tableJson["vpxFile"] = table.vpxFile;
        tableJson["folder"] = table.folder;
        tableJson["romPath"] = table.romPath;
        tableJson["romName"] = table.romName;
        tableJson["playfieldImage"] = table.playfieldImage;
        tableJson["wheelImage"] = table.wheelImage;
        tableJson["backglassImage"] = table.backglassImage;
        tableJson["dmdImage"] = table.dmdImage;
        tableJson["topperImage"] = table.topperImage;
        tableJson["playfieldVideo"] = table.playfieldVideo;
        tableJson["backglassVideo"] = table.backglassVideo;
        tableJson["dmdVideo"] = table.dmdVideo;
        tableJson["topperVideo"] = table.topperVideo;
        tableJson["music"] = table.music;
        tableJson["launchAudio"] = table.launchAudio;
        
        // Boolean flags
        tableJson["altSound"] = table.altSound;
        tableJson["altColor"] = table.altColor;
        tableJson["hasPup"] = table.hasPup;
        tableJson["hasAltMusic"] = table.hasAltMusic; 
        tableJson["hasUltraDMD"] = table.hasUltraDMD; 


        // ------------ FILE METADATA (vpin/vpxtool) -----------
        tableJson["tableName"] = table.tableName;
        tableJson["tableAuthor"] = table.tableAuthor;
        tableJson["tableDescription"] = table.tableDescription;
        tableJson["tableSaveDate"] = table.tableSaveDate;
        tableJson["tableLastModified"] = table.tableLastModified;
        tableJson["tableReleaseDate"] = table.tableReleaseDate;
        tableJson["tableVersion"] = table.tableVersion;
        tableJson["tableRevision"] = table.tableRevision;
        tableJson["tableBlurb"] = table.tableBlurb;
        tableJson["tableRules"] = table.tableRules;
        tableJson["tableAuthorEmail"] = table.tableAuthorEmail;
        tableJson["tableAuthorWebsite"] = table.tableAuthorWebsite;
        tableJson["tableType"] = table.tableType; 
        tableJson["tableManufacturer"] = table.tableManufacturer;
        tableJson["tableYear"] = table.tableYear; 


        // --------------- VPSDB METADATA -------------
        tableJson["vpsId"] = table.vpsId;
        tableJson["vpsName"] = table.vpsName;
        tableJson["vpsType"] = table.vpsType;
        tableJson["vpsThemes"] = table.vpsThemes;
        tableJson["vpsDesigners"] = table.vpsDesigners;
        tableJson["vpsPlayers"] = table.vpsPlayers;
        tableJson["vpsIpdbUrl"] = table.vpsIpdbUrl;
        tableJson["vpsVersion"] = table.vpsVersion;
        tableJson["vpsAuthors"] = table.vpsAuthors;
        tableJson["vpsFeatures"] = table.vpsFeatures;
        tableJson["vpsComment"] = table.vpsComment;
        tableJson["vpsManufacturer"] = table.vpsManufacturer;
        tableJson["vpsYear"] = table.vpsYear;
        tableJson["vpsTableImgUrl"] = table.vpsTableImgUrl;
        tableJson["vpsTableUrl"] = table.vpsTableUrl;
        tableJson["vpsB2SImgUrl"] = table.vpsB2SImgUrl;
        tableJson["vpsB2SUrl"] = table.vpsB2SUrl;
        tableJson["vpsFormat"] = table.vpsFormat;

        // --------------- OPERATIONAL TAGS ------------------
        tableJson["matchConfidence"] = table.matchConfidence;
        tableJson["jsonOwner"] = table.jsonOwner;
        tableJson["playCount"] = table.playCount; 

        asapIndex["tables"].push_back(tableJson);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded++;
        }
    }

    try {
        // Ensure parent directories exist before writing the file
        fs::path outputPath = settings.indexPath;
        fs::create_directories(outputPath.parent_path()); 
        
        std::ofstream out(outputPath);
        if (!out.is_open()) {
            LOG_ERROR("AsapIndexManager: Failed to open " << settings.indexPath << " for writing. Check permissions.");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: Failed to open asapcab_index.json for writing. Check permissions.");
            }
            return false;
        }
        out << asapIndex.dump(4); // Serialize with 4-space indentation for readability
        out.close();
        LOG_INFO("AsapIndexManager: Saved " << tables.size() << " tables to asapcab_index.json");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("INFO: Saved " + std::to_string(tables.size()) + " tables to index.");
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("AsapIndexManager: Failed to save asapcab_index.json: " << e.what());
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("ERROR: Failed to save asapcab_index.json: " + std::string(e.what()));
        }
        return false;
    }
}