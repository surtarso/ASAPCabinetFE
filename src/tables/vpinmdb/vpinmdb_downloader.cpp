/**
 * @file vpinmdb_downloader.cpp
 * @brief Implements the VpinMdbImagesDownloader class for downloading table media.
 *
 * This file provides the implementation for downloading images from vpinmdb.json
 * using libcurl, parsing with nlohmann::json, and updating TableData media paths.
 * It supports resolution selection, rotation for vertical monitors, and optional resizing with ffmpeg.
 */

#include "vpinmdb_downloader.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <array>
#include <memory>
#include <future> // For std::async and std::future

namespace fs = std::filesystem;

// Callback for libcurl to write data to a file
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* out = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    out->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

VpinMdbImagesDownloader::VpinMdbImagesDownloader(const Settings& settings, LoadingProgress* progress, const nlohmann::json* mediaDb)
    : settings_(settings), progress_(progress) {
    // Load or download vpinmdb.json from data/vpinmdb.json
    fs::path dbPath = settings_.resolvePath("data/vpinmdb.json", settings_.exeDir);
    
    // If mediaDb is provided, use it
    if (mediaDb) {
        mediaDb_ = *mediaDb;
        return;
    }

    // Download vpinmdb.json if it doesn't exist
    if (!fs::exists(dbPath)) {
        const std::string url = "https://raw.githubusercontent.com/superhac/vpinmediadb/refs/heads/main/vpinmdb.json";
        // Only attempt to create directory if it doesn't exist
        if (!fs::exists(dbPath.parent_path())) {
            try {
                fs::create_directories(dbPath.parent_path());
                LOG_INFO("Created directory " << dbPath.parent_path().string());
                if (progress_) {
                    std::lock_guard<std::mutex> lock(progress_->mutex);
                    progress_->logMessages.push_back("Created directory " + dbPath.parent_path().string());
                }
            } catch (const fs::filesystem_error& e) {
                LOG_ERROR("Failed to create directory " << dbPath.parent_path().string() << ": " << e.what());
                if (progress_) {
                    std::lock_guard<std::mutex> lock(progress_->mutex);
                    progress_->logMessages.push_back("Failed to create directory for vpinmdb.json: " + std::string(e.what()));
                }
                return;
            }
        }

        // Download vpinmdb.json
        if (downloadFile(url, dbPath)) {
            LOG_INFO("Downloaded vpinmdb.json to " << dbPath.string());
            if (progress_) {
                std::lock_guard<std::mutex> lock(progress_->mutex);
                progress_->logMessages.push_back("Downloaded vpinmdb.json to " + dbPath.string());
            }
        } else {
            LOG_ERROR("Failed to download vpinmdb.json from " << url);
            if (progress_) {
                std::lock_guard<std::mutex> lock(progress_->mutex);
                progress_->logMessages.push_back("Failed to download vpinmdb.json");
            }
            return;
        }
    }

    // Load vpinmdb.json
    std::ifstream file(dbPath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open " << dbPath.string());
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Failed to open vpinmdb.json: " + dbPath.string());
        }
        return;
    }
    try {
        file >> mediaDb_;
        LOG_INFO("Loaded vpinmdb.json from " << dbPath.string());
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Loaded vpinmdb.json from " + dbPath.string());
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse vpinmdb.json: " << e.what());
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Failed to parse vpinmdb.json: " + std::string(e.what()));
        }
    }
}

