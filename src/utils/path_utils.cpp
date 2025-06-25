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

#include "path_utils.h"
#include "log/logging.h"
#include <filesystem>
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify path operations

std::string PathUtils::getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath; // Construct path from root and custom image path
    if (fs::exists(imageFile)) {
        return imageFile.string();
    }
    if (!fs::exists(defaultImagePath)) {
        LOG_ERROR("Default image not found: " + std::string(defaultImagePath)); // Log error if default is missing
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

std::string PathUtils::getAudioPath(const std::string& root, const std::string& musicPath) {
    if (musicPath.empty()) {
        // LOG_DEBUG("Music path from settings is empty for root: " << root); // Log debug if path is empty
        return "";
    }
    fs::path musicFile = fs::path(root) / musicPath; // Construct path from root and music path
    if (fs::exists(musicFile) && fs::is_regular_file(musicFile)) {
        //LOG_DEBUG("Found Music: " << musicFile.string()); // Commented debug log
        return musicFile.string();
    }
    //LOG_DEBUG("No music file found or not a regular file for: " << musicFile.string()); // Commented debug log
    return ""; // Return empty string if file doesn't exist or isn't regular
}

// PathUtils::getAltMusic - Checks for 'music' subfolder
bool PathUtils::getAltMusic(const std::string& tableRoot) {
    std::string musicFolderActualPath = findSubfolderCaseInsensitive(tableRoot, "music");
    // Check if folder exists and is NOT empty (contains any files or subdirectories)
    if (!musicFolderActualPath.empty() && fs::exists(musicFolderActualPath) && !fs::is_empty(musicFolderActualPath)) {
        // LOG_DEBUG("Found 'music' folder with content for table: " << tableRoot);
        return true;
    }
    // LOG_DEBUG("No 'music' folder with content found in " << tableRoot);
    return false;
}

// PathUtils::getUltraDmdPath - Checks for any folder ending with '.ultradmd'
bool PathUtils::getUltraDmdPath(const std::string& tableRoot) {
    std::string ultraDmdFolderActualPath = findSubfolderBySuffixCaseInsensitive(tableRoot, ".ultradmd");
    // Check if folder exists and is NOT empty (contains any files or subdirectories)
    if (!ultraDmdFolderActualPath.empty() && fs::exists(ultraDmdFolderActualPath) && !fs::is_empty(ultraDmdFolderActualPath)) {
        // LOG_DEBUG("Found '.UltraDMD' folder with content for table: " << tableRoot);
        return true;
    }
    // LOG_DEBUG("No '.UltraDMD' folder with content found in " << tableRoot);
    return false;
}

// PathUtils::getPupPath - NOW uses !fs::is_empty() for content check
bool PathUtils::getPupPath(const std::string& root) {
    std::string pupFolderActualPath = findSubfolderCaseInsensitive(root, "pupvideos");
    // Check if folder exists and is NOT empty (contains any files or subdirectories)
    if (!pupFolderActualPath.empty() && fs::exists(pupFolderActualPath) && !fs::is_empty(pupFolderActualPath)) {
        // LOG_DEBUG("Found pupvideos folder with content: " << pupFolderActualPath);
        return true;
    }
    // LOG_DEBUG("No pupvideos folder with content found in " << root);
    return false;
}

// PathUtils::getPinmamePath - NOW uses findSubfolderCaseInsensitive
std::string PathUtils::getPinmamePath(const std::string& root) {
    std::string pinmameFolderActualPath = findSubfolderCaseInsensitive(root, "pinmame");
    if (!pinmameFolderActualPath.empty()) { // findSubfolderCaseInsensitive already checks exists and is_directory
        // LOG_DEBUG("Found pinmame folder: " << pinmameFolderActualPath);
        return pinmameFolderActualPath;
    }
    // LOG_DEBUG("No pinmame folder found at " << root); // Log parent folder if not found
    return "";
}

// PathUtils::getAltcolorPath - NOW uses !fs::is_empty() for content check
bool PathUtils::getAltcolorPath(const std::string& pinmamePath) {
    if (pinmamePath.empty()) {
        // LOG_DEBUG("pinmamePath is empty, skipping AltColor check.");
        return false;
    }
    std::string altcolorFolderActualPath = findSubfolderCaseInsensitive(pinmamePath, "altcolor");
    // Check if folder exists and is NOT empty (contains any files or subdirectories)
    if (!altcolorFolderActualPath.empty() && fs::exists(altcolorFolderActualPath) && !fs::is_empty(altcolorFolderActualPath)) {
        // LOG_DEBUG("Found AltColor folder with content: " << altcolorFolderActualPath);
        return true;
    }
    // LOG_DEBUG("No AltColor folder with content found in " << pinmamePath);
    return false;
}

// PathUtils::getAltsoundPath - NOW uses !fs::is_empty() for content check
bool PathUtils::getAltsoundPath(const std::string& pinmamePath) {
    if (pinmamePath.empty()) {
        // LOG_DEBUG("pinmamePath is empty, skipping AltSound check.");
        return false;
    }
    std::string altsoundFolderActualPath = findSubfolderCaseInsensitive(pinmamePath, "altsound");
    // Check if folder exists and is NOT empty (contains any files or subdirectories)
    if (!altsoundFolderActualPath.empty() && fs::exists(altsoundFolderActualPath) && !fs::is_empty(altsoundFolderActualPath)) {
        // LOG_DEBUG("Found AltSound folder with content: " << altsoundFolderActualPath);
        return true;
    }
    // LOG_DEBUG("No AltSound folder with content found in " << pinmamePath);
    return false;
}

std::string PathUtils::getRomPath(const std::string& pinmamePath, std::string& outRomName) {
    outRomName = ""; // Ensure romName is cleared initially
    if (pinmamePath.empty()) {
        // LOG_DEBUG("pinmamePath is empty, skipping ROM check.");
        return "";
    }

    fs::path romsFolder = fs::path(pinmamePath) / "roms";
    if (fs::exists(romsFolder) && fs::is_directory(romsFolder)) {
        for (const auto& romEntry : fs::directory_iterator(romsFolder)) {
            if (romEntry.is_regular_file() && romEntry.path().extension() == ".zip") {
                outRomName = romEntry.path().stem().string();
                // LOG_DEBUG("Found ROM for pinmame path " << pinmamePath << ": romName=" << outRomName << ", romPath=" << romEntry.path().string());
                return romEntry.path().string(); // Take the first .zip file
            }
        }
        // LOG_DEBUG("No .zip file found in " << romsFolder.string());
    } else {
        // LOG_DEBUG("No pinmame/roms folder found at " << romsFolder.string());
    }
    return ""; // No ROM found
}

// Helper to check if a directory exists and contains any regular files
bool PathUtils::containsRegularFiles(const std::string& directoryPath) {
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return false;
    }
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            return true;
        }
    }
    return false;
}

