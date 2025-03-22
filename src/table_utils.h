#ifndef TABLE_UTILS_H
#define TABLE_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "config.h"

namespace fs = std::filesystem;

struct Table {
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

extern std::map<char, int> letterIndex; // Declaration only

std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath);
std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath);
std::vector<Table> loadTableList();

#endif