bool VpinMdbImagesDownloader::downloadMedia(std::vector<TableData>& tables) {
    if (!settings_.fetchVpinMediaDb) {
        LOG_INFO("Media downloading disabled (fetchVpinMediaDb=false)");
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Media downloading disabled (fetchVpinMediaDb=false)");
        }
        return false;
    }

    if (mediaDb_.empty()) {
        LOG_ERROR("vpinmdb.json not loaded");
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Failed to load vpinmdb.json");
        }
        return false;
    }

    std::string resolution = selectResolution();
    std::vector<std::future<void>> futures;
    int downloadedCount = 0;

    // Launch async tasks for each table
    for (auto& table : tables) {
        futures.push_back(std::async(std::launch::async, [&table, this, resolution, &downloadedCount]() {
            if (table.vpsId.empty()) {
                std::string msg = "Skipping media download for " + table.title + ": No VPSDB ID";
                LOG_INFO(msg);
                if (progress_) {
                    std::lock_guard<std::mutex> lock(progress_->mutex);
                    progress_->logMessages.push_back(msg);
                    progress_->numNoMatch++;
                }
                return;
            }

            bool downloaded = false;
            fs::path tableDir = table.folder;

            // Media mappings
            struct MediaInfo {
                std::string type; // vpinmdb.json key (table, bg, wheel, dmd)
                std::string* destPath; // TableData field (playfieldImage, etc.)
                std::string filename; // Output filename (e.g., table.png)
                int width; // Target width for resizing
                int height; // Target height for resizing
            };
            std::vector<MediaInfo> mediaTypes;

            // Determine target dimensions for playfield based on monitor orientation.
            // Your settings: playfieldWindowWidth = 1080, playfieldWindowHeight = 1920
            // The rotated image (originally 1920x1080, becoming 1080x1920 after rotation)
            // should be resized to the *window* dimensions, which are already defined correctly.
            int playfieldTargetWidth = settings_.playfieldWindowWidth;
            int playfieldTargetHeight = settings_.playfieldWindowHeight;
            
            mediaTypes.push_back({"table", &table.playfieldImage, settings_.customPlayfieldImage, playfieldTargetWidth, playfieldTargetHeight});
            
            if (settings_.showBackglass) {
                mediaTypes.push_back({"bg", &table.backglassImage, settings_.customBackglassImage, settings_.backglassMediaWidth, settings_.backglassMediaHeight});
            }
            if (settings_.showDMD) {
                mediaTypes.push_back({"dmd", &table.dmdImage, settings_.customDmdImage, settings_.dmdMediaWidth, settings_.dmdMediaHeight});
            }
            if (settings_.showWheel) {
                mediaTypes.push_back({"wheel", &table.wheelImage, settings_.customWheelImage, settings_.wheelMediaWidth, settings_.wheelMediaHeight});
            }

            for (const auto& media : mediaTypes) {
                // Construct destination path
                fs::path destPath = tableDir / media.filename;
                if (fs::exists(destPath)) {
                    LOG_INFO("Skipping " << media.type << " for " << table.title << ": File exists at " << destPath.string());
                    if (progress_) {
                        std::lock_guard<std::mutex> lock(progress_->mutex);
                        progress_->logMessages.push_back("Skipping " + media.type + " for " + table.title + ": File exists");
                    }
                    continue;
                }

                // Get URL from vpinmdb.json
                std::string url;
                try {
                    if (!mediaDb_.contains(table.vpsId)) {
                        std::string msg = "No entry for vpsId " + table.vpsId + " in vpinmdb.json for " + table.title;
                        LOG_INFO(msg);
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back(msg);
                        }
                        continue;
                    }
                    auto tableEntry = mediaDb_[table.vpsId];
                    if (media.type == "wheel") {
                        url = tableEntry.value("wheel", "");
                    } else {
                        auto resEntry = tableEntry.value(resolution, nlohmann::json{});
                        url = resEntry.value(media.type, "");
                        if (url.empty() && resolution == "4k") {
                            resEntry = tableEntry.value("1k", nlohmann::json{});
                            url = resEntry.value(media.type, "");
                        }
                    }
                    if (url.empty()) {
                        std::string msg = "No " + media.type + " URL for " + table.title + " in " + (media.type == "wheel" ? "wheel" : resolution);
                        LOG_INFO(msg);
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back(msg);
                        }
                        continue;
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Error parsing media for " << table.title << ": " << e.what());
                    if (progress_) {
                        std::lock_guard<std::mutex> lock(progress_->mutex);
                        progress_->logMessages.push_back("Error parsing media for " + table.title + ": " + std::string(e.what()));
                    }
                    continue;
                }

                // Create directory if needed
                if (!fs::exists(destPath.parent_path())) {
                    try {
                        fs::create_directories(destPath.parent_path());
                        LOG_INFO("Created directory " << destPath.parent_path().string() << " for " << table.title);
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back("Created directory for " + table.title + ": " + destPath.parent_path().string());
                        }
                    } catch (const fs::filesystem_error& e) {
                        LOG_ERROR("Failed to create directory " << destPath.parent_path().string() << " for " << table.title << ": " << e.what());
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back("Failed to create directory for " + table.title + ": " + std::string(e.what()));
                        }
                        continue;
                    }
                }

                // Download file
                if (downloadFile(url, destPath)) {
                    // Determine if rotation is needed for this specific playfield image and vertical monitor.
                    bool isPlayfieldImage = (media.type == "table");
                    bool isVerticalMonitor = settings_.playfieldWindowHeight > settings_.playfieldWindowWidth;
                    bool shouldAttemptRotation = isPlayfieldImage && isVerticalMonitor;

                    if (shouldAttemptRotation) {
                        LOG_INFO("Calling rotateImage for " << destPath.string() << " (Target: Playfield, Monitor: Vertical)");
                        if (!rotateImage(destPath, shouldAttemptRotation)) { // Pass the explicit decision
                            LOG_ERROR("Failed to rotate " << media.type << " for " << table.title << " at " << destPath.string());
                            fs::remove(destPath);
                            continue;
                        }
                        LOG_INFO("rotateImage completed for " << destPath.string());
                    } else {
                        LOG_INFO("No rotation attempt for " << destPath.string() << " (Playfield: " << isPlayfieldImage << ", Vertical Monitor: " << isVerticalMonitor << ")");
                    }

                    // Resize if needed
                    if (settings_.resizeToWindows) {
                        LOG_INFO("resizeToWindows: 1, calling resizeImage for " << destPath.string() << " to " << media.width << "x" << media.height);
                        if (!resizeImage(destPath, media.width, media.height)) {
                            LOG_ERROR("Failed to resize " << media.type << " for " << table.title << " at " << destPath.string());
                            fs::remove(destPath);
                            continue;
                        }
                        LOG_INFO("resizeImage completed for " << destPath.string());
                    }
                    *media.destPath = destPath.string();
                    downloaded = true;
                    downloadedCount++;
                    LOG_INFO("Downloaded " << media.type << " for " << table.title << " to " << destPath.string());
                    if (progress_) {
                        std::lock_guard<std::mutex> lock(progress_->mutex);
                        progress_->logMessages.push_back("Downloaded " + media.type + " for " + table.title);
                        progress_->currentTablesLoaded++;
                    }
                } else {
                    LOG_ERROR("Failed to download " << media.type << " for " << table.title << " from " << url);
                }
            }

            if (downloaded && progress_) {
                std::lock_guard<std::mutex> lock(progress_->mutex);
                progress_->numMatched++;
            }
        }));
    }

    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }

    if (progress_) {
        std::lock_guard<std::mutex> lock(progress_->mutex);
        progress_->numNoMatch = tables.size() - progress_->numMatched;
        progress_->currentTask = "Media downloading complete";
    }
    return downloadedCount > 0;
}

