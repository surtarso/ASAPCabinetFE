#include "render/table_loader.h"
#include "utils/logging.h"
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