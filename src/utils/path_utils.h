/**
 * @file path_utils.h
 * @brief Defines the PathUtils class for resolving file paths in ASAPCabinetFE.
 *
 * This header provides the PathUtils class, which contains static methods to resolve
 * file paths for images, videos, and music based on a root directory and optional
 * custom/default paths. The class uses std::filesystem to handle path operations and
 * supports configurable path settings (e.g., via Settings), with potential for
 * configUI integration to customize default paths or validation rules in the future.
 */

#ifndef PATH_UTILS_H
#define PATH_UTILS_H // Header guard to prevent multiple inclusions

#include <string> // For std::string to represent file paths
#include <nlohmann/json.hpp>
#include <algorithm> // For std::remove_if, std::remove
#include <cctype>    // For std::iscntrl, std::isspace

/**
 * @class PathUtils
 * @brief Utility class for resolving file paths in ASAPCabinetFE.
 *
 * This class provides static methods to construct and validate file paths for
 * images, videos, and music, starting from a root directory. It checks file
 * existence and falls back to default paths when necessary, logging errors for
 * missing defaults. The methods are configurable via input parameters and can be
 * extended with configUI for user-defined path preferences.
 */
class PathUtils {
public:
    /**
     * @brief Resolves the path to an image file.
     *
     * Constructs a path by combining the root directory with the provided imagePath.
     * If the resulting file exists, its path is returned; otherwise, it falls back
     * to defaultImagePath, logging an error if the default is also missing. This
     * method supports configurable image paths and can be enhanced via configUI
     * for custom defaults.
     *
     * @param root The base directory to start the path resolution.
     * @param imagePath The custom path to the image file.
     * @param defaultImagePath The fallback path if the custom path is invalid for wheel image.
     * @return The resolved path as a string (custom if exists, default otherwise).
     */
    static std::string getImagePath(const std::string& root, const std::string& imagePath, const std::string& defaultImagePath);

    /**
     * @brief Resolves the path to a video file.
     *
     * Constructs a path by combining the root directory with the provided videoPath.
     * If the resulting file exists, its path is returned; if not, it checks
     * defaultVideoPath and returns it if valid, otherwise returns an empty string.
     * This method is configurable and supports future configUI customization.
     *
     * @param root The base directory to start the path resolution.
     * @param videoPath The custom path to the video file.
     * @return The resolved path as a string (custom if exists, default if exists, empty otherwise).
     */
    static std::string getVideoPath(const std::string& root, const std::string& videoPath);

    /**
     * @brief Resolves the path to a music file.
     *
     * Constructs a path by combining the root directory with the provided musicPath.
     * If the resulting file exists and is a regular file, its path is returned;
     * otherwise, an empty string is returned with a debug log. This method lacks
     * a default fallback and can be extended via configUI for additional options.
     *
     * @param root The base directory to start the path resolution.
     * @param musicPath The path to the music file.
     * @return The resolved path as a string (valid path if exists, empty otherwise).
     */
    static std::string getAudioPath(const std::string& root, const std::string& musicPath);

    // Directory finders
    static bool getPupPath(const std::string& root);
    static std::string getPinmamePath(const std::string& root);
    static bool getAltcolorPath(const std::string& pinmamePath);
    static bool getAltsoundPath(const std::string& pinmamePath);
    static bool getAltMusic(const std::string& tableRoot);
    static bool getUltraDmdPath(const std::string& tableRoot);
    static bool hasIniForTable(const std::string& tableFolder, const std::string& tableStem);
    static bool hasB2SForTable(const std::string& tableFolder, const std::string& tableStem);
    static std::string getRomPath(const std::string& pinmamePath, std::string& outRomName);


private:
    // Self helpers
    static bool containsRegularFiles(const std::string& directoryPath);
    static std::string findSubfolderCaseInsensitive(const std::string& parentPath, const std::string& targetFolderNameLowercase);
    static std::string findSubfolderBySuffixCaseInsensitive(const std::string& parentPath, const std::string& targetSuffixLowercase);
};

#endif // PATH_UTILS_H
