/**
 * @file file_scanner.cpp
 * @brief Implements the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 *
 * This file provides the implementation of the FileScanner class, which recursively scans
 * a directory for VPX files and constructs TableData objects with file paths, media
 * asset paths, and VB script hashes. The scanner supports incremental updates by skipping
 * unchanged files based on an existing index, using Settings for configuration and
 * progress tracking via LoadingProgress.
 */

#include "tables/file_scanner.h"
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

namespace fs = std::filesystem;

std::vector<TableData> FileScanner::scan(const Settings& settings, LoadingProgress* progress, 
                                         const std::vector<TableData>* existingTables) {
    std::vector<TableData> tables;
    std::mutex tables_mutex;

    if (settings.VPXTablesPath.empty() || !fs::exists(settings.VPXTablesPath)) {
        LOG_ERROR("FileScanner: Invalid or empty VPX tables path: " << settings.VPXTablesPath);
        return tables;
    }

    // Build map of existing tables for quick lookup
    std::unordered_map<std::string, TableData> existingTableMap;
    if (!settings.forceRebuildMetadata && existingTables && !existingTables->empty()) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Building existing table map...";
        }
        for (const auto& table : *existingTables) {
            if (!table.vpxFile.empty()) {
                existingTableMap[table.vpxFile] = table;
            }
        }
    }

    std::vector<fs::path> vpx_files;
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            vpx_files.push_back(entry.path());
        }
    }

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->totalTablesToLoad = vpx_files.size();
        progress->currentTablesLoaded = 0;
        progress->currentTask = "Scanning VPX files...";
    }

    std::vector<std::future<void>> futures;
    std::regex year_regex(R"(\b(19|20)\d{2}\b)");

    for (const auto& vpx_path : vpx_files) {
        futures.push_back(std::async(std::launch::async, [&settings, &year_regex, &tables, &tables_mutex, 
                                                         progress, &existingTableMap](const fs::path& path) {
            // Check if file is unchanged based on index
            if (!settings.forceRebuildMetadata && !existingTableMap.empty()) {
                auto it = existingTableMap.find(path.string());
                if (it != existingTableMap.end()) {
                    const auto& existingTable = it->second;
                    uint64_t fileLastModified = 0;
                    try {
                        auto ftime = fs::last_write_time(path);
                        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                        );
                        fileLastModified = std::chrono::duration_cast<std::chrono::seconds>(
                            sctp.time_since_epoch()
                        ).count();
                    } catch (const fs::filesystem_error& e) {
                        LOG_ERROR("FileScanner: Failed to get last modified time for " << path << ": " << e.what());
                    }

                    // Compute hashes to compare
                    std::string vpxHash, vbsHash;
                    char* code_ptr = get_vpx_gamedata_code(path.string().c_str());
                    if (code_ptr != nullptr) {
                        vpxHash = calculate_string_sha256(std::string(code_ptr));
                        free_rust_string(code_ptr);
                    }
                    fs::path vbs_path = path.parent_path() / (path.stem().string() + ".vbs");
                    if (fs::exists(vbs_path)) {
                        vbsHash = compute_file_sha256(vbs_path.string());
                    }

                    // Skip if unchanged
                    if (fileLastModified == existingTable.fileLastModified && 
                        vpxHash == existingTable.hashFromVpx && 
                        vbsHash == existingTable.hashFromVbs) {
                        LOG_DEBUG("FileScanner: Skipping unchanged file: " << path.string());
                        if (progress) {
                            std::lock_guard<std::mutex> lock(progress->mutex);
                            progress->currentTablesLoaded++;
                        }
                        return;
                    }
                }
            }

            // Process new or modified file
            TableData table;
            table.vpxFile = path.string();
            table.folder = path.parent_path().string();
            table.title = path.stem().string();

            try {
                auto ftime = fs::last_write_time(path);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                table.fileLastModified = std::chrono::duration_cast<std::chrono::seconds>(
                    sctp.time_since_epoch()
                ).count();
            } catch (const fs::filesystem_error& e) {
                LOG_ERROR("FileScanner: Failed to get last modified time for " << path << ": " << e.what());
                table.fileLastModified = 0;
            }

            std::smatch match;
            if (std::regex_search(table.title, match, year_regex)) {
                table.year = match.str(0);
            } else {
                table.year = "";
            }

            std::string lowerTitle = table.title;
            std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(),
                           [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
            for (const auto& manufacturerNameLower : PinballManufacturers::MANUFACTURERS_LOWERCASE) {
                if (lowerTitle.find(manufacturerNameLower) != std::string::npos) {
                    table.manufacturer = StringUtils::capitalizeWords(manufacturerNameLower);
                    break;
                }
            }
            if (table.manufacturer.empty()) {
                LOG_DEBUG("FileScanner: No known manufacturer found in filename: " << table.title);
            }

            char* code_ptr = get_vpx_gamedata_code(table.vpxFile.c_str());
            std::string vpx_script;
            if (code_ptr != nullptr) {
                vpx_script = std::string(code_ptr);
                free_rust_string(code_ptr);
                table.hashFromVpx = calculate_string_sha256(vpx_script);
                LOG_INFO("FileScanner: hashFromVpx for " << table.title << ": " << table.hashFromVpx);
            } else {
                LOG_ERROR("FileScanner: Failed to extract GameData.code for: " << table.vpxFile);
                table.hashFromVpx = "";
            }

            fs::path vbs_path = path.parent_path() / (path.stem().string() + ".vbs");
            if (fs::exists(vbs_path)) {
                table.hashFromVbs = compute_file_sha256(vbs_path.string());
                if (table.hashFromVbs.empty()) {
                    LOG_ERROR("FileScanner: Failed to compute hash for .vbs file: " << vbs_path.string());
                    table.hasDiffVbs = false;
                } else {
                    LOG_INFO("FileScanner: hashFromVbs for " << vbs_path.string() << ": " << table.hashFromVbs);
                    if (!vpx_script.empty()) {
                        std::ifstream vbs_file(vbs_path, std::ios::binary);
                        if (vbs_file.is_open()) {
                            std::string vbs_content((std::istreambuf_iterator<char>(vbs_file)), std::istreambuf_iterator<char>());
                            table.hasDiffVbs = (vpx_script != vbs_content);
                            if (table.hasDiffVbs) {
                                LOG_INFO("FileScanner: .vbs differs from VPX script for " << table.title);
                            }
                            vbs_file.close();
                        } else {
                            LOG_ERROR("FileScanner: Failed to open .vbs file for comparison: " << vbs_path.string());
                            table.hasDiffVbs = false;
                        }
                    } else {
                        table.hasDiffVbs = false;
                    }
                }
            } else {
                table.hashFromVbs = "";
                table.hasDiffVbs = false;
                LOG_DEBUG("FileScanner: No .vbs file found for " << table.title);
            }

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
                table.hasAltColor = PathUtils::getAltcolorPath(pinmamePath);
                table.hasAltSound = PathUtils::getAltsoundPath(pinmamePath);
                table.romPath = PathUtils::getRomPath(pinmamePath, table.romName);
            } else {
                table.hasAltColor = false;
                table.hasAltSound = false;
                table.romPath = "";
            }

            table.jsonOwner = "System File Scan";

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

    for (auto& future : futures) {
        future.wait();
    }

    LOG_INFO("FileScanner: Processed " << tables.size() << " VPX tables (out of " << vpx_files.size() << " found)");
    return tables;
}