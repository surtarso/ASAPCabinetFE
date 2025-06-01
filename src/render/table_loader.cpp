#include "table_loader.h"
#include "utils/logging.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include "json.hpp"

namespace fs = std::filesystem;

std::vector<TableData> TableLoader::loadTableList(const Settings& settings) {
    std::vector<TableData> tables;

    if (settings.VPXTablesPath.empty() || !fs::exists(settings.VPXTablesPath)) {
        LOG_ERROR("TableLoader: Invalid or empty VPX tables path: " << settings.VPXTablesPath);
        return tables;
    }

    // Load cached ASAP index if metadata is enabled
    if (settings.titleSource == "metadata") {
        if (loadAsapIndex(settings, tables)) {
            LOG_INFO("TableLoader: Loaded " << tables.size() << " tables from asapcabinetfe_index.json");
            // **SORTING STEP HERE**
            std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
                return a.title < b.title;
            });
            LOG_DEBUG("TableLoader: Re-sorted tables after loading from asapcabinetfe_index.json");
            // Rebuild letter index after sorting
            letterIndex.clear();
            for (size_t i = 0; i < tables.size(); ++i) {
                char firstChar = tables[i].title[0];
                if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
                    char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
                    if (letterIndex.find(key) == letterIndex.end()) {
                        letterIndex[key] = i;
                    }
                }
            }
            return tables;
        } else {
            LOG_INFO("TableLoader: asapcabinetfe_index.json not found or failed to load, scanning VPX files and building from scratch.");
        }
    }

    // Original scanning logic (only reached if ASAP index not loaded or not in metadata mode)
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            TableData table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.title = entry.path().stem().string(); // Initial title from filename stem
            table.music = getMusicPath(table.folder, settings.tableMusic);
            table.playfieldImage = getImagePath(table.folder, settings.customPlayfieldImage, settings.defaultPlayfieldImage);
            table.wheelImage = getImagePath(table.folder, settings.customWheelImage, settings.defaultWheelImage);
            table.backglassImage = getImagePath(table.folder, settings.customBackglassImage, settings.defaultBackglassImage);
            table.dmdImage = getImagePath(table.folder, settings.customDmdImage, settings.defaultDmdImage);
            table.topperImage = getImagePath(table.folder, settings.customTopperImage, settings.defaultTopperImage);
            if (settings.forceImagesOnly) {
                table.playfieldVideo = "";
                table.backglassVideo = "";
                table.dmdVideo = "";
                table.topperVideo = "";
            } else {
                table.playfieldVideo = getVideoPath(table.folder, settings.customPlayfieldVideo, settings.defaultPlayfieldVideo);
                table.backglassVideo = getVideoPath(table.folder, settings.customBackglassVideo, settings.defaultBackglassVideo);
                table.dmdVideo = getVideoPath(table.folder, settings.customDmdVideo, settings.defaultDmdVideo);
                table.topperVideo = getVideoPath(table.folder, settings.customTopperVideo, settings.defaultTopperVideo);
            }
            tables.push_back(table);
        }
    }

    // Load vpxtool and VPS metadata if titleSource=metadata
    if (settings.titleSource == "metadata") {
        std::string jsonPath = settings.VPXTablesPath + settings.vpxtoolIndex;
        if (fs::exists(jsonPath)) {
            try {
                std::ifstream file(jsonPath);
                nlohmann::json vpxtoolJson;
                file >> vpxtoolJson;
                file.close();

                // Initialize VPS client
                VpsDatabaseClient vpsClient(settings.vpsDbPath);
                bool vpsLoaded = false;
                if (vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency) && vpsClient.load()) {
                    vpsLoaded = true;
                } else {
                    LOG_ERROR("TableLoader: Failed to load vpsdb.json, using vpxtool only");
                }

                if (!vpxtoolJson.contains("tables") || !vpxtoolJson["tables"].is_array()) {
                    LOG_ERROR("TableLoader: Invalid vpxtool_index.json: 'tables' missing or not an array");
                } else {
                    LOG_DEBUG("TableLoader: vpxtoolJson['tables'] type: " << vpxtoolJson["tables"].type_name());
                    for (size_t i = 0; i < vpxtoolJson["tables"].size(); ++i) {
                        try {
                            const auto& tableJson = vpxtoolJson["tables"][i];
                            if (!tableJson.is_object()) {
                                LOG_DEBUG("TableLoader: Skipping invalid entry at index " << i << ": not an object, type: " << tableJson.type_name());
                                continue;
                            }

                            std::string path;
                            if (tableJson.contains("path") && tableJson["path"].is_string()) {
                                path = tableJson["path"].get<std::string>();
                            } else {
                                LOG_DEBUG("TableLoader: Skipping table entry with missing or non-string path at index " << i);
                                continue;
                            }
                            if (path.empty()) {
                                LOG_DEBUG("TableLoader: Skipping table with empty path at index " << i);
                                continue;
                            }

                            for (auto& table : tables) {
                                if (table.vpxFile == path) {
                                    // Populate tableData from vpxtool_index.json, with robust checks
                                    if (tableJson.contains("table_info") && tableJson["table_info"].is_object()) {
                                        const auto& tableInfo = tableJson["table_info"];

                                        if (tableInfo.contains("table_name") && tableInfo["table_name"].is_string()) {
                                            table.tableName = tableInfo["table_name"].get<std::string>();
                                        } else {
                                            table.tableName = table.title; // Fallback to filename stem if vpxtool table_name is bad
                                        }
                                        if (table.tableName.empty()) { // Ensure tableName is never empty
                                            table.tableName = table.title;
                                        }
                                        // REMOVED: table.title = table.tableName; // This line was causing issues by overwriting filename stem
                                                                               // with potentially generic vpxtool table_name.
                                                                               // VpsDatabaseClient now handles setting table.title.

                                        if (tableInfo.contains("author_name") && tableInfo["author_name"].is_string()) {
                                            table.authorName = tableInfo["author_name"].get<std::string>();
                                        } else {
                                            table.authorName = "";
                                        }

                                        if (tableInfo.contains("table_description") && tableInfo["table_description"].is_string()) {
                                            table.tableDescription = tableInfo["table_description"].get<std::string>();
                                        } else {
                                            table.tableDescription = "";
                                        }

                                        if (tableInfo.contains("table_save_date") && tableInfo["table_save_date"].is_string()) {
                                            table.tableSaveDate = tableInfo["table_save_date"].get<std::string>();
                                        } else {
                                            table.tableSaveDate = "";
                                        }

                                        if (tableInfo.contains("release_date") && tableInfo["release_date"].is_string()) {
                                            table.releaseDate = tableInfo["release_date"].get<std::string>();
                                        } else {
                                            table.releaseDate = "";
                                        }

                                        // Robustly get table_version, handling string or number types
                                        if (tableInfo.contains("table_version")) {
                                            if (tableInfo["table_version"].is_string()) {
                                                table.tableVersion = tableInfo["table_version"].get<std::string>();
                                            } else if (tableInfo["table_version"].is_number()) {
                                                table.tableVersion = std::to_string(tableInfo["table_version"].get<double>());
                                            } else {
                                                table.tableVersion = "";
                                            }
                                        } else {
                                            table.tableVersion = "";
                                        }

                                        if (tableInfo.contains("table_save_rev") && tableInfo["table_save_rev"].is_string()) {
                                            table.tableRevision = tableInfo["table_save_rev"].get<std::string>();
                                        } else {
                                            table.tableRevision = "";
                                        }
                                    } else {
                                        LOG_DEBUG("TableLoader: 'table_info' missing or malformed for VPX file: " << path << ". Title remains filename stem.");
                                        // table.tableName will remain filename stem
                                        // Other tableInfo fields will remain empty
                                    }

                                    if (tableJson.contains("game_name") && tableJson["game_name"].is_string()) {
                                        table.gameName = tableJson["game_name"].get<std::string>();
                                    } else {
                                        table.gameName = "";
                                    }

                                    if (tableJson.contains("rom_path") && tableJson["rom_path"].is_string()) {
                                        table.romPath = tableJson["rom_path"].get<std::string>();
                                    } else {
                                        table.romPath = "";
                                    }

                                    if (tableJson.contains("last_modified") && tableJson["last_modified"].is_string()) {
                                        table.lastModified = tableJson["last_modified"].get<std::string>();
                                    } else {
                                        table.lastModified = "";
                                    }

                                    // Year and manufacturer parsing (now using the possibly updated tableName/releaseDate)
                                    if (table.year.empty() && !table.releaseDate.empty()) {
                                        std::regex dateRegex_YYYY("\\d{4}"); // Matches 4-digit year
                                        std::regex dateRegex_DDMMYYYY("\\d{2}\\.\\d{2}\\.(\\d{4})"); // Matches DD.MM.YYYY
                                        std::smatch match;

                                        if (std::regex_search(table.releaseDate, match, dateRegex_DDMMYYYY)) {
                                            table.year = match[1].str();
                                        } else if (std::regex_search(table.releaseDate, match, dateRegex_YYYY)) {
                                            table.year = match[0].str();
                                        }
                                    }
                                    // Fallback year extraction from table.tableName if still empty
                                    if (table.year.empty() && !table.tableName.empty()) {
                                        std::regex yearFromTableNameRegex("\\((\\d{4})\\)");
                                        std::smatch match;
                                        if (std::regex_search(table.tableName, match, yearFromTableNameRegex)) {
                                            table.year = match[1].str();
                                        }
                                    }

                                    if (table.manufacturer.empty() && !table.tableName.empty()) {
                                        std::regex manufFromTableNameRegex("\\(([^)]+?)(?:\\s+\\d{4})?\\)"); // Catches " (Manufacturer)" or " (Manufacturer Year)"
                                        std::smatch match;
                                        if (std::regex_search(table.tableName, match, manufFromTableNameRegex)) {
                                            table.manufacturer = match[1].str();
                                        }
                                    }

                                    // VPS enrichment
                                    // The vpsClient.enrichTableData will set table.title based on the best VPSDB match.
                                    if (vpsLoaded && vpsClient.enrichTableData(tableJson, table)) {
                                        LOG_DEBUG("TableLoader: Enriched table: " << table.title);
                                    }
                                    break; // Found matching tableData, break inner loop
                                }
                            }
                        } catch (const nlohmann::json::exception& e) {
                            LOG_DEBUG("TableLoader: JSON parsing error processing table at index " << i << " from vpxtool_index.json: " << e.what());
                            // Log the error and continue to the next table, as one bad entry shouldn't halt everything.
                            continue;
                        } catch (const std::exception& e) {
                            LOG_DEBUG("TableLoader: General error processing table at index " << i << " from vpxtool_index.json: " << e.what());
                            continue;
                        }
                    }
                }

                // Save ASAP index
                if (saveAsapIndex(settings, tables)) {
                    LOG_INFO("TableLoader: Saved " << tables.size() << " tables to asapcabinetfe_index.json");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("TableLoader: Failed to parse vpxtool_index.json: " << e.what());
                if (saveAsapIndex(settings, tables)) {
                    LOG_INFO("TableLoader: Saved " << tables.size() << " tables to asapcabinetfe_index.json despite vpxtool error");
                }
            }
        } else {
            LOG_INFO("TableLoader: vpxtool_index.json not found at: " << jsonPath);
        }
    }

    // Sort and build letter index (ALWAYS perform this after full data loading/enrichment)
    std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
        return a.title < b.title;
    });
    letterIndex.clear();
    for (size_t i = 0; i < tables.size(); ++i) {
        char firstChar = tables[i].title[0];
        if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = i;
            }
        }
    }

    return tables;
}

