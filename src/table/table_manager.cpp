#include "table/table_manager.h"
#include "utils/logging.h"
#include <algorithm>
#include <iostream>
#include <cctype>

std::map<char, int> letterIndex; // Maps first letter of table names to their indices for quick navigation.

// Resolves image path, checking custom path first, then falling back to default.
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
  
// Resolves video path, checking custom path, then default, returning empty string if neither exists.
std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile))
        return videoFile.string();
    else if (fs::exists(defaultVideoPath))
        return defaultVideoPath;
    else
        return "";
}

// Loads and sorts table list from VPX files, populating table metadata and letter index.
std::vector<Table> loadTableList() {
    std::vector<Table> tables;
    // Iterate through VPX files in the tables directory.
    for (const auto& entry : fs::recursive_directory_iterator(VPX_TABLES_PATH)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            Table table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.tableName = entry.path().stem().string();
            table.tableImage = getImagePath(table.folder, CUSTOM_TABLE_IMAGE, DEFAULT_TABLE_IMAGE);
            table.wheelImage = getImagePath(table.folder, CUSTOM_WHEEL_IMAGE, DEFAULT_WHEEL_IMAGE);
            table.backglassImage = getImagePath(table.folder, CUSTOM_BACKGLASS_IMAGE, DEFAULT_BACKGLASS_IMAGE);
            table.dmdImage = getImagePath(table.folder, CUSTOM_DMD_IMAGE, DEFAULT_DMD_IMAGE);
            table.tableVideo = getVideoPath(table.folder, CUSTOM_TABLE_VIDEO, DEFAULT_TABLE_VIDEO);
            table.backglassVideo = getVideoPath(table.folder, CUSTOM_BACKGLASS_VIDEO, DEFAULT_BACKGLASS_VIDEO);
            table.dmdVideo = getVideoPath(table.folder, CUSTOM_DMD_VIDEO, DEFAULT_DMD_VIDEO);
            tables.push_back(table);
        }
    }
    
    // Sort tables alphabetically by name.
    std::sort(tables.begin(), tables.end(), [](const Table& a, const Table& b) {
        return a.tableName < b.tableName;
    });

    // Build letter index for quick navigation by first letter.
    letterIndex.clear();
    for (size_t i = 0; i < tables.size(); ++i) {
        char firstLetter = toupper(tables[i].tableName[0]);
        if (letterIndex.find(firstLetter) == letterIndex.end()) {
            letterIndex[firstLetter] = i; // Store index of first table starting with this letter.
        }
    }
    return tables;
}