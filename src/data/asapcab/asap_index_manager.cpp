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

#include "data/asapcab/asap_index_manager.h"
#include "utils/path_utils.h"
#include "log/logging.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations
using json = nlohmann::json; // Alias for nlohmann::json to simplify JSON usage

AsapIndexManager::AsapIndexManager(const Settings& settings) : settings_(settings) {}

bool AsapIndexManager::load(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    if (!fs::exists(settings.indexPath)) {
        LOG_DEBUG("asapcab_index.json not found at: " + settings.indexPath + ". Will create a new one on save.");
        return false;
    }

    try {
        std::ifstream indexFile(settings.indexPath);
        if (!indexFile.is_open()) {
            LOG_ERROR("Failed to open " + settings.indexPath + " for reading.");
            return false;
        }

        json asapIndex;
        indexFile >> asapIndex; // Parse JSON from file
        indexFile.close();

        if (!asapIndex.contains("tables") || !asapIndex["tables"].is_array()) {
            LOG_ERROR("Invalid asapcab_index.json: 'tables' key missing or not an array. Attempting to clear and rebuild index.");
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
            if (table.contains("flyerFront") && table["flyerFront"].is_string()) tableData.flyerFront = table["flyerFront"].get<std::string>();
            if (table.contains("flyerBack") && table["flyerBack"].is_string()) tableData.flyerBack = table["flyerBack"].get<std::string>();

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
            if (table.contains("tableRom") && table["tableRom"].is_string()) tableData.tableRom = table["tableRom"].get<std::string>();


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

            // Launchbox DB ID
            if (table.contains("lbdbID") && table["lbdbID"].is_string()) tableData.lbdbID = table["lbdbID"].get<std::string>();


            // --------------- OPERATIONAL TAGS ------------------
            if (table.contains("matchConfidence") && table["matchConfidence"].is_number_float()) tableData.matchConfidence = table["matchConfidence"].get<float>();
            if (table.contains("jsonOwner") && table["jsonOwner"].is_string()) tableData.jsonOwner = table["jsonOwner"].get<std::string>();
            if (table.contains("playCount") && table["playCount"].is_number_integer()) tableData.playCount = table["playCount"].get<int>();
            if (table.contains("playTimeLast") && table["playTimeLast"].is_number_float()) tableData.playTimeLast = table["playTimeLast"].get<float>();
            if (table.contains("playTimeTotal") && table["playTimeTotal"].is_number_float()) tableData.playTimeTotal = table["playTimeTotal"].get<float>();
            if (table.contains("isBroken") && table["isBroken"].is_boolean()) tableData.isBroken = table["isBroken"].get<bool>();
            if (table.contains("fileLastModified") && table["fileLastModified"].is_number_unsigned()) tableData.fileLastModified = table["fileLastModified"].get<uint64_t>();
            if (table.contains("folderLastModified") && table["folderLastModified"].is_number_unsigned()) tableData.folderLastModified = table["folderLastModified"].get<uint64_t>();
            if (table.contains("hashFromVpx") && table["hashFromVpx"].is_string()) tableData.hashFromVpx = table["hashFromVpx"].get<std::string>();
            if (table.contains("hashFromVbs") && table["hashFromVbs"].is_string()) tableData.hashFromVbs = table["hashFromVbs"].get<std::string>();
            if (table.contains("hasDiffVbs") && table["hasDiffVbs"].is_boolean()) tableData.hasDiffVbs = table["hasDiffVbs"].get<bool>();
            if (table.contains("isPatched") && table["isPatched"].is_boolean()) tableData.isPatched = table["isPatched"].get<bool>();
            // EXTRA FILES SCAN Boolean flags (ensure they are boolean type in JSON)
            if (table.contains("hasAltSound") && table["hasAltSound"].is_boolean()) tableData.hasAltSound = table["hasAltSound"].get<bool>();
            if (table.contains("hasAltColor") && table["hasAltColor"].is_boolean()) tableData.hasAltColor = table["hasAltColor"].get<bool>();
            if (table.contains("hasPup") && table["hasPup"].is_boolean()) tableData.hasPup = table["hasPup"].get<bool>();
            if (table.contains("hasAltMusic") && table["hasAltMusic"].is_boolean()) tableData.hasAltMusic = table["hasAltMusic"].get<bool>();
            if (table.contains("hasUltraDMD") && table["hasUltraDMD"].is_boolean()) tableData.hasUltraDMD = table["hasUltraDMD"].get<bool>();
            if (table.contains("hasB2S") && table["hasB2S"].is_boolean()) tableData.hasB2S = table["hasB2S"].get<bool>();
            if (table.contains("hasINI") && table["hasINI"].is_boolean()) tableData.hasINI = table["hasINI"].get<bool>();
            if (table.contains("hasVBS") && table["hasVBS"].is_boolean()) tableData.hasVBS = table["hasVBS"].get<bool>();
            if (table.contains("hasOverride") && table["hasOverride"].is_boolean()) tableData.hasOverride = table["hasOverride"].get<bool>();
            // MEDIA SCAN (Boolean flags)
            if (table.contains("hasPlayfieldImage") && table["hasPlayfieldImage"].is_boolean()) tableData.hasPlayfieldImage = table["hasPlayfieldImage"].get<bool>();
            if (table.contains("hasWheelImage") && table["hasWheelImage"].is_boolean()) tableData.hasWheelImage = table["hasWheelImage"].get<bool>();
            if (table.contains("hasBackglassImage") && table["hasBackglassImage"].is_boolean()) tableData.hasBackglassImage = table["hasBackglassImage"].get<bool>();
            if (table.contains("hasDmdImage") && table["hasDmdImage"].is_boolean()) tableData.hasDmdImage = table["hasDmdImage"].get<bool>();
            if (table.contains("hasTopperImage") && table["hasTopperImage"].is_boolean()) tableData.hasTopperImage = table["hasTopperImage"].get<bool>();
            if (table.contains("hasPlayfieldVideo") && table["hasPlayfieldVideo"].is_boolean()) tableData.hasPlayfieldVideo = table["hasPlayfieldVideo"].get<bool>();
            if (table.contains("hasBackglassVideo") && table["hasBackglassVideo"].is_boolean()) tableData.hasBackglassVideo = table["hasBackglassVideo"].get<bool>();
            if (table.contains("hasDmdVideo") && table["hasDmdVideo"].is_boolean()) tableData.hasDmdVideo = table["hasDmdVideo"].get<bool>();
            if (table.contains("hasTopperVideo") && table["hasTopperVideo"].is_boolean()) tableData.hasTopperVideo = table["hasTopperVideo"].get<bool>();
            if (table.contains("hasTableMusic") && table["hasTableMusic"].is_boolean()) tableData.hasTableMusic = table["hasTableMusic"].get<bool>();
            if (table.contains("hasLaunchAudio") && table["hasLaunchAudio"].is_boolean()) tableData.hasLaunchAudio = table["hasLaunchAudio"].get<bool>();
            if (table.contains("hasFlyerFront") && table["hasFlyerFront"].is_boolean()) tableData.hasFlyerFront = table["hasFlyerFront"].get<bool>();
            if (table.contains("hasFlyerBack") && table["hasFlyerBack"].is_boolean()) tableData.hasFlyerBack = table["hasFlyerBack"].get<bool>();


            tables.push_back(tableData);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded++;
            }
        }
        LOG_DEBUG("Loaded " + std::to_string(tables.size()) + " tables from asapcab_index.json");
        return !tables.empty();
    } catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error while loading asapcab_index.json: " + std::string(e.what()) + ". File might be corrupt.");
        // Potentially rename/backup the corrupt file here
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load asapcab_index.json: " + std::string(e.what()));
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
        tableJson["flyerFront"] = table.flyerFront;
        tableJson["flyerBack"] = table.flyerBack;

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
        tableJson["tableRom"] = table.tableRom;


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

        // Launchbox ID
        tableJson["lbdbID"] = table.lbdbID;

        // --------------- OPERATIONAL TAGS ------------------
        tableJson["matchConfidence"] = table.matchConfidence;
        tableJson["jsonOwner"] = table.jsonOwner;
        tableJson["playCount"] = table.playCount;
        tableJson["playTimeLast"] = table.playTimeLast;
        tableJson["playTimeTotal"] = table.playTimeTotal;
        tableJson["isBroken"] = table.isBroken;
        tableJson["fileLastModified"] = table.fileLastModified;
        tableJson["folderLastModified"] = table.folderLastModified;
        tableJson["hashFromVpx"] = table.hashFromVpx;
        tableJson["hashFromVbs"] = table.hashFromVbs;
        tableJson["hasDiffVbs"] = table.hasDiffVbs;
        tableJson["isPatched"] = table.isPatched;
        // EXTRA FILE SCAN (Boolean flags)
        tableJson["hasAltSound"] = table.hasAltSound;
        tableJson["hasAltColor"] = table.hasAltColor;
        tableJson["hasPup"] = table.hasPup;
        tableJson["hasAltMusic"] = table.hasAltMusic;
        tableJson["hasUltraDMD"] = table.hasUltraDMD;
        tableJson["hasB2S"] = table.hasB2S;
        tableJson["hasINI"] = table.hasINI;
        tableJson["hasVBS"] = table.hasVBS;
        tableJson["hasOverride"] = table.hasOverride;
        // MEDIA SCAN (Boolean flags)
        tableJson["hasPlayfieldImage"] = table.hasPlayfieldImage;
        tableJson["hasWheelImage"] = table.hasWheelImage;
        tableJson["hasBackglassImage"] = table.hasBackglassImage;
        tableJson["hasDmdImage"] = table.hasDmdImage;
        tableJson["hasTopperImage"] = table.hasTopperImage;
        tableJson["hasPlayfieldVideo"] = table.hasPlayfieldVideo;
        tableJson["hasBackglassVideo"] = table.hasBackglassVideo;
        tableJson["hasDmdVideo"] = table.hasDmdVideo;
        tableJson["hasTopperVideo"] = table.hasTopperVideo;
        tableJson["hasTableMusic"] = table.hasTableMusic;
        tableJson["hasLaunchAudio"] = table.hasLaunchAudio;
        tableJson["hasFlyerFront"] = table.hasFlyerFront;
        tableJson["hasFlyerBack"] = table.hasFlyerBack;

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
            LOG_ERROR("Failed to open " + settings.indexPath + " for writing. Check permissions.");
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->logMessages.push_back("ERROR: Failed to open asapcab_index.json for writing. Check permissions.");
            }
            return false;
        }
        out << asapIndex.dump(4); // Serialize with 4-space indentation for readability
        out.close();
        LOG_INFO("Saved " + std::to_string(tables.size()) + " tables to asapcab_index.json");
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("INFO: Saved " + std::to_string(tables.size()) + " tables to index.");
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save asapcab_index.json: " + std::string(e.what()));
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->logMessages.push_back("ERROR: Failed to save asapcab_index.json: " + std::string(e.what()));
        }
        return false;
    }
}

