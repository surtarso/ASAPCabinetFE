/**
 * @file vpx_scanner.cpp
 * @brief Implements the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 *
 * This file provides the implementation of the FileScanner class, which recursively scans
 * a directory for VPX files and constructs TableData objects with file paths and media
 * asset paths. The scanner uses Settings to determine the base path and media preferences
 * (e.g., images vs. videos with forceImagesOnly), and supports progress tracking via
 * LoadingProgress. The process is configurable, with potential for future customization
 * via configUI (e.g., additional path rules or media types).
 */

#include "tables/file_scanner.h"
#include "tables/path_utils.h"
#include "utils/logging.h"
#include <filesystem>

namespace fs = std::filesystem;

std::vector<TableData> FileScanner::scan(const Settings& settings, LoadingProgress* progress) {
    std::vector<TableData> tables;

    if (settings.VPXTablesPath.empty() || !fs::exists(settings.VPXTablesPath)) {
        LOG_ERROR("FileScanner: Invalid or empty VPX tables path: " << settings.VPXTablesPath);
        return tables;
    }

    // Initialize progress
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->totalTablesToLoad = 0;
        progress->currentTablesLoaded = 0;
    }

    // Single pass: scan and count VPX files
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            TableData table;
            // table file and folder
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            // basic title from filename without extension
            table.title = entry.path().stem().string();
            //media paths (audios, images, videos)
            table.music = PathUtils::getAudioPath(table.folder, settings.tableMusic);
            table.launchAudio = PathUtils::getAudioPath(table.folder, settings.customLaunchSound);
            table.playfieldImage = PathUtils::getImagePath(table.folder, settings.customPlayfieldImage, settings.defaultPlayfieldImage);
            table.wheelImage = PathUtils::getImagePath(table.folder, settings.customWheelImage, settings.defaultWheelImage);
            table.backglassImage = PathUtils::getImagePath(table.folder, settings.customBackglassImage, settings.defaultBackglassImage);
            table.dmdImage = PathUtils::getImagePath(table.folder, settings.customDmdImage, settings.defaultDmdImage);
            table.topperImage = PathUtils::getImagePath(table.folder, settings.customTopperImage, settings.defaultTopperImage);
            table.playfieldVideo = PathUtils::getVideoPath(table.folder, settings.customPlayfieldVideo, settings.defaultPlayfieldVideo);
            table.backglassVideo = PathUtils::getVideoPath(table.folder, settings.customBackglassVideo, settings.defaultBackglassVideo);
            table.dmdVideo = PathUtils::getVideoPath(table.folder, settings.customDmdVideo, settings.defaultDmdVideo);
            table.topperVideo = PathUtils::getVideoPath(table.folder, settings.customTopperVideo, settings.defaultTopperVideo);

            // Check for pinmame/roms/ folder and extract romName and romPath
            // TODO: getRomPath from pathUtils
            fs::path romsFolder = fs::path(table.folder) / "pinmame" / "roms";
            if (fs::exists(romsFolder) && fs::is_directory(romsFolder)) {
                for (const auto& romEntry : fs::directory_iterator(romsFolder)) {
                    if (romEntry.is_regular_file() && romEntry.path().extension() == ".zip") {
                        table.romPath = romEntry.path().string();
                        table.romName = romEntry.path().stem().string();
                        LOG_DEBUG("FileScanner: Found ROM for table " << table.vpxFile << ": romName=" << table.romName << ", romPath=" << table.romPath);
                        break; // Take the first .zip file
                    }
                }
                if (table.romName.empty()) {
                    LOG_DEBUG("FileScanner: No .zip file found in " << romsFolder.string() << " for table " << table.vpxFile);
                }
            } else {
                LOG_DEBUG("FileScanner: No pinmame/roms folder found at " << romsFolder.string() << " for table " << table.vpxFile);
            }
            //TODO: Assign the json owner for incremental updates
            table.jsonOwner = "File Scan";
            tables.push_back(table);

            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->totalTablesToLoad++;
                progress->currentTablesLoaded++;
            }
        }
    }

    LOG_INFO("FileScanner: Found " << tables.size() << " VPX tables");
    return tables;
}