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
#include <cstdint>

/**
 * @struct TableData
 * @brief Stores metadata and media paths for a VPX table.
 *
 * This struct holds information about a Visual Pinball X (VPX) table, including paths
 * to media assets (images, videos, music) and metadata from vpxtool or vpsdb.json.
 * It supports rendering and sorting of tables in the ASAPCabinetFE application.
 */

 // vpx = visual pinball X file
 // vbs = visual basic script file
 // vps = virtual pinball spreadsheet (database)
struct TableData {
    // ------------------ FILE PATHS ------------------
    // We need to get the most out of the files before trying to
    // match with vpsdb. We should get the most without any tool, than
    // upgrade the metadata with vpin to get best of both, than finally
    // try to match with the online db to get the best match possible.
    // ----------------- BEST MATCHES --------------------
    // Sorting metadata (calculated best results)
    // title is a generic string to be shown on the UI.
    // with filename it uses the filename, with vpin it uses the best
    // of both (for bad metadata), used to match against vpsdb.
    std::string title;           ///< Table title (from filename).
    // if all fails we should try to extract these from the file name.
    std::string manufacturer;    ///< Manufacturer from title, vpin/vpxtool or vpsdb, used for sorting.
    std::string year;            ///< Year from title, vpin/vpxtool or vpsdb, used for sorting.

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

    // ------------ FILE METADATA (vpin/vpxtool) -----------
    // we must keep repeated fields if found for better vpsdb matching
    std::string tableName;       ///< Table name from file metadata (e.g., from vpin's `table_name`).
    std::string tableAuthor;     ///< Author(s) from file metadata (e.g., from vpin's `author_name`).
    std::string tableDescription;///< Description from file metadata (e.g., from vpin's `table_description`).
    std::string tableSaveDate;   ///< Save date from file metadata (e.g., from vpin's `table_save_date`).
    std::string tableLastModified;///< Last modified date from file metadata (e.g., from vpin's `last_modified`).
    std::string tableReleaseDate;///< Release date from file metadata (e.g., from vpin's `release_date`).
    std::string tableVersion;    ///< Table version from file metadata (e.g., from vpin's `table_version`).
    std::string tableRevision;   ///< Table revision from file metadata (e.g., from vpin's `table_save_rev`).
    std::string tableBlurb;      ///< Short blurb/summary from file metadata (from vpin's `table_blurb`).
    std::string tableRules;      ///< Rules from file metadata (from vpin's `table_rules`).
    std::string tableAuthorEmail;///< Author's email from file metadata (from vpin's `author_email`).
    std::string tableAuthorWebsite;///< Author's website from file metadata (from vpin's `author_website`).
    // These are inside 'properties' dictionary in 'table_info'
    std::string tableType;       ///< Table type from file metadata properties (e.g., from vpin's `properties.TableType`).
    std::string tableManufacturer;///< Manufacturer/Company from file metadata properties (e.g., from vpin's `properties.CompanyName` or `Company`).
    std::string tableYear;       ///< Year from file metadata properties (e.g., from vpin's `properties.CompanyYear` or `Year`).

    // --------------- VPSDB METADATA -------------
    // These fields will be populated ONLY IF a match is found in the VPS database.
    std::string vpsId;           ///< Unique ID from vpsdb.json (game.id).
    std::string vpsName;         ///< Table name from vpsdb.json (game.name).
    std::string vpsType;         ///< Table type (e.g., SS, EM) from vpsdb.json (game.type).
    std::string vpsThemes;       ///< Comma-separated themes from vpsdb.json (game.theme).
    std::string vpsDesigners;    ///< Comma-separated Designers from vpsdb.json (game.designers).
    std::string vpsPlayers;      ///< Number of Players from vpsdb.json (game.players).
    std::string vpsIpdbUrl;      ///< IPDB URL from vpsdb.json (game.ipdbUrl).
    std::string vpsVersion;      ///< Table version from vpsdb.json tableFiles (tableFiles[].version).
    std::string vpsAuthors;      ///< Comma-separated authors from vpsdb.json tableFiles (tableFiles[].authors).
    std::string vpsFeatures;     ///< Comma-separated features from vpsdb.json tableFiles (tableFiles[].features).
    std::string vpsComment;      ///< Comment from vpsdb.json tableFiles (tableFiles[].comment).
    std::string vpsManufacturer; ///< Manufacturer from vpsdb (game.manufacturer).
    std::string vpsYear;         ///< Year from vpsdb (game.year).
    std::string vpsTableImgUrl;  ///< table image from vpsdb (from tableFiles[].imgUrl, or other media files).
    std::string vpsTableUrl;     ///< table URL to download (from tableFiles[].urls[0].url).
    std::string vpsB2SImgUrl;    ///< table image from vpsdb (from b2sFiles[].imgUrl, or other media files).
    std::string vpsB2SUrl;       ///< table URL to download (from b2sFiles[].urls[0].url).
    std::string vpsFormat;       ///< table format (VPX etc) (from tableFiles[].tableFormat)

    // --------------- OPERATIONAL TAGS ------------------
    float matchConfidence = 0.0f;///< Confidence score of match with vpsdb
    // vbs script patcher related
    std::string hashFromVpx;     ///< SHA256 hash of internal .vpx VB script
    std::string hashFromVbs;     ///< SHA256 hash of (patched) sidecar vb script
    bool isPatched = false;      ///< true if a script patch was applied
    bool hasDiffVbs = false;     ///< check if sidecar .vbs is different than the .vbs inside the VPX file.
    // table launch related
    int playCount = 0;           ///< capture successful launches
    bool isBroken = false;       ///< true if failed to load, dont increment playCount
    float playTimeLast = 0.0f;   ///< last session play time
    float playTimeTotal = 0.0f;  ///< sums playTimeLast
    // extra files scan
    bool hasAltSound = false;    ///< True if found the pinmame/altsound folder (non-empty)
    bool hasAltColor = false;    ///< True if found the pinmame/AltColor folder (non-empty)
    bool hasPup = false;         ///< True if found the pupvideos/ folder (non-empty)
    bool hasAltMusic = false;    ///< True if found the music/ folder (non-empty)
    bool hasUltraDMD = false;    ///< True if found the *.UltraDMD folder (non-empty)
    bool hasB2S = false;         ///< True if found the *.b2s file alongside the .vpx
    bool hasINI = false;         ///< True if found the *.ini file alongside the .vpx
    bool hasVBS = false;         ///< True if found the *.vbs file alongside the .vpx
    // media scan
    bool hasPlayfieldImage = false;  ///< True if found a custom playfield image
    bool hasWheelImage = false;      ///< True if found a custom wheel image
    bool hasBackglassImage = false;  ///< True if found a custom backglass image
    bool hasDmdImage = false;        ///< True if found a custom DMD image
    bool hasTopperImage = false;     ///< True if found a custom topper image
    bool hasPlayfieldVideo = false;  ///< True if found a custom playfield video
    bool hasBackglassVideo = false;  ///< True if found a custom backglass video
    bool hasDmdVideo = false;        ///< True if found a custom DMD video
    bool hasTopperVideo = false;     ///< True if found a custom topper video
    bool hasTableMusic = false;      ///< True if found a custom table music file
    bool hasLaunchAudio = false;     ///< True if found a custom launch audio file

    uint64_t folderLastModified = 0;
    uint64_t fileLastModified = 0;   ///< Timestamp of the last modification of the .vpx file

    std::string jsonOwner;       ///< file_scanner, vpin_scanner, vpxtool_scanner, vpsdb_scanner
};

#endif // TABLE_DATA_H