bool TableLoader::loadAsapIndex(const Settings& settings, std::vector<TableData>& tables) {
    if (!fs::exists(settings.indexPath)) {
        LOG_INFO("TableLoader: asapcabinetfe_index.json not found at: " << settings.indexPath);
        return false;
    }

    try {
        std::ifstream indexFile(settings.indexPath);
        nlohmann::json asapIndex;
        indexFile >> asapIndex;
        indexFile.close();

        if (!asapIndex.contains("tables") || !asapIndex["tables"].is_array()) {
            LOG_ERROR("TableLoader: Invalid asapcabinetfe_index.json: 'tables' missing or not an array");
            return false;
        }

        tables.clear(); // Clear existing tables before loading from index
        for (const auto& table : asapIndex["tables"]) {
            TableData tableData;
            try {
                // Use explicit checks for safer loading from ASAP index
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
                // Load media paths
                if (table.contains("music") && table["music"].is_string()) tableData.music = table["music"].get<std::string>();
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
            } catch (const std::exception& e) {
                LOG_DEBUG("TableLoader: Skipping invalid ASAP index entry: " << e.what());
                continue;
            }
        }
        LOG_INFO("TableLoader: Loaded " << tables.size() << " tables from asapcabinetfe_index.json");
        return !tables.empty();
    } catch (const std::exception& e) {
        LOG_ERROR("TableLoader: Failed to load asapcabinetfe_index.json: " << e.what());
        return false;
    }
}

