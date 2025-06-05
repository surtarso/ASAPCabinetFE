/**
 * @file path_utils.cpp
 * @brief Implements the PathUtils class for resolving file paths in ASAPCabinetFE.
 *
 * This file provides the implementation of the PathUtils class, which resolves file
 * paths for images, videos, and music by combining a root directory with custom or
 * default paths. The class uses std::filesystem to check file existence and handles
 * fallbacks with logging for errors or debug information. The methods are configurable
 * via input parameters and can be extended with configUI for user-defined path settings
 * (e.g., custom defaults or validation rules) in the future.
 */

#include "tables/path_utils.h"
#include "utils/logging.h"
#include <filesystem>

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify path operations

std::string PathUtils::getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath; // Construct path from root and custom image path
    if (fs::exists(imageFile)) {
        return imageFile.string();
    }
    if (!fs::exists(defaultImagePath)) {
        LOG_ERROR("PathUtils: Default image not found: " << defaultImagePath); // Log error if default is missing
    }
    return defaultImagePath; // Return default even if it doesn't exist
}

std::string PathUtils::getVideoPath(const std::string& root, const std::string& videoPath, const std::string& defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath; // Construct path from root and custom video path
    if (fs::exists(videoFile)) {
        return videoFile.string();
    }
    if (fs::exists(defaultVideoPath)) {
        return defaultVideoPath; // Return default if it exists
    }
    return ""; // Return empty string if neither custom nor default exists
}

std::string PathUtils::getMusicPath(const std::string& root, const std::string& musicPath) {
    if (musicPath.empty()) {
        LOG_DEBUG("PathUtils: Music path from settings is empty for root: " << root); // Log debug if path is empty
        return "";
    }
    fs::path musicFile = fs::path(root) / musicPath; // Construct path from root and music path
    if (fs::exists(musicFile) && fs::is_regular_file(musicFile)) {
        //LOG_DEBUG("PathUtils: Found Music: " << musicFile.string()); // Commented debug log
        return musicFile.string();
    }
    //LOG_DEBUG("PathUtils: No music file found or not a regular file for: " << musicFile.string()); // Commented debug log
    return ""; // Return empty string if file doesn't exist or isn't regular
}