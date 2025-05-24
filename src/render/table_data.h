#ifndef TABLE_DATA_H
#define TABLE_DATA_H

#include <string>

/**
 * @brief Structure holding table and media information along with vpxtool metadata.
 *
 * This structure encapsulates paths to various media assets related to a table (such as images and videos)
 * as well as metadata extracted using vpxtool. It is designed to manage the data for a pinball cabinet
 * rendering front-end.
 *
 * Members:
 *   - title: Filename or title of the table.
 *   - vpxFile: Full path to the .vpx file associated with the table.
 *   - folder: Directory containing the .vpx file.
 *   - playfieldImage: File path to the static table image.
 *   - wheelImage: File path to the static wheel image.
 *   - backglassImage: File path to the static backglass image.
 *   - dmdImage: File path to the static DMD (Dot Matrix Display) image.
 *   - playfieldVideo: File path to the table video (if available).
 *   - backglassVideo: File path to the backglass video (if available).
 *   - dmdVideo: File path to the DMD video (if available).
 *
 *   - tableName: Metadata representing the table's name.
 *   - authorName: Metadata representing the author's name.
 *   - gameName: Metadata representing the game name.
 *   - romPath: Metadata representing the ROM path.
 *   - tableDescription: Metadata with the table's description.
 *   - tableSaveDate: Metadata capturing when the table was saved.
 *   - lastModified: Metadata representing the last modification date.
 *   - releaseDate: Metadata capturing the release date of the table.
 *   - tableVersion: Metadata indicating the version of the table.
 *   - tableRevision: Metadata indicating the revision of the table.
 *
 *   - manufacturer: Extracted metadata used for sorting, indicating the manufacturer.
 *   - year: Extracted metadata used for sorting, indicating the year related to the table.
 */
struct TableData {
    // Standard paths
    std::string title;           // Filename
    std::string vpxFile;         // Full path to the .vpx file
    std::string folder;          // Directory containing the .vpx file
    std::string playfieldImage;  // Path to table image (static)
    std::string wheelImage;      // Path to wheel image (static)
    std::string backglassImage;  // Path to backglass image (static)
    std::string dmdImage;        // Path to DMD image (static)
    std::string playfieldVideo;  // Path to table video (if any)
    std::string backglassVideo;  // Path to backglass video (if any)
    std::string dmdVideo;        // Path to DMD video (if any)
    std::string music;       // Path to the Music file (if any)

    // vpxtool metadata
    std::string tableName;
    std::string authorName;
    std::string gameName;
    std::string romPath;
    std::string tableDescription;
    std::string tableSaveDate;
    std::string lastModified;
    std::string releaseDate;
    std::string tableVersion;
    std::string tableRevision;
    // extracted for sorting later
    std::string manufacturer;
    std::string year;
};

#endif // TABLE_DATA_H