std::vector<TableData> AsapIndexManager::mergeTables(const Settings& settings, const std::vector<TableData>& newTables, LoadingProgress* progress) {
    std::vector<TableData> existingTables;
    std::unordered_map<std::string, TableData> existingTableMap;

    // Load existing index
    if (load(settings, existingTables, progress)) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Building existing table map...";
        }
        for (const auto& table : existingTables) {
            if (!table.vpxFile.empty()) {
                existingTableMap[table.vpxFile] = table;
            }
        }
    } else {
        LOG_WARN("Treating all tables as new.");
    }

    // Define jsonOwner priority
    auto getJsonOwnerPriority = [](const std::string& owner) -> int {
        if (owner == "Virtual Pinball Spreadsheet Database") return 3;
        if (owner == "VPin Filescan" || owner == "VPXTool Index") return 2;
        if (owner == "System File Scan") return 1;
        return 0; // Unknown owner
    };

    std::vector<TableData> mergedTables;
    std::unordered_map<std::string, bool> processedNewTables;

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Merging tables...";
        progress->totalTablesToLoad = newTables.size();
        progress->currentTablesLoaded = 0;
    }

    // Process new tables
    for (const auto& newTable : newTables) {
        if (newTable.vpxFile.empty()) {
            LOG_WARN("Skipping new table with empty vpxFile.");
            continue;
        }

        processedNewTables[newTable.vpxFile] = true;
        auto existingIt = existingTableMap.find(newTable.vpxFile);
        TableData mergedTable = newTable;

        if (existingIt != existingTableMap.end()) {
            const auto& existingTable = existingIt->second;
            int existingPriority = getJsonOwnerPriority(existingTable.jsonOwner);
            int newPriority = getJsonOwnerPriority(newTable.jsonOwner);

            bool shouldUpdate = false;
            std::string updateReason;

            // Check for file modifications
            if (newTable.fileLastModified > existingTable.fileLastModified) {
                shouldUpdate = true;
                updateReason = "file modified (newer timestamp)";
            } else if (newTable.hashFromVpx != existingTable.hashFromVpx || newTable.hashFromVbs != existingTable.hashFromVbs) {
                shouldUpdate = true;
                updateReason = "file modified (different hashes)";
            }
            // Check for higher-quality metadata
            else if (newPriority > existingPriority) {
                shouldUpdate = true;
                updateReason = "higher-quality metadata (new owner: " + newTable.jsonOwner + ")";
            }

            if (shouldUpdate) {
                LOG_INFO("Updating table " + newTable.vpxFile + " due to " + updateReason);
                // Preserve user fields
                mergedTable.playCount = existingTable.playCount;
                mergedTable.playTimeLast = existingTable.playTimeLast;
                mergedTable.playTimeTotal = existingTable.playTimeTotal;
                mergedTable.isBroken = existingTable.isBroken;
            } else {
                LOG_DEBUG("Keeping existing table " + newTable.vpxFile + " (no update needed)");
                mergedTable = existingTable;
                mergedTable.fileLastModified = newTable.fileLastModified; // Update timestamp to match scan
            }

        } else {
            LOG_INFO("Adding new table " + newTable.vpxFile);
        }

        // ---------------- RECHECK LINKED FILES AND MEDIA FLAGS ----------------
        mergedTable.hasINI      = PathUtils::hasIniForTable(mergedTable.folder, fs::path(mergedTable.vpxFile).stem().string());
        mergedTable.hasB2S      = PathUtils::hasB2SForTable(mergedTable.folder, fs::path(mergedTable.vpxFile).stem().string());
        mergedTable.hasPup      = PathUtils::getPupPath(mergedTable.folder);
        mergedTable.hasAltColor = PathUtils::getAltcolorPath(mergedTable.folder);
        mergedTable.hasAltSound = PathUtils::getAltsoundPath(mergedTable.folder);
        mergedTable.hasAltMusic = PathUtils::getAltMusic(mergedTable.folder);
        mergedTable.hasUltraDMD = PathUtils::getUltraDmdPath(mergedTable.folder);


        mergedTables.push_back(mergedTable);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded++;
            progress->logMessages.push_back("Merged table: " + newTable.vpxFile);
        }
    }

    // Check for deleted tables
    for (const auto& [vpxFile, existingTable] : existingTableMap) {
        if (processedNewTables.find(vpxFile) == processedNewTables.end()) {
            if (!fs::exists(vpxFile)) {
                LOG_INFO("Removing deleted table " + vpxFile);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->logMessages.push_back("Removed deleted table: " + vpxFile);
                }
            } else {
                LOG_DEBUG("Keeping existing table " + vpxFile + " (not in new scan but file exists)");
                mergedTables.push_back(existingTable);
            }
        }
    }

    LOG_DEBUG("Merged " + std::to_string(mergedTables.size()) + " tables");
    return mergedTables;
}
