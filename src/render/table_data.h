#ifndef TABLE_DATA_H
#define TABLE_DATA_H

#include <string>

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