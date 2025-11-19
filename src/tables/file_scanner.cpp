/**
 * @file file_scanner.cpp
 * @brief Implements the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 */

#include "file_scanner.h"
#include "utils/manufacturers.h"
#include "utils/path_utils.h"
#include "utils/string_utils.h"
#include "utils/sha_utils.h"
#include "log/logging.h"
#include "vpin_wrapper.h"
#include <filesystem>
#include <regex>
#include <algorithm>
#include <cctype>
#include <future>
#include <chrono>
#include <unordered_map>
#include <fstream>

namespace fs = std::filesystem;

// Helper to get the last modified timestamp of a folder recursively
static uint64_t getFolderLastModifiedRecursive(const fs::path& folder) {
    uint64_t lastMod = 0;
    for (auto& entry : fs::recursive_directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            try {
                auto ftime = fs::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                lastMod = std::max(lastMod,
                    static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                        sctp.time_since_epoch()).count())
                );
            } catch (...) {
                // Ignore inaccessible files
            }
        }
    }
    return lastMod;
}

std::vector<TableData> FileScanner::scan(const Settings& settings, LoadingProgress* progress,
                                         const std::vector<TableData>* existingTables) {
    std::vector<TableData> tables;
    std::mutex tables_mutex;

    if (settings.VPXTablesPath.empty() || !fs::exists(settings.VPXTablesPath)) {
        LOG_ERROR("Invalid or empty VPX tables path: " + settings.VPXTablesPath);
        return tables;
    }

    // Build existing table map
    std::unordered_map<std::string, TableData> existingTableMap;
    if (!settings.forceRebuildMetadata && existingTables && !existingTables->empty()) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Building existing table map...";
        }
        for (const auto& table : *existingTables)
            if (!table.vpxFile.empty())
                existingTableMap[table.vpxFile] = table;
    }

    // --- Incremental VPX file discovery ---
    std::vector<fs::path> vpx_files;
    size_t discoveredFiles = 0;
    for (auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            vpx_files.push_back(entry.path());
            discoveredFiles++;
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->totalTablesToLoad = discoveredFiles;
                progress->currentTablesLoaded = 0; // will be incremented later
                progress->currentTask = "Scanning VPX files...";
            }
        }
    }

    if (vpx_files.empty()) return tables;

    std::regex year_regex(R"(\b(19|20)\d{2}\b)");

    // --- Folder last modified cache ---
    std::unordered_map<std::string, uint64_t> folderLastModCache;

    // --- Parallel table processing ---
    std::vector<std::future<void>> futures;
    for (const auto& vpx_path : vpx_files) {
        futures.push_back(std::async(std::launch::async, [&settings, &year_regex, &tables, &tables_mutex,
                                                         &existingTableMap, &folderLastModCache, progress](const fs::path& path) {

            TableData table;
            table.vpxFile = path.string();
            table.folder = path.parent_path().string();
            table.title = path.stem().string();

            // --- File last modified ---
            uint64_t fileLastModified = 0;
            try {
                auto ftime = fs::last_write_time(path);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                fileLastModified = std::chrono::duration_cast<std::chrono::seconds>(
                    sctp.time_since_epoch()
                ).count();
            } catch (...) {}

            // --- Folder last modified (cached) ---
            uint64_t folderLastModified = 0;
            {
                std::lock_guard<std::mutex> lock(tables_mutex); // safe for cache
                auto it = folderLastModCache.find(table.folder);
                if (it != folderLastModCache.end())
                    folderLastModified = it->second;
                else {
                    folderLastModified = getFolderLastModifiedRecursive(path.parent_path());
                    folderLastModCache[table.folder] = folderLastModified;
                }
            }

            // check for .json override
            for (auto& o_entry : fs::directory_iterator(path.parent_path())) {
                if (!o_entry.is_regular_file())
                    continue;

                std::string ext = o_entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".json" && o_entry.path().stem() == path.stem()) {
                    table.hasOverride = true;
                    break;
                }
            }

            // --- Skip unchanged tables ---
            if (!settings.forceRebuildMetadata && !existingTableMap.empty()) {
                auto it = existingTableMap.find(path.string());
                if (it != existingTableMap.end()) {
                    const auto& existingTable = it->second;

                    bool iniNow = PathUtils::hasIniForTable(path.parent_path(), path.stem().string());
                    bool b2sNow = PathUtils::hasB2SForTable(path.parent_path(), path.stem().string());

                    bool unchanged =
                        fileLastModified == existingTable.fileLastModified &&
                        folderLastModified == existingTable.folderLastModified &&
                        iniNow == existingTable.hasINI &&
                        b2sNow == existingTable.hasB2S;

                    // Force rescan if override exists in previous metadata OR new override is detected
                    bool mustRescan = existingTable.hasOverride || table.hasOverride;

                    if (unchanged && !mustRescan) {
                        if (progress) {
                            std::lock_guard<std::mutex> lock(progress->mutex);
                            progress->currentTablesLoaded++;
                        }
                        return;
                    }
                }
            }

            // --- Year and Manufacturer ---
            std::smatch match;
            if (std::regex_search(table.title, match, year_regex))
                table.year = match.str(0);

            std::string lowerTitle = table.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
            for (const auto& manufacturerNameLower : PinballManufacturers::MANUFACTURERS_LOWERCASE) {
                if (lowerTitle.find(manufacturerNameLower) != std::string::npos) {
                    table.manufacturer = StringUtils::capitalizeWords(manufacturerNameLower);
                    break;
                }
            }

            table.fileLastModified = fileLastModified;
            table.folderLastModified = folderLastModified;

            // --- VPX GameData ---
            char* code_ptr = get_vpx_gamedata_code(table.vpxFile.c_str());
            std::string vpx_script;
            if (code_ptr) {
                vpx_script = std::string(code_ptr);
                free_rust_string(code_ptr);
                table.hashFromVpx = calculate_string_sha256(vpx_script);
            }

            // --- VBS Detection ---
            fs::path found_vbs_path;
            bool foundVbs = false;
            for (auto& entry : fs::directory_iterator(path.parent_path())) {
                if (!entry.is_regular_file()) continue;
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".vbs" && entry.path().stem() == path.stem()) {
                    found_vbs_path = entry.path();
                    foundVbs = true;
                    break;
                }
            }
            if (foundVbs) {
                table.hasVBS = true;
                table.hashFromVbs = compute_file_sha256(found_vbs_path.string());
                if (!vpx_script.empty()) {
                    std::ifstream vbs_file(found_vbs_path, std::ios::binary);
                    if (vbs_file.is_open()) {
                        std::string vbs_content((std::istreambuf_iterator<char>(vbs_file)), std::istreambuf_iterator<char>());
                        table.hasDiffVbs = (vpx_script != vbs_content);
                    }
                }
            }

            // check for .json override
            for (auto& o_entry : fs::directory_iterator(path.parent_path())) {
                if (!o_entry.is_regular_file())
                    continue;

                std::string ext = o_entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".json" && o_entry.path().stem() == path.stem()) {
                    table.hasOverride = true;
                    break;
                }
            }

            // --- Media paths (unchanged, could also cache defaults) ---
            table.music = PathUtils::getAudioPath(table.folder, settings.tableMusic);
            table.launchAudio = PathUtils::getAudioPath(table.folder, settings.customLaunchSound);
            table.playfieldImage = PathUtils::getImagePath(table.folder, settings.customPlayfieldImage, "");
            table.wheelImage = PathUtils::getImagePath(table.folder, settings.customWheelImage, settings.defaultWheelImage);
            table.backglassImage = PathUtils::getImagePath(table.folder, settings.customBackglassImage, "");
            table.dmdImage = PathUtils::getImagePath(table.folder, settings.customDmdImage, "");
            table.topperImage = PathUtils::getImagePath(table.folder, settings.customTopperImage, "");
            table.playfieldVideo = PathUtils::getVideoPath(table.folder, settings.customPlayfieldVideo);
            table.backglassVideo = PathUtils::getVideoPath(table.folder, settings.customBackglassVideo);
            table.dmdVideo = PathUtils::getVideoPath(table.folder, settings.customDmdVideo);
            table.topperVideo = PathUtils::getVideoPath(table.folder, settings.customTopperVideo);

            auto markUserAsset = [&](const std::string& assetpath, const std::string& defaultPath){
                return !assetpath.empty() && assetpath != defaultPath;
            };

            table.hasTableMusic = markUserAsset(table.music, settings.tableMusic);
            table.hasLaunchAudio = markUserAsset(table.launchAudio, settings.customLaunchSound);
            table.hasWheelImage = markUserAsset(table.wheelImage, settings.defaultWheelImage);
            table.hasPlayfieldImage = !table.playfieldImage.empty();
            table.hasBackglassImage = !table.backglassImage.empty();
            table.hasDmdImage = !table.dmdImage.empty();
            table.hasTopperImage = !table.topperImage.empty();
            table.hasPlayfieldVideo = !table.playfieldVideo.empty();
            table.hasBackglassVideo = !table.backglassVideo.empty();
            table.hasDmdVideo = !table.dmdVideo.empty();
            table.hasTopperVideo = !table.topperVideo.empty();

            // --- Folder assets ---
            table.hasPup = PathUtils::getPupPath(table.folder);
            table.hasAltMusic = PathUtils::getAltMusic(table.folder);
            table.hasUltraDMD = PathUtils::getUltraDmdPath(table.folder);
            table.hasINI = PathUtils::hasIniForTable(table.folder, path.stem().string());
            table.hasB2S = PathUtils::hasB2SForTable(table.folder, path.stem().string());

            // --- Pinmame ---
            std::string pinmamePath = PathUtils::getPinmamePath(table.folder);
            if (!pinmamePath.empty()) {
                table.hasAltColor = PathUtils::getAltcolorPath(pinmamePath);
                table.hasAltSound = PathUtils::getAltsoundPath(pinmamePath);
                table.romPath = PathUtils::getRomPath(pinmamePath, table.romName);
            }

            table.jsonOwner = "System File Scan";

            // --- Push result ---
            {
                std::lock_guard<std::mutex> lock(tables_mutex);
                tables.push_back(table);
            }
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded++;
                progress->logMessages.push_back("Processed table: " + table.vpxFile);
            }

        }, vpx_path));
    }

    for (auto& future : futures) future.wait();
    LOG_INFO("Processed " + std::to_string(tables.size()) + " VPX tables (out of " + std::to_string(vpx_files.size()) + " found)");
    return tables;
}
