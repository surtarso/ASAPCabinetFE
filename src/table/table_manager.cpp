#include "table/table_manager.h"
#include "utils/logging.h"
#include "config/config_manager.h" // Add this to access ConfigManager
#include <algorithm>
#include <iostream>
#include <cctype>

std::map<char, int> letterIndex;

std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath;
    LOG_DEBUG("Checking custom path: " << imageFile.string());
    if (fs::exists(imageFile)) {
        return imageFile.string();
    }
    LOG_DEBUG("Falling back to default: " << defaultImagePath);
    if (!fs::exists(defaultImagePath)) {
        LOG_DEBUG("Default image not found: " << defaultImagePath);
    }
    return defaultImagePath;
}

std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile))
        return videoFile.string();
    else if (fs::exists(defaultVideoPath))
        return defaultVideoPath;
    else
        return "";
}

std::vector<Table> loadTableList(const Settings& settings) { // Take Settings as parameter
    std::vector<Table> tables;
    if (settings.vpxTablesPath.empty() || !fs::exists(settings.vpxTablesPath)) {
        LOG_DEBUG("Invalid or empty VPX tables path: " << settings.vpxTablesPath);
        return tables; // Return empty list if path is invalid
    }
    for (const auto& entry : fs::recursive_directory_iterator(settings.vpxTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            Table table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.tableName = entry.path().stem().string();
            table.tableImage = getImagePath(table.folder, settings.customTableImage, settings.defaultTableImage);
            table.wheelImage = getImagePath(table.folder, settings.customWheelImage, settings.defaultWheelImage);
            table.backglassImage = getImagePath(table.folder, settings.customBackglassImage, settings.defaultBackglassImage);
            table.dmdImage = getImagePath(table.folder, settings.customDmdImage, settings.defaultDmdImage);
            table.tableVideo = getVideoPath(table.folder, settings.customTableVideo, settings.defaultTableVideo);
            table.backglassVideo = getVideoPath(table.folder, settings.customBackglassVideo, settings.defaultBackglassVideo);
            table.dmdVideo = getVideoPath(table.folder, settings.customDmdVideo, settings.defaultDmdVideo);
            tables.push_back(table);
        }
    }
    std::sort(tables.begin(), tables.end(), [](const Table& a, const Table& b) {
        return a.tableName < b.tableName;
    });
    letterIndex.clear();
    for (size_t i = 0; i < tables.size(); ++i) {
        char firstLetter = toupper(tables[i].tableName[0]);
        if (letterIndex.find(firstLetter) == letterIndex.end()) {
            letterIndex[firstLetter] = i;
        }
    }
    return tables;
}