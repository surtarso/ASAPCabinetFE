/**
 * @file file_scanner.cpp
 * @brief Implements the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 *
 * This file provides the implementation of the FileScanner class, which recursively scans
 * a directory for VPX files and constructs TableData objects with file paths and media
 * asset paths. The scanner uses Settings to determine the base path and media preferences
 * (e.g., images vs. videos with forceImagesOnly), and supports progress tracking via
 * LoadingProgress.
 */

#include "tables/file_scanner.h"
#include "utils/manufacturers.h"
#include "utils/path_utils.h"
#include "log/logging.h"
#include <filesystem>
#include <regex>     // For year extraction
#include <algorithm> // For std::transform (for manufacturer search)
#include <cctype>    // For std::tolower (for manufacturer search)

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

    // Prepare regex for year extraction (19xx or 20xx)
    // \b word boundary, (19|20) matches 19 or 20, \d{2} matches two digits, \b word boundary
    std::regex year_regex(R"(\b(19|20)\d{2}\b)");

    // Single pass: scan and count VPX files
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            TableData table;
            // table file and folder
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            // basic title from filename without extension
            table.title = entry.path().stem().string();

            // --- Extract Year from filename ---
            std::smatch match;
            if (std::regex_search(table.title, match, year_regex)) {
                table.year = match.str(0); // The entire matched year string
                // LOG_DEBUG("FileScanner: Found year '" << table.year << "' for table: " << table.title);
            } else {
                table.year = ""; // Ensure it's empty if not found
            }

            // --- Extract Manufacturer from filename and CAPITALIZE IT ---
            std::string lowerTitle = table.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(),
                           [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

            for (const auto& manufacturerNameLower : PinballManufacturers::MANUFACTURERS_LOWERCASE) {
                if (lowerTitle.find(manufacturerNameLower) != std::string::npos) {
                    // Store the capitalized manufacturer name directly in TableData
                    table.manufacturer = PathUtils::capitalizeWords(manufacturerNameLower);
                    // LOG_DEBUG("FileScanner: Found manufacturer '" << table.manufacturer << "' for table: " << table.title);
                    break;
                }
            }
            if (table.manufacturer.empty()) {
                LOG_DEBUG("FileScanner: No known manufacturer found in filename: " << table.title);
            }

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
            table.hasPup = PathUtils::getPupPath(table.folder);
            table.hasAltMusic = PathUtils::getAltMusic(table.folder);
            table.hasUltraDMD = PathUtils::getUltraDmdPath(table.folder);

            std::string pinmamePath = PathUtils::getPinmamePath(table.folder);
            if (!pinmamePath.empty()) {
                table.altColor = PathUtils::getAltcolorPath(pinmamePath);
                table.altSound = PathUtils::getAltsoundPath(pinmamePath);
                table.romPath = PathUtils::getRomPath(pinmamePath, table.romName); // romName is passed by reference
            } else {
                // LOG_DEBUG("FileScanner: No pinmame folder found at " << table.folder << ", skipping AltColor/AltSound/ROM checks.");
                // Ensure these are explicitly false/empty if pinmamePath isn't found
                table.altColor = false;
                table.altSound = false;
                table.romPath = "";
            }
            
            table.jsonOwner = "System File Scan";
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