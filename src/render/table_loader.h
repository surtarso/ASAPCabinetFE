#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "config/settings.h" // Change to settings.h

namespace fs = std::filesystem;

struct TableLoader {
    std::string tableName;
    std::string vpxFile;
    std::string folder;
    std::string tableImage;
    std::string wheelImage;
    std::string backglassImage;
    std::string dmdImage;
    std::string tableVideo;
    std::string backglassVideo;
    std::string dmdVideo;
};

extern std::map<char, int> letterIndex;

std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath);
std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath);
std::vector<TableLoader> loadTableList(const Settings& settings); // Update signature

#endif