bool TableLoader::saveAsapIndex(const Settings& settings, const std::vector<TableData>& tables) {
    nlohmann::json asapIndex;
    asapIndex["tables"] = nlohmann::json::array();

    for (const auto& table : tables) { // This loop already iterates the *sorted* vector
        nlohmann::json tableJson;
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
        // Add media paths
        tableJson["music"] = table.music;
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
    }

    try {
        fs::create_directories(fs::path(settings.indexPath).parent_path());
        std::ofstream out(settings.indexPath);
        if (!out.is_open()) {
            LOG_ERROR("TableLoader: Failed to open " << settings.indexPath << " for writing");
            return false;
        }
        out << asapIndex.dump(4);
        out.close();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("TableLoader: Failed to save asapcabinetfe_index.json: " << e.what());
        return false;
    }
}

std::string TableLoader::getImagePath(const std::string& root, const std::string& imagePath,
                                     const std::string& defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath;
    if (fs::exists(imageFile)) {
        return imageFile.string();
    }
    if (!fs::exists(defaultImagePath)) {
        LOG_ERROR("TableLoader: Default image not found: " << defaultImagePath);
    }
    return defaultImagePath;
}

std::string TableLoader::getVideoPath(const std::string& root, const std::string& videoPath,
                                     const std::string& defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile)) {
        return videoFile.string();
    }
    if (fs::exists(defaultVideoPath)) {
        return defaultVideoPath;
    }
    return "";
}

std::string TableLoader::getMusicPath(const std::string& root, const std::string& musicPath) {
    if (musicPath.empty()) {
        LOG_DEBUG("TableLoader: Music path from settings is empty for root: " << root);
        return "";
    }

    fs::path musicFile = fs::path(root) / musicPath;
    if (fs::exists(musicFile) && fs::is_regular_file(musicFile)) {
        LOG_DEBUG("TableLoader: Found Music: " << musicFile.string());
        return musicFile.string();
    }

    LOG_DEBUG("TableLoader: No music file found or not a regular file for: " << musicFile.string());
    return "";
}