/**
 * @file vpinmdb_client.cpp
 * @brief Implements the VpinMdbScanner class for orchestrating table media downloads.
 *
 * This file provides the implementation for coordinating media downloads using vpinmdb_downloader
 * and vpinmdb_image components, parsing vpinmdb.json, and updating TableData media paths.
 */

#include "vpinmdb_scanner.h"
#include "vpinmdb_image.h"
#include <fstream>
#include <future>

namespace fs = std::filesystem;

VpinMdbScanner::VpinMdbScanner(const Settings& settings, LoadingProgress* progress, const nlohmann::json* mediaDb)
    : settings_(settings), progress_(progress) {
    fs::path dbPath = settings_.vpinmdbPath;
    const std::string url = settings_.vpinmdbUrl;

    if (mediaDb) {
        mediaDb_ = *mediaDb;
        return;
    }

    // Delegate download/update to the new updater class
    data::vpinmdb::VpinMdbUpdater updater(settings_, progress_);
    if (!updater.ensureAvailable()) {
        return;
    }

    // Delegate JSON loading to loader
    data::vpinmdb::VpinMdbLoader loader(settings_, progress_);
    try {
        mediaDb_ = loader.load();
    } catch (const std::exception& /*ex*/) {
        return;
    }
}

bool VpinMdbScanner::scanForMedia(std::vector<TableData>& tables) {
    if (!settings_.fetchMediaOnline) {
        LOG_WARN("Media downloading disabled (fetchMediaOnline=false)");
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Media downloading disabled (fetchMediaOnline=false)");
        }
        return false;
    }

    bool noMediaSelected =
        !settings_.downloadPlayfieldImage &&
        !settings_.downloadBackglassImage &&
        !settings_.downloadDmdImage &&
        !settings_.downloadWheelImage;

    if (noMediaSelected) {
        LOG_WARN("No Media selected to download, skipping VPin Media Database.");
        if (progress_) {
            std::lock_guard<std::mutex> lock(progress_->mutex);
            progress_->logMessages.push_back("Media downloading disabled (fetchMediaOnline=false)");
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

    for (auto& table : tables) {
        futures.push_back(std::async(std::launch::async, [&table, this, resolution, &downloadedCount]() {
            if (table.vpsId.empty()) {
                std::string msg = "Skipping media download for " + table.bestTitle + ": No VPSDB ID";
                LOG_WARN(msg);
                if (progress_) {
                    std::lock_guard<std::mutex> lock(progress_->mutex);
                    progress_->logMessages.push_back(msg);
                    progress_->numNoMatch++;
                }
                return;
            }

            bool downloaded = false;
            fs::path tableDir = table.folder;

            struct MediaInfo {
                std::string type;
                std::string* destPath;
                std::string filename;
                int width;
                int height;
            };
            std::vector<MediaInfo> mediaTypes;

            int playfieldTargetWidth = settings_.playfieldWindowWidth;
            int playfieldTargetHeight = settings_.playfieldWindowHeight;

            if (settings_.downloadPlayfieldImage) {
                mediaTypes.push_back({"table", &table.playfieldImage, settings_.customPlayfieldImage, playfieldTargetWidth, playfieldTargetHeight});
            }
            if (settings_.downloadBackglassImage) {
                mediaTypes.push_back({"bg", &table.backglassImage, settings_.customBackglassImage, settings_.backglassMediaWidth, settings_.backglassMediaHeight});
            }
            if (settings_.downloadDmdImage) {
                mediaTypes.push_back({"dmd", &table.dmdImage, settings_.customDmdImage, settings_.dmdMediaWidth, settings_.dmdMediaHeight});
            }
            if (settings_.downloadWheelImage) {
                mediaTypes.push_back({"wheel", &table.wheelImage, settings_.customWheelImage, settings_.wheelMediaWidth, settings_.wheelMediaHeight});
            }

            for (const auto& media : mediaTypes) {
                fs::path destPath = tableDir / media.filename;
                if (fs::exists(destPath)) {
                    LOG_DEBUG("Skipping " + media.type + " for " + table.bestTitle + ": File exists at " + destPath.string());
                    if (progress_) {
                        std::lock_guard<std::mutex> lock(progress_->mutex);
                        progress_->logMessages.push_back("Skipping " + media.type + " for " + table.bestTitle + ": File exists");
                    }
                    continue;
                }

                std::string url;
                try {
                    if (!mediaDb_.contains(table.vpsId)) {
                        std::string msg = "No entry for vpsId " + table.vpsId + " in vpinmdb.json for " + table.bestTitle;
                        LOG_WARN(msg);
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
                        std::string msg = "No " + media.type + " URL for " + table.bestTitle + " in " + (media.type == "wheel" ? "wheel" : resolution);
                        LOG_WARN(msg);
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back(msg);
                        }
                        continue;
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Error parsing media for " + table.bestTitle + ": " + e.what());
                    if (progress_) {
                        std::lock_guard<std::mutex> lock(progress_->mutex);
                        progress_->logMessages.push_back("Error parsing media for " + table.bestTitle + ": " + std::string(e.what()));
                    }
                    continue;
                }

                if (!fs::exists(destPath.parent_path())) {
                    try {
                        fs::create_directories(destPath.parent_path());
                        LOG_INFO("Created directory " + destPath.parent_path().string() + " for " + table.bestTitle);
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back("Created directory for " + table.bestTitle + ": " + destPath.parent_path().string());
                        }
                    } catch (const fs::filesystem_error& e) {
                        LOG_ERROR("Failed to create directory " + destPath.parent_path().string() + " for " + table.bestTitle + ": " + std::string(e.what()));
                        if (progress_) {
                            std::lock_guard<std::mutex> lock(progress_->mutex);
                            progress_->logMessages.push_back("Failed to create directory for " + table.bestTitle + ": " + std::string(e.what()));
                        }
                        continue;
                    }
                }

                if (data::filedownloader::downloadFile(url, destPath)) {
                    bool isPlayfieldImage = (media.type == "table");
                    bool isVerticalMonitor = settings_.playfieldWindowHeight > settings_.playfieldWindowWidth;
                    bool shouldAttemptRotation = isPlayfieldImage && isVerticalMonitor;

                    if (shouldAttemptRotation) {
                        LOG_INFO("Calling rotateImage for " + destPath.string() + " (Target: Playfield, Monitor: Vertical)");
                        if (!vpinmdb::rotateImage(destPath, shouldAttemptRotation)) {
                            LOG_ERROR("Failed to rotate " + media.type + " for " + table.bestTitle + " at " + destPath.string());
                            fs::remove(destPath);
                            continue;
                        }
                        LOG_INFO("rotateImage completed for " + destPath.string());
                    } else {
                        LOG_INFO("No rotation attempt for " + destPath.string() + " (Playfield: " + std::to_string(isPlayfieldImage) + ", Vertical Monitor: " + std::to_string(isVerticalMonitor) + ")");
                    }

                    if (settings_.resizeToWindows) {
                        LOG_INFO("resizeToWindows: 1, calling resizeImage for " + destPath.string() + " to " + std::to_string(media.width) + "x" + std::to_string(media.height));
                        if (!vpinmdb::resizeImage(destPath, media.width, media.height)) {
                            LOG_ERROR("Failed to resize " + media.type + " for " + table.bestTitle + " at " + destPath.string());
                            fs::remove(destPath);
                            continue;
                        }
                        LOG_INFO("resizeImage completed for " + destPath.string());
                    }
                    *media.destPath = destPath.string();
                    downloaded = true;
                    downloadedCount++;
                    LOG_INFO("Downloaded " + media.type + " for " + table.bestTitle + " to " + destPath.string());
                    if (progress_) {
                        std::lock_guard<std::mutex> lock(progress_->mutex);
                        progress_->logMessages.push_back("Downloaded " + media.type + " for " + table.bestTitle);
                        progress_->currentTablesLoaded++;
                    }
                } else {
                    LOG_ERROR("Failed to download " + media.type + " for " + table.bestTitle + " from " + url);
                }
            }

            if (downloaded && progress_) {
                std::lock_guard<std::mutex> lock(progress_->mutex);
                progress_->numMatched++;
            }
        }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    if (progress_) {
        std::lock_guard<std::mutex> lock(progress_->mutex);
    progress_->numNoMatch = static_cast<int>(tables.size() - static_cast<size_t>(progress_->numMatched));
        progress_->currentTask = "Media downloading complete";
    }
    return downloadedCount > 0;
}

std::string VpinMdbScanner::selectResolution() const {
    int maxPlayfieldDim = std::max(settings_.playfieldWindowWidth, settings_.playfieldWindowHeight);
    int maxBackglassDim = std::max(settings_.backglassWindowWidth, settings_.backglassWindowHeight);
    int maxDmdDim = std::max(settings_.dmdWindowWidth, settings_.dmdWindowHeight);

    if (maxPlayfieldDim >= 2560 && maxPlayfieldDim >= 1440 &&
        maxBackglassDim >= 2560 && maxBackglassDim >= 1440 &&
        maxDmdDim >= 2560 && maxDmdDim >= 1440) {
        LOG_INFO("Selected 4k for high-resolution displays");
        return "4k";
    }
    LOG_WARN("Selected 1k resolution for your display dimensions: Playfield: " +
             std::to_string(settings_.playfieldWindowWidth) + "x" + std::to_string(settings_.playfieldWindowHeight));
    return "1k";
}