std::string PathUtils::findSubfolderCaseInsensitive(const std::string& parentPath, const std::string& targetFolderNameLowercase) {
    if (!fs::exists(parentPath) || !fs::is_directory(parentPath)) {
        return "";
    }

    for (const auto& entry : fs::directory_iterator(parentPath)) {
        if (entry.is_directory()) {
            std::string currentFolderName = entry.path().filename().string();
            // Convert current folder name to lowercase for comparison
            std::transform(currentFolderName.begin(), currentFolderName.end(), currentFolderName.begin(),
                           [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); }); // Use static_cast for safety

            if (currentFolderName == targetFolderNameLowercase) {
                // Found a match (case-insensitively), return the actual path (with original casing)
                return entry.path().string();
            }
        }
    }
    return ""; // No matching subfolder found
}

std::string PathUtils::findSubfolderBySuffixCaseInsensitive(const std::string& parentPath, const std::string& targetSuffixLowercase) {
    if (!fs::exists(parentPath) || !fs::is_directory(parentPath)) {
        return "";
    }

    for (const auto& entry : fs::directory_iterator(parentPath)) {
        if (entry.is_directory()) {
            std::string currentFolderName = entry.path().filename().string();
            std::string lowerCurrentFolderName = currentFolderName;
            std::transform(lowerCurrentFolderName.begin(), lowerCurrentFolderName.end(), lowerCurrentFolderName.begin(),
                           [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

            // Check if the lowercase folder name ends with the target suffix
            if (lowerCurrentFolderName.length() >= targetSuffixLowercase.length() &&
                lowerCurrentFolderName.substr(lowerCurrentFolderName.length() - targetSuffixLowercase.length()) == targetSuffixLowercase) {
                return entry.path().string(); // Return the original (correct case) path
            }
        }
    }
    return "";
}




