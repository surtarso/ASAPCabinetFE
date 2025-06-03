/**
 * @file table_loader.h
 * @brief Defines the TableLoader class for loading table data in ASAPCabinetFE.
 *
 * This header provides the TableLoader class, which implements the ITableLoader interface
 * to load metadata and media paths for Visual Pinball X (VPX) tables, with optional VPS
 * metadata enrichment via VpsDatabaseClient.
 */

#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include "tables/itable_loader.h"
#include "vps_database_client.h"
#include <filesystem>

/**
 * @brief Alias for std::filesystem namespace.
 */
namespace fs = std::filesystem;

/**
 * @class TableLoader
 * @brief Loads table data and metadata for VPX tables.
 *
 * Implements ITableLoader to scan VPX table directories, load vpxtool metadata, and enrich
 * tables with VPS metadata. Maintains a letter index for navigation.
 */
class TableLoader : public ITableLoader {
public:
    TableLoader() = default; // Empty constructor

    /**
     * @brief Loads a list of table data based on settings.
     *
     * Scans VPXTablesPath for .vpx files, loads vpxtool metadata if titleSource="metadata",
     * and enriches with VPS data. Caches results in asapcabinetfe_index.json.
     *
     * @param settings Application settings.
     * @return Vector of TableData objects.
     */
    std::vector<TableData> loadTableList(const Settings& settings) override;

    /**
     * @brief Retrieves the letter index for navigation.
     *
     * Maps each letter (e.g., 'A', 'B') to the index of the first table starting with that letter.
     *
     * @return Const reference to the letter index map.
     */
    const std::map<char, int>& getLetterIndex() const override { return letterIndex; }

private:
    std::map<char, int> letterIndex; ///< Map of letters to table indices.

    /**
     * @brief Resolves image asset path.
     *
     * @param root Root directory.
     * @param imagePath Specific image path.
     * @param defaultImagePath Default image path.
     * @return Resolved image path.
     */
    std::string getImagePath(const std::string& root, const std::string& imagePath,
                             const std::string& defaultImagePath);

    /**
     * @brief Resolves video asset path.
     *
     * @param root Root directory.
     * @param videoPath Specific video path.
     * @param defaultVideoPath Default video path.
     * @return Resolved video path.
     */
    std::string getVideoPath(const std::string& root, const std::string& videoPath,
                             const std::string& defaultVideoPath);

    /**
     * @brief Resolves music asset path.
     *
     * @param root Root directory.
     * @param musicPath Specific music path.
     * @return Resolved music path.
     */
    std::string getMusicPath(const std::string& root, const std::string& musicPath);

    /**
     * @brief Loads ASAP index file.
     *
     * @param settings Application settings.
     * @param tables Vector to store loaded TableData.
     * @return True if successful, false otherwise.
     */
    bool loadAsapIndex(const Settings& settings, std::vector<TableData>& tables);

    /**
     * @brief Saves table list to ASAP index file.
     *
     * @param settings Application settings.
     * @param tables Vector of TableData to save.
     * @return True if successful, false otherwise.
     */
    bool saveAsapIndex(const Settings& settings, const std::vector<TableData>& tables);
};

#endif // TABLE_LOADER_H