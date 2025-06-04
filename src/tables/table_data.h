/**
 * @file table_data.h
 * @brief Defines the TableData struct for storing VPX table metadata and media paths.
 *
 * This header provides the TableData struct, which encapsulates metadata and file paths
 * for Visual Pinball X (VPX) tables, including media assets (images, videos, music) and
 * metadata from vpxtool or VPS database (vpsdb). It is used by the TableLoader and
 * rendering components in ASAPCabinetFE to manage table information.
 */

#ifndef TABLE_DATA_H
#define TABLE_DATA_H

#include <string>

/**
 * @struct TableData
 * @brief Stores metadata and media paths for a VPX table.
 *
 * This struct holds information about a Visual Pinball X (VPX) table, including paths
 * to media assets (images, videos, music) and metadata from vpxtool or vpsdb.json.
 * It supports rendering and sorting of tables in the ASAPCabinetFE application.
 */
struct TableData {
    // Standard paths
    std::string title;           ///< Table title (from vpxtool, vpsdb, or filename).
    std::string vpxFile;         ///< Full path to the .vpx file.
    std::string folder;          ///< Directory containing the .vpx file.
    std::string playfieldImage;  ///< Path to the static playfield image.
    std::string wheelImage;      ///< Path to the static wheel image.
    std::string backglassImage;  ///< Path to the static backglass image.
    std::string dmdImage;        ///< Path to the static DMD image.
    std::string topperImage;     ///< Path to the static topper image.
    std::string playfieldVideo;  ///< Path to the playfield video, if available.
    std::string backglassVideo;  ///< Path to the backglass video, if available.
    std::string dmdVideo;        ///< Path to the DMD video, if available.
    std::string topperVideo;        ///< Path to the topper video, if available.
    std::string music;           ///< Path to the music file, if available.
    std::string launchAudio;    ///< Path to the custom launch audio, if available.

    // vpxtool metadata
    std::string tableName;       ///< Table name from vpxtool metadata.
    std::string authorName;      ///< Author(s) from vpxtool metadata.
    std::string gameName;        ///< Game name from vpxtool metadata.
    std::string romPath;         ///< ROM path from vpxtool metadata.
    std::string tableDescription; ///< Description from vpxtool or vpsdb comment.
    std::string tableSaveDate;   ///< Save date from vpxtool metadata.
    std::string lastModified;    ///< Last modified date from vpxtool metadata.
    std::string releaseDate;     ///< Release date from vpxtool metadata.
    std::string tableVersion;    ///< Table version from vpxtool or vpsdb.
    std::string tableRevision;   ///< Table revision from vpxtool metadata.

    // vpsdb metadata
    std::string vpsId;           ///< Unique ID from vpsdb.json.
    std::string vpsName;         ///< Table name from vpsdb.json.
    std::string type;            ///< Table type (e.g., SS, EM) from vpsdb.json.
    std::string themes;          ///< Comma-separated themes from vpsdb.json.
    std::string designers;       ///< Comma-separated designers from vpsdb.json.
    std::string players;         ///< Number of players from vpsdb.json.
    std::string ipdbUrl;         ///< IPDB URL from vpsdb.json.
    std::string vpsVersion;      ///< Table version from vpsdb.json tableFiles.
    std::string vpsAuthors;      ///< Comma-separated authors from vpsdb.json tableFiles.
    std::string features;        ///< Comma-separated features from vpsdb.json tableFiles.
    std::string vpsComment;      ///< Comment from vpsdb.json tableFiles.

    // Sorting metadata
    std::string manufacturer;    ///< Manufacturer from vpxtool or vpsdb, used for sorting.
    std::string year;            ///< Year from vpxtool or vpsdb, used for sorting.

    float matchConfidence = 0.0f; ///< Confidence score of match with vpsdb
};

#endif // TABLE_DATA_H