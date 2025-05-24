/**
 * @file table_loader.h
 * @brief Defines the TableLoader class for loading table data in ASAPCabinetFE.
 *
 * This header provides the TableLoader class, which implements the ITableLoader interface
 * to load metadata and media paths for Visual Pinball X (VPX) tables. It processes table
 * directories and settings to populate TableData instances and maintains a letter index
 * for table navigation.
 */

#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include "render/itable_loader.h"
#include <filesystem>

/**
 * @brief Alias for std::filesystem namespace.
 */
namespace fs = std::filesystem;

/**
 * @class TableLoader
 * @brief Loads table data and metadata for VPX tables.
 *
 * This class implements the ITableLoader interface to scan table directories, load
 * VPX table metadata, and associate media assets (images, videos, music) based on
 * the provided settings. It also maintains a letter-based index for table navigation.
 */
class TableLoader : public ITableLoader {
public:
    /**
     * @brief Loads a list of table data based on the provided settings.
     *
     * Scans the table directories specified in the settings, extracts metadata using
     * vpxtool, and populates TableData instances with paths to VPX files and associated
     * media assets (images, videos, music).
     *
     * @param settings The application settings containing table paths and media directories.
     * @return A vector of TableData instances representing the loaded tables.
     */
    std::vector<TableData> loadTableList(const Settings& settings) override;

    /**
     * @brief Retrieves the letter index for table navigation.
     *
     * Returns a map associating each letter (e.g., 'A', 'B') with the index of the first
     * table starting with that letter in the loaded table list.
     *
     * @return A const reference to the letter index map.
     */
    const std::map<char, int>& getLetterIndex() const override { return letterIndex; }

private:
    std::map<char, int> letterIndex; ///< Map of letters to table indices for navigation.

    /**
     * @brief Resolves the path to an image asset.
     *
     * Constructs the full path to an image file, falling back to a default path if the
     * specified image path is invalid or empty.
     *
     * @param root The root directory for resolving relative paths.
     * @param imagePath The specific image path to resolve.
     * @param defaultImagePath The default image path to use if imagePath is invalid.
     * @return The resolved image file path.
     */
    std::string getImagePath(const std::string& root, const std::string& imagePath,
                             const std::string& defaultImagePath);

    /**
     * @brief Resolves the path to a video asset.
     *
     * Constructs the full path to a video file, falling back to a default path if the
     * specified video path is invalid or empty.
     *
     * @param root The root directory for resolving relative paths.
     * @param videoPath The specific video path to resolve.
     * @param defaultVideoPath The default video path to use if videoPath is invalid.
     * @return The resolved video file path.
     */
    std::string getVideoPath(const std::string& root, const std::string& videoPath,
                             const std::string& defaultVideoPath);

    /**
     * @brief Resolves the path to a music asset.
     *
     * Constructs the full path to a music file associated with a table.
     *
     * @param root The root directory for resolving relative paths.
     * @param musicPath The specific music path to resolve.
     * @return The resolved music file path.
     */
    std::string getMusicPath(const std::string& root, const std::string& musicPath);
};

#endif // TABLE_LOADER_H