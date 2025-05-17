#include "render/table_loader.h"
#include "utils/logging.h"
#include <json.hpp>
#include <regex>
#include <algorithm>
#include <cctype>

std::vector<TableData> TableLoader::loadTableList(const Settings& settings) {
    std::vector<TableData> tables;
    if (settings.VPXTablesPath.empty() || !fs::exists(settings.VPXTablesPath)) {
        LOG_ERROR("TableLoader: Invalid or empty VPX tables path: " << settings.VPXTablesPath);
        return tables;
    }
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            TableData table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.title = entry.path().stem().string();
            table.playfieldImage = getImagePath(table.folder, settings.customPlayfieldImage, settings.defaultPlayfieldImage);
            table.wheelImage = getImagePath(table.folder, settings.customWheelImage, settings.defaultWheelImage);
            table.backglassImage = getImagePath(table.folder, settings.customBackglassImage, settings.defaultBackglassImage);
            table.dmdImage = getImagePath(table.folder, settings.customDmdImage, settings.defaultDmdImage);
            table.playfieldVideo = getVideoPath(table.folder, settings.customPlayfieldVideo, settings.defaultPlayfieldVideo);
            table.backglassVideo = getVideoPath(table.folder, settings.customBackglassVideo, settings.defaultBackglassVideo);
            table.dmdVideo = getVideoPath(table.folder, settings.customDmdVideo, settings.defaultDmdVideo);
            tables.push_back(table);
        }
    }

    // Load vpxtool metadata if titleSource=metadata
    if (settings.titleSource == "metadata") {
        std::string jsonPath = settings.VPXTablesPath + "vpxtool_index.json";
        if (fs::exists(jsonPath)) {
            try {
                std::ifstream file(jsonPath);
                nlohmann::json json;
                file >> json;
                for (const auto& tableJson : json["tables"]) {
                    std::string path = tableJson["path"].get<std::string>();
                    for (auto& table : tables) {
                        if (table.vpxFile == path) {
                            table.tableName = tableJson["table_info"]["table_name"].is_null() ? table.title : tableJson["table_info"]["table_name"].get<std::string>();
                            LOG_DEBUG("TableLoader: Set title = " << table.title << " for vpxFile = " << table.vpxFile);
                            table.title = table.tableName; // Use tableName for title
                            table.authorName = tableJson["table_info"]["author_name"].is_null() ? "" : tableJson["table_info"]["author_name"].get<std::string>();
                            table.gameName = tableJson["game_name"].is_null() ? "" : tableJson["game_name"].get<std::string>();
                            table.romPath = tableJson["rom_path"].is_null() ? "" : tableJson["rom_path"].get<std::string>();
                            table.tableDescription = tableJson["table_info"]["table_description"].is_null() ? "" : tableJson["table_info"]["table_description"].get<std::string>();
                            table.tableSaveDate = tableJson["table_info"]["table_save_date"].is_null() ? "" : tableJson["table_info"]["table_save_date"].get<std::string>();
                            table.lastModified = tableJson["last_modified"].is_null() ? "" : tableJson["last_modified"].get<std::string>();
                            table.releaseDate = tableJson["table_info"]["release_date"].is_null() ? "" : tableJson["table_info"]["release_date"].get<std::string>();
                            table.tableVersion = tableJson["table_info"]["table_version"].is_null() ? "" : tableJson["table_info"]["table_version"].get<std::string>();
                            table.tableRevision = tableJson["table_info"]["table_save_rev"].is_null() ? "" : tableJson["table_info"]["table_save_rev"].get<std::string>();

                            // Parse year
                            if (!tableJson["table_info"]["release_date"].is_null()) {
                                std::string releaseDate = tableJson["table_info"]["release_date"].get<std::string>();
                                std::regex dateRegex(R"((\d{4})|(\d{2}\.\d{2}\.\d{4}))");
                                std::smatch match;
                                if (std::regex_search(releaseDate, match, dateRegex)) {
                                    table.year = match[1].matched ? match[1].str() : match[2].str().substr(6);
                                }
                            }
                            if (table.year.empty()) {
                                std::regex yearRegex(R"regex(\((\d{4})\))regex");
                                std::smatch match;
                                if (std::regex_search(table.tableName, match, yearRegex)) {
                                    table.year = match[1].str();
                                }
                            }

                            // Parse manufacturer
                            std::regex manufRegex(R"regex(\(([^)]+)\s+\d{4}\))regex");
                            std::smatch match;
                            if (std::regex_search(table.tableName, match, manufRegex)) {
                                table.manufacturer = match[1].str();
                            }
                            break;
                        }
                    }
                }
            } catch (const std::exception& e) {
                LOG_ERROR("TableLoader: Failed to parse vpxtool_index.json: " << e.what());
                // Fallback to filename-based titles
            }
        } else {
            LOG_ERROR("TableLoader: vpxtool_index.json not found at " << jsonPath);
            // TODO: Trigger pop-up in ConfigUI
        }
    }

    std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
        return a.title < b.title;
    });
    letterIndex.clear();
    for (size_t i = 0; i < tables.size(); ++i) {
        char firstChar = tables[i].title[0]; // Raw first char, no toupper yet
        if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = i;
            }
        }
    }
    return tables;
}

std::string TableLoader::getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath;
    //LOG_DEBUG("TableLoader: Checking custom path: " << imageFile.string());
    if (fs::exists(imageFile)) {
        return imageFile.string();
    }
    //LOG_DEBUG("TableLoader: Falling back to default: " << defaultImagePath);
    if (!fs::exists(defaultImagePath)) {
        LOG_ERROR("TableLoader: Default image not found: " << defaultImagePath);
    }
    return defaultImagePath;
}

std::string TableLoader::getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile))
        return videoFile.string();
    else if (fs::exists(defaultVideoPath))
        return defaultVideoPath;
    else
        return "";
}