/**
 * @file table_data.h
 * @brief Defines the TableData struct for storing VPX table metadata and media paths.
 *
 * This header provides the TableData struct, which encapsulates metadata and file paths
 * for Visual Pinball X (VPX) tables, including media assets (images, videos, music) and
 * vpxtool-extracted metadata. It is used by the TableLoader and rendering components
 * in ASAPCabinetFE to manage table information.
 */

#ifndef TABLE_DATA_H
#define TABLE_DATA_H

#include <string>

/**
 * @struct TableData
 * @brief Stores metadata and media paths for a VPX table.
 *
 * This struct holds information about a Visual Pinball X (VPX) table, including paths
 * to media assets (images, videos, music) and metadata extracted via vpxtool. It supports
 * rendering and sorting of tables in the ASAPCabinetFE application.
 */
struct TableData {
    // Standard paths
    std::string title;           ///< Filename or title of the table.
    std::string vpxFile;         ///< Full path to the .vpx file.
    std::string folder;          ///< Directory containing the .vpx file.
    std::string playfieldImage;  ///< Path to the static playfield image.
    std::string wheelImage;      ///< Path to the static wheel image.
    std::string backglassImage;  ///< Path to the static backglass image.
    std::string dmdImage;        ///< Path to the static DMD (Dot Matrix Display) image.
    std::string playfieldVideo;  ///< Path to the playfield video, if available.
    std::string backglassVideo;  ///< Path to the backglass video, if available.
    std::string dmdVideo;        ///< Path to the DMD video, if available.
    std::string music;           ///< Path to the music file, if available.

    // vpxtool metadata
    std::string tableName;       ///< Metadata representing the table's name.
    std::string authorName;      ///< Metadata representing the author's name.
    std::string gameName;        ///< Metadata representing the game name.
    std::string romPath;         ///< Metadata representing the ROM path.
    std::string tableDescription; ///< Metadata describing the table.
    std::string tableSaveDate;   ///< Metadata indicating when the table was saved.
    std::string lastModified;    ///< Metadata indicating the last modification date.
    std::string releaseDate;     ///< Metadata indicating the table's release date.
    std::string tableVersion;    ///< Metadata indicating the table's version.
    std::string tableRevision;   ///< Metadata indicating the table's revision.

    // Sorting metadata
    std::string manufacturer;    ///< Metadata indicating the table's manufacturer, used for sorting.
    std::string year;            ///< Metadata indicating the table's year, used for sorting.
};

#endif // TABLE_DATA_H