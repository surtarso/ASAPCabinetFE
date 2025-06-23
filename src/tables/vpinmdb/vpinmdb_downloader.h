/**
 * @file vpinmdb_downloader.h
 * @brief Defines the VpinMdbImagesDownloader class for downloading table media in ASAPCabinetFE.
 *
 * This header provides the VpinMdbImagesDownloader class, which downloads media (playfield, backglass, wheel, dmd/topper)
 * from vpinmdb.json for tables with valid VPSDB IDs. It integrates with TableLoader, using Settings to control
 * downloading, resolution, and resizing, and updates TableData media paths. Progress is tracked via LoadingProgress.
 */

#ifndef VPINMDB_DOWNLOADER_H
#define VPINMDB_DOWNLOADER_H

#include "config/settings.h"
#include "tables/table_data.h"
#include "core/loading_progress.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <filesystem>
#include <string>
#include <vector>
#include <mutex>

namespace fs = std::filesystem;

/**
 * @class VpinMdbImagesDownloader
 * @brief Downloads table media from vpinmdb.json and updates TableData.
 *
 * This class handles downloading images for VPX tables based on vpsId, using libcurl and nlohmann::json.
 * It supports resolution selection (1k/4k), resizing to window sizes using ffmpeg, and DMD/topper mapping.
 * The process is controlled by Settings (fetchVpinMediaDb, resizeToWindows).
 */
class VpinMdbImagesDownloader {
public:
    /**
     * @brief Constructs a VpinMdbImagesDownloader instance.
     * @param settings Application settings controlling download behavior.
     * @param progress Optional pointer to LoadingProgress for updates.
     * @param mediaDb Optional pre-loaded vpinmdb.json (if null, loads from data/vpinmdb.json).
     */
    VpinMdbImagesDownloader(const Settings& settings, LoadingProgress* progress = nullptr, const nlohmann::json* mediaDb = nullptr);

    /**
     * @brief Downloads media for a list of tables and updates their media paths.
     * @param tables Vector of TableData to process (updated with new media paths).
     * @return True if any media was successfully downloaded.
     */
    bool downloadMedia(std::vector<TableData>& tables);

private:
    const Settings& settings_; ///< Reference to application settings.
    LoadingProgress* progress_; ///< Pointer to progress tracker (nullable).
    nlohmann::json mediaDb_; ///< Loaded vpinmdb.json.
    std::mutex mutex_; ///< Mutex for thread-safe progress updates.

    /**
     * @brief Determines the preferred resolution based on window sizes.
     * @return "4k" if any window is >=2560x1440, else "1k".
     */
    std::string selectResolution() const;

    /**
     * @brief Downloads a single file using libcurl.
     * @param url URL to download from.
     * @param destPath Destination file path.
     * @return True if successful, false otherwise.
     */
    bool downloadFile(const std::string& url, const fs::path& destPath);

    /**
     * @brief Resizes an image to the specified dimensions if resizeToWindows is true.
     * @param srcPath Source image path.
     * @param width Target width.
     * @param height Target height.
     * @return True if successful or resizing not needed, false otherwise.
     */
    bool resizeImage(const fs::path& srcPath, int width, int height);
    bool rotateImage(const fs::path& srcPath, bool shouldRotate);
};

#endif // VPINMDB_DOWNLOADER_H