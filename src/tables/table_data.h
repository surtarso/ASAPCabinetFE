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
    // We need to get the most out of the files before trying to
    // match with vpsdb. We should get the most without any tool, than
    // upgrade the metadata with vpin to get best of both, than finally
    // try to match with the online db to get the best match possible.
    // ----------------- BEST MATCHES --------------------
    // Sorting metadata (calculated best results)
    // title is a generic string to be shown on the UI.
    // with filename it uses the filename, with vpin it uses the best
    // of both (for bad metadata), used to match against vpsdb.
    std::string title;           ///< Table title (from vpxtool, vpsdb, or filename).
    // if all fails we should try to extract these from the file name.
    std::string manufacturer;    ///< Manufacturer from title, vpin/vpxtool or vpsdb, used for sorting.
    std::string year;            ///< Year from title, vpin/vpxtool or vpsdb, used for sorting.
    
    // ------------------ FILE PATHS ------------------
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
    std::string topperVideo;     ///< Path to the topper video, if available.
    std::string music;           ///< Path to the music file, if available.
    std::string launchAudio;     ///< Path to the custom launch audio, if available.
    std::string romPath;         ///< Path to the pinmame/roms/'romname'.zip
    std::string romName;         ///< file found in romPath without extension.
    bool altSound = false;       ///< True if found the pinmame/altsound folder (non-empty)
    bool altColor = false;       ///< True if found the pinmame/AltColor folder (non-empty)
    bool hasPup = false;         ///< True if found the pupvideos/ folder (non-empty)
    bool hasAltMusic = false;    ///< True if found the music/ folder (non-empty)
    bool hasUltraDMD = false;    ///< True if found the *.UltraDMD folder (non-empty)

    // ------------ FILE METADATA (vpin/vpxtool) -----------
    // we must keep repeated fields if found for better vpsdb matching 
    std::string tableName;       ///< Table name from file metadata.
    std::string authorName;      ///< Author(s) from file metadata.
    std::string tableDescription;///< Description from file metadata
    std::string tableSaveDate;   ///< Save date from file metadata.
    std::string lastModified;    ///< Last modified date from file metadata.
    std::string releaseDate;     ///< Release date from file metadata.
    std::string tableVersion;    ///< Table version from file or vpsdb.
    std::string tableRevision;   ///< Table revision from file metadata.   
    //TODO (not yet being collected from table_info?)
    std::string tableyear;
    std::string tableRules;
    //these are inside 'properties' indent in 'table_info'
    std::string tableType;
    std::string companyName;     ///< for "manufacturer"
    std::string companyYear;     ///< for "year"

    // --------------- VPSDB METADATA -------------
    std::string vpsId;           ///< Unique ID from vpsdb.json.
    std::string vpsName;         ///< Table name from vpsdb.json.
    std::string vpsType;         ///< Table vpsType (e.g., SS, EM) from vpsdb.json.
    std::string vpsThemes;       ///< Comma-separated vpsThemes from vpsdb.json.
    std::string vpsDesigners;    ///< Comma-separated Designers from vpsdb.json.
    std::string vpsPlayers;      ///< Number of Players from vpsdb.json.
    std::string vpsIpdbUrl;      ///< IPDB URL from vpsdb.json.
    std::string vpsVersion;      ///< Table version from vpsdb.json tableFiles.
    std::string vpsAuthors;      ///< Comma-separated authors from vpsdb.json tableFiles.
    std::string vpsFeatures;     ///< Comma-separated vpsFeatures from vpsdb.json tableFiles.
    std::string vpsComment;      ///< Comment from vpsdb.json tableFiles.
    //TODO (not yet being collected)
    std::string vpsManufacturer; ///< Manufacturer from vpsdb
    std::string vpsYear;         ///< Year from vpsdb
    std::string vpsImgUrl;       ///< table image from vpsdb (to show)
    std::string vpsTableUrl;     ///< table URL to download.

    // --------------- OPERATIONAL TAGS ------------------
    // this later should be a "rating" for the metadata with 1-5 stars or w.e.
    float matchConfidence = 0.0f; ///< Confidence score of match with vpsdb
    float matchScore = 0.0f;      ///< for display
    std::string jsonOwner; /// < file_scanner, vpin_scanner, vpxtool_scanner, vpsdb_scanner
    //TODO (not yet implemented)
    std::string playCount;
};

#endif // TABLE_DATA_H