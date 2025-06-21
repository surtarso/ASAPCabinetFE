/**
 * @file file_scanner.cpp
 * @brief Implements the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 *
 * This file provides the implementation of the FileScanner class, which recursively scans
 * a directory for VPX files and constructs TableData objects with file paths, media
 * asset paths, and VB script hashes. The scanner uses Settings to determine the base path
 * and media preferences (e.g., images vs. videos with forceImagesOnly), and supports
 * progress tracking via LoadingProgress.
 */

#include "tables/file_scanner.h"
#include "utils/manufacturers.h"
#include "utils/path_utils.h"
#include "utils/string_utils.h"
#include "log/logging.h"
#include "vpin_wrapper.h"
#include <filesystem>
#include <regex>
#include <algorithm>
#include <cctype>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <future>
#include <chrono>

namespace fs = std::filesystem;

std::string normalize_line_endings(const std::string& input) {
    std::string result = input;
    size_t pos = 0;
    while ((pos = result.find("\r\n", pos)) != std::string::npos) {
        result.replace(pos, 2, "\n");
    }
    pos = 0;
    while ((pos = result.find("\n", pos)) != std::string::npos) {
        result.replace(pos, 1, "\r\n");
        pos += 2;
    }
    return result;
}

std::string calculate_string_sha256(const std::string& input) {
    std::string normalized = normalize_line_endings(input);
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        LOG_ERROR("FileScanner: Failed to create EVP_MD_CTX");
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("FileScanner: Failed to initialize SHA256 digest");
        return "";
    }

    if (EVP_DigestUpdate(mdctx, normalized.c_str(), normalized.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("FileScanner: Failed to update SHA256 digest");
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("FileScanner: Failed to finalize SHA256 digest");
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::vector<TableData> FileScanner::scan(const Settings& settings, LoadingProgress* progress) {
    std::vector<TableData> tables;
    std::mutex tables_mutex;  // To protect the tables vector

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

    // Step 1: Collect all VPX file paths
    std::vector<fs::path> vpx_files;
    for (const auto& entry : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            vpx_files.push_back(entry.path());
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->totalTablesToLoad++;
            }
        }
    }

    // Step 2: Process files in parallel
    std::vector<std::future<void>> futures;
    std::regex year_regex(R"(\b(19|20)\d{2}\b)");

    for (const auto& vpx_path : vpx_files) {
        futures.push_back(std::async(std::launch::async, [&settings, &year_regex, &tables, &tables_mutex, progress](const fs::path& path) {
            TableData table;
            table.vpxFile = path.string();
            table.folder = path.parent_path().string();
            table.title = path.stem().string();

            // Get file last modified time
            try {
                auto ftime = fs::last_write_time(path); // Get last modification time
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                table.fileLastModified = std::chrono::duration_cast<std::chrono::seconds>(
                    sctp.time_since_epoch()
                ).count();
            } catch (const fs::filesystem_error& e) {
                LOG_ERROR("FileScanner: Failed to get last modified time for " << path << ": " << e.what());
                table.fileLastModified = 0; // Fallback to 0 on error
            }

            // Extract Year from filename
            std::smatch match;
            if (std::regex_search(table.title, match, year_regex)) {
                table.year = match.str(0);
            } else {
                table.year = "";
            }

            // Extract Manufacturer from filename and CAPITALIZE IT
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

            // Extract and hash VB script from VPX
            char* code_ptr = get_vpx_gamedata_code(table.vpxFile.c_str());
            std::string vpx_script;  // Define locally to avoid referencing issues
            if (code_ptr != nullptr) {
                vpx_script = std::string(code_ptr);
                free_rust_string(code_ptr);
                table.codeHash = calculate_string_sha256(vpx_script);
                LOG_INFO("FileScanner: codeHash for " << table.title << ": " << table.codeHash);
            } else {
                LOG_ERROR("FileScanner: Failed to extract GameData.code for: " << table.vpxFile);
                table.codeHash = "";
            }

            // Check for corresponding .vbs file
            fs::path vbs_path = path.parent_path() / (path.stem().string() + ".vbs");
            if (fs::exists(vbs_path)) {
                std::ifstream vbs_file(vbs_path, std::ios::binary);
                if (vbs_file.is_open()) {
                    std::string vbs_content((std::istreambuf_iterator<char>(vbs_file)), std::istreambuf_iterator<char>());
                    vbs_file.close();
                    table.patchHash = calculate_string_sha256(vbs_content);
                    LOG_INFO("FileScanner: patchHash for " << vbs_path.string() << ": " << table.patchHash);

                    // Compare with VPX script if it exists
                    if (!vpx_script.empty()) {
                        table.vbsHasDiff = (vpx_script != vbs_content);
                        if (table.vbsHasDiff) {
                            LOG_INFO("FileScanner: .vbs differs from VPX script for " << table.title);
                        }
                    } else {
                        table.vbsHasDiff = false;  // No VPX script to compare, keep false
                    }
                } else {
                    LOG_ERROR("FileScanner: Failed to open .vbs file: " << vbs_path.string());
                    table.patchHash = "";
                    table.vbsHasDiff = false;
                }
            } else {
                table.patchHash = "";
                table.vbsHasDiff = false;
            }

            // Media paths (audios, images, videos)
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
                table.romPath = PathUtils::getRomPath(pinmamePath, table.romName);
            } else {
                table.altColor = false;
                table.altSound = false;
                table.romPath = "";
            }

            table.jsonOwner = "System File Scan";

            // Thread-safe addition to tables
            {
                std::lock_guard<std::mutex> lock(tables_mutex);
                tables.push_back(table);
            }

            // Thread-safe progress update
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded++;
            }
        }, vpx_path));
    }

    // Step 3: Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }

    LOG_INFO("FileScanner: Found " << tables.size() << " VPX tables");
    return tables;
}