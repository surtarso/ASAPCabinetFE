#include "tables/vpx_scanner.h"
#include "tables/path_utils.h"
#include "utils/logging.h"
#include <filesystem>

namespace fs = std::filesystem;

std::vector<TableData> VpxScanner::scan(const Settings& settings, LoadingProgress* progress) {
    std::vector<TableData> tables;

    if (settings.VPXTablesPath.empty() || !fs::exists(settings.VPXTablesPath)) {
        LOG_ERROR("VpxScanner: Invalid or empty VPX tables path: " << settings.VPXTablesPath);
        return tables;
    }

    // Count total VPX files for progress
    int total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            total++;
        }
    }
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->totalTablesToLoad = total;
        progress->currentTablesLoaded = 0;
    }

    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            TableData table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.title = entry.path().stem().string();
            table.music = PathUtils::getMusicPath(table.folder, settings.tableMusic);
            table.launchAudio = PathUtils::getMusicPath(table.folder, settings.customLaunchSound);
            table.playfieldImage = PathUtils::getImagePath(table.folder, settings.customPlayfieldImage, settings.defaultPlayfieldImage);
            table.wheelImage = PathUtils::getImagePath(table.folder, settings.customWheelImage, settings.defaultWheelImage);
            table.backglassImage = PathUtils::getImagePath(table.folder, settings.customBackglassImage, settings.defaultBackglassImage);
            table.dmdImage = PathUtils::getImagePath(table.folder, settings.customDmdImage, settings.defaultDmdImage);
            table.topperImage = PathUtils::getImagePath(table.folder, settings.customTopperImage, settings.defaultTopperImage);
            if (settings.forceImagesOnly) {
                table.playfieldVideo = "";
                table.backglassVideo = "";
                table.dmdVideo = "";
                table.topperVideo = "";
            } else {
                table.playfieldVideo = PathUtils::getVideoPath(table.folder, settings.customPlayfieldVideo, settings.defaultPlayfieldVideo);
                table.backglassVideo = PathUtils::getVideoPath(table.folder, settings.customBackglassVideo, settings.defaultBackglassVideo);
                table.dmdVideo = PathUtils::getVideoPath(table.folder, settings.customDmdVideo, settings.defaultDmdVideo);
                table.topperVideo = PathUtils::getVideoPath(table.folder, settings.customTopperVideo, settings.defaultTopperVideo);
            }
            tables.push_back(table);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded++;
            }
        }
    }
    return tables;
}