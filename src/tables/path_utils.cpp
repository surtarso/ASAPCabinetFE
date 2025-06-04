#include "tables/path_utils.h"
#include "utils/logging.h"
#include <filesystem>

namespace fs = std::filesystem;

std::string PathUtils::getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath;
    if (fs::exists(imageFile)) {
        return imageFile.string();
    }
    if (!fs::exists(defaultImagePath)) {
        LOG_ERROR("PathUtils: Default image not found: " << defaultImagePath);
    }
    return defaultImagePath;
}

std::string PathUtils::getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile)) {
        return videoFile.string();
    }
    if (fs::exists(defaultVideoPath)) {
        return defaultVideoPath;
    }
    return "";
}

std::string PathUtils::getMusicPath(const std::string& root, const std::string& musicPath) {
    if (musicPath.empty()) {
        LOG_DEBUG("PathUtils: Music path from settings is empty for root: " << root);
        return "";
    }
    fs::path musicFile = fs::path(root) / musicPath;
    if (fs::exists(musicFile) && fs::is_regular_file(musicFile)) {
        //LOG_DEBUG("PathUtils: Found Music: " << musicFile.string());
        return musicFile.string();
    }
    //LOG_DEBUG("PathUtils: No music file found or not a regular file for: " << musicFile.string());
    return "";
}