#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "config/settings.h"

namespace fs = std::filesystem;

// Struct to hold all assets for a single table (VPX file and its media)
struct TableLoader {
    std::string tableName;       // Name of the table (e.g., "Pinball FX")
    std::string vpxFile;         // Full path to the .vpx file
    std::string folder;          // Directory containing the .vpx file
    std::string tableImage;      // Path to table image (static)
    std::string wheelImage;      // Path to wheel image (static)
    std::string backglassImage;  // Path to backglass image (static)
    std::string dmdImage;        // Path to DMD image (static)
    std::string tableVideo;      // Path to table video (if any)
    std::string backglassVideo;  // Path to backglass video (if any)
    std::string dmdVideo;        // Path to DMD video (if any)
};

// Global map for quick index lookup by first letter/number (e.g., 'A' -> index 5)
extern std::map<char, int> letterIndex;

// Utility: Resolves image path, falling back to default if custom doesn’t exist
std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath);

// Utility: Resolves video path, returning empty string if neither custom nor default exists
std::string getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath);

// Core function to scan VPX tables and build the table list
std::vector<TableLoader> loadTableList(const Settings& settings);

#endif