std::string VpinMdbImagesDownloader::selectResolution() const {
    // Use 4k only if both width and height indicate a high-resolution display
    // For vertical or horizontal monitors, check max dimension
    int maxPlayfieldDim = std::max(settings_.playfieldWindowWidth, settings_.playfieldWindowHeight);
    int maxBackglassDim = std::max(settings_.backglassWindowWidth, settings_.backglassWindowHeight);
    int maxDmdDim = std::max(settings_.dmdWindowWidth, settings_.dmdWindowHeight);

    if (maxPlayfieldDim >= 2560 && maxPlayfieldDim >= 1440 &&
        maxBackglassDim >= 2560 && maxBackglassDim >= 1440 &&
        maxDmdDim >= 2560 && maxDmdDim >= 1440) {
        LOG_INFO("Selected 4k resolution for high-resolution displays");
        return "4k";
    }
    LOG_INFO("Selected 1k resolution for display dimensions (e.g., playfield: " << 
             settings_.playfieldWindowWidth << "x" << settings_.playfieldWindowHeight << ")");
    return "1k";
}

bool VpinMdbImagesDownloader::downloadFile(const std::string& url, const fs::path& destPath) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize curl for " << destPath.string());
        return false;
    }

    std::ofstream out(destPath, std::ios::binary);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open " << destPath.string());
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30-second timeout
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ASAPCabinetFE/1.0"); // Avoid GitHub rate-limiting

    CURLcode res = curl_easy_perform(curl);
    out.close();
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("curl failed for " << destPath.string() << ": " << curl_easy_strerror(res));
        fs::remove(destPath);
        return false;
    }
    return true;
}

bool VpinMdbImagesDownloader::resizeImage(const fs::path& srcPath, int width, int height) {
    LOG_INFO("resizeImage called for " << srcPath.string() << " to " << width << "x" << height);
    std::string ffmpegPath = "/usr/bin/ffmpeg";
    std::stringstream cmd;
    fs::path tempPath = srcPath.parent_path() / ("temp_" + srcPath.filename().string());
    fs::path errorLogPath = srcPath.parent_path() / ("ffmpeg_error_" + srcPath.filename().string() + ".log");

    // Adding -noautorotate and explicit format/setsar for robustness in resize step too
    cmd << "\"" << ffmpegPath << "\" -y -loglevel error -noautorotate -i \"" << srcPath.string()
        << "\" -vf \"scale=" << width << ":" << height << ",format=yuv420p,setsar=1\" \"" << tempPath.string()
        << "\" 2>\"" << errorLogPath.string() << "\"";

    LOG_INFO("Executing FFmpeg resize command: " << cmd.str());
    int ret = std::system(cmd.str().c_str());

    // Read FFmpeg error log
    std::string ffmpegError;
    {
        std::ifstream errorFile(errorLogPath);
        if (errorFile.is_open()) {
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            ffmpegError = buffer.str();
            errorFile.close();
            fs::remove(errorLogPath);
        }
    }

    if (ret != 0) {
        LOG_ERROR("FFmpeg resize failed for " << srcPath.string() << ", return code: " << ret);
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " << ffmpegError);
        }
        if (fs::exists(tempPath)) {
            fs::remove(tempPath);
        }
        return false;
    }

    // Verify output file exists
    if (!fs::exists(tempPath)) {
        LOG_ERROR("FFmpeg did not create resized image: " << tempPath.string());
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " << ffmpegError);
        }
        return false;
    }

    // Replace original file with resized file
    try {
        fs::rename(tempPath, srcPath);
        LOG_INFO("Saved resized image to " << srcPath.string() << ", dimensions: " << width << "x" << height);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to rename resized image: " << e.what());
        fs::remove(tempPath);
        return false;
    }

    return true;
}

// Modified to accept a boolean flag indicating if rotation should occur.
// This removes any re-evaluation of 'settings_' state within the function.
bool VpinMdbImagesDownloader::rotateImage(const fs::path& srcPath, bool shouldRotate) {
    LOG_INFO("Entering rotateImage for " << srcPath.string());
    LOG_INFO("rotateImage received shouldRotate parameter: " << shouldRotate); // Log the received parameter

    if (!shouldRotate) {
        LOG_INFO("Explicitly skipping rotation for " << srcPath.string() << " as 'shouldRotate' is false.");
        return true; // Indicate success (no rotation needed)
    }

    LOG_INFO("Proceeding with rotation for playfield image 90 degrees clockwise: " << srcPath.string());

    // Create temporary output path and error log
    fs::path tempPath = srcPath.parent_path() / ("temp_" + srcPath.filename().string());
    fs::path errorLogPath = srcPath.parent_path() / ("ffmpeg_error_" + srcPath.filename().string() + ".log");

    // Construct FFmpeg command for rotation:
    // -y: Overwrite output file without asking.
    // -loglevel error: Only show errors from FFmpeg.
    // -noautorotate: Crucial for ignoring input's rotation metadata and processing raw pixel data.
    // -i: Input file.
    // -vf "transpose=1,format=yuv420p,setsar=1":
    //    - transpose=1: Rotates clockwise by 90 degrees.
    //    - format=yuv420p: Forces a common, compatible pixel format.
    //    - setsar=1: Ensures square pixels to prevent subtle stretching due to aspect ratio interpretation.
    // Output file path.
    // 2> Redirects FFmpeg's stderr (error output) to the specified log file.
    std::string ffmpegPath = "/usr/bin/ffmpeg"; // <--- IMPORTANT: Verify this path for your system!
    std::stringstream cmd;
    cmd << "\"" << ffmpegPath << "\" -y -loglevel error -noautorotate -i \"" << srcPath.string()
        << "\" -vf \"transpose=1,format=yuv420p,setsar=1\" \"" << tempPath.string() << "\" 2>\"" << errorLogPath.string() << "\"";

    // Execute FFmpeg command
    LOG_INFO("Executing FFmpeg command: " << cmd.str());
    int ret = std::system(cmd.str().c_str());

    // Read FFmpeg error log (this file will contain FFmpeg's error output if any)
    std::string ffmpegError;
    {
        std::ifstream errorFile(errorLogPath);
        if (errorFile.is_open()) {
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            ffmpegError = buffer.str();
            errorFile.close();
            fs::remove(errorLogPath); // Clean up the error log file
        }
    }

    if (ret != 0) { // If FFmpeg exited with a non-zero (error) code
        LOG_ERROR("FFmpeg command failed for " << srcPath.string() << ", return code: " << ret);
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " << ffmpegError); // Report FFmpeg's own error messages
        }
        if (fs::exists(tempPath)) {
            fs::remove(tempPath); // Clean up any partially created temporary file
        }
        return false;
    }

    // Verify output file exists. FFmpeg might sometimes not return an error code but fail to write file.
    if (!fs::exists(tempPath)) {
        LOG_ERROR("FFmpeg did not create rotated image: " << tempPath.string());
        if (!ffmpegError.empty()) {
            LOG_ERROR("FFmpeg error output: " << ffmpegError);
        }
        return false;
    }

    // Replace original file with the newly rotated file
    try {
        fs::rename(tempPath, srcPath);
        LOG_INFO("Saved rotated image to " << srcPath.string());
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to rename rotated image: " << e.what()); // Report file system errors during rename
        fs::remove(tempPath); // Ensure temp file is cleaned up if rename fails
        return false;
    }

    return true; // Rotation and file replacement successful
}