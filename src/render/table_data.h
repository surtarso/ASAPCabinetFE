#ifndef TABLE_DATA_H
#define TABLE_DATA_H

#include <string>

struct TableData {
    std::string title;       // Name of the table (e.g., "Pinball FX")
    std::string vpxFile;     // Full path to the .vpx file
    std::string folder;      // Directory containing the .vpx file
    std::string playfieldImage;  // Path to table image (static)
    std::string wheelImage;      // Path to wheel image (static)
    std::string backglassImage;  // Path to backglass image (static)
    std::string dmdImage;        // Path to DMD image (static)
    std::string playfieldVideo;  // Path to table video (if any)
    std::string backglassVideo;  // Path to backglass video (if any)
    std::string dmdVideo;        // Path to DMD video (if any)
};

#endif // TABLE_DATA_H