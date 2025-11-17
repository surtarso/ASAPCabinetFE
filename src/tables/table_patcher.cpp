#include "table_patcher.h"
#include "utils/sha_utils.h"
#include "log/logging.h"
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t totalSize = size * nmemb;
    s->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string TablePatcher::downloadHashesJson(const Settings& settings) {
    namespace fs = std::filesystem;
    std::string cachePath = settings.exeDir + "data/hashes.json";
    std::string readBuffer;

    // Ensure data directory exists
    try {
        fs::create_directories(settings.exeDir + "data/");
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to create data directory: " + std::string(e.what()));
        return "";
    }

    // Check if forceRebuildMetadata is false and cached file exists
    if (!settings.forceRebuildMetadata && fs::exists(cachePath)) {
        std::ifstream cacheFile(cachePath, std::ios::binary);
        if (cacheFile.is_open()) {
            readBuffer.assign((std::istreambuf_iterator<char>(cacheFile)), std::istreambuf_iterator<char>());
            cacheFile.close();
            LOG_INFO("Loaded hashes.json from cache: " + cachePath);

            // Get file's last-modified time for If-Modified-Since
            try {
                auto ftime = fs::last_write_time(cachePath);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                auto lastModified = std::chrono::system_clock::to_time_t(sctp);

                CURL* curl;
                CURLcode res;
                std::string tempBuffer;
                // curl_global_init(CURL_GLOBAL_DEFAULT);
                curl = curl_easy_init();
                if (curl) {
                    curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/jsm174/vpx-standalone-scripts/master/hashes.json");
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tempBuffer);
                    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
                    curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
                    curl_easy_setopt(curl, CURLOPT_TIMEVALUE, lastModified);
                    res = curl_easy_perform(curl);
                    if (res == CURLE_OK) {
                        long responseCode;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
                        if (responseCode == 304) { // Not Modified
                            LOG_DEBUG("Cached hashes.json is up-to-date");
                            curl_easy_cleanup(curl);
                            // curl_global_cleanup();
                            return readBuffer;
                        }
                    } else {
                        LOG_WARN("Failed to check hashes.json modification: " + std::string(curl_easy_strerror(res)));
                    }
                    curl_easy_cleanup(curl);
                } else {
                    LOG_ERROR("Failed to initialize curl for modification check");
                }
                // curl_global_cleanup();
            } catch (const fs::filesystem_error& e) {
                LOG_WARN("Failed to get last modified time for " + cachePath + ": " + std::string(e.what()));
            }
        } else {
            LOG_WARN("Failed to open cached hashes.json: " + cachePath);
        }
    }

    // Download fresh copy
    CURL* curl;
    CURLcode res;
    readBuffer.clear();
    // curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/jsm174/vpx-standalone-scripts/master/hashes.json");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            LOG_INFO("Successfully downloaded VBScript patches database.");
            // Save to cache
            std::ofstream outFile(cachePath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(readBuffer.data(), readBuffer.size());
                outFile.close();
                LOG_DEBUG("Saved hashes.json to: " + cachePath);
            } else {
                LOG_ERROR("Failed to save hashes.json to " + cachePath);
            }
        } else {
            LOG_ERROR("Failed to download hashes.json: " + std::string(curl_easy_strerror(res)));
            readBuffer.clear();
        }
        curl_easy_cleanup(curl);
    } else {
        LOG_ERROR("Failed to initialize curl");
    }
    // curl_global_cleanup();
    return readBuffer;
}

nlohmann::json TablePatcher::parseHashesJson(const std::string& jsonContent) {
    if (jsonContent.empty()) {
        LOG_ERROR("No content to parse from hashes.json");
        return nlohmann::json();
    }
    try {
        nlohmann::json parsed = nlohmann::json::parse(jsonContent);
        LOG_DEBUG("Successfully parsed hashes.json");
        return parsed;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("Failed to parse hashes.json: " + std::string(e.what()));
        return nlohmann::json();
    }
}

bool TablePatcher::needsPatch(TableData& table, const nlohmann::json& hashes) {
    if (table.hashFromVpx.empty()) {
        LOG_DEBUG("No hashFromVpx for table " + table.title + ", skipping patch check");
        return false;
    }

    for (const auto& entry : hashes) {
        if (entry.is_object() && entry.contains("sha256") && entry["sha256"] == table.hashFromVpx) {
            std::string patchedHash = entry["patched"]["sha256"];
            if (table.hashFromVbs.empty()) {
                LOG_WARN("No sidecar .vbs for " + table.title + ", patch needed");
                return true;
            } else if (table.hashFromVbs != patchedHash) {
                LOG_WARN("Sidecar .vbs hash mismatch for " + table.title + ", computed: " + table.hashFromVbs + ", expected: " + patchedHash);
                return true;
            } else {
                table.isPatched = true;
                LOG_INFO("Sidecar .vbs for " + table.title + " is already patched");
                return false;
            }
        }
    }
    LOG_DEBUG("No matching hash entry found for " + table.title);
    return false;
}

void TablePatcher::downloadAndSaveVbs(const std::string& url, const std::string& savePath) {
    std::string encodedUrl = url;
    size_t pos = 0;
    while ((pos = encodedUrl.find(' ', pos)) != std::string::npos) {
        encodedUrl.replace(pos, 1, "%20");
        pos += 3;
    }
    size_t refsPos = encodedUrl.find("/refs/heads/master/");
    if (refsPos != std::string::npos) {
        encodedUrl.replace(refsPos, 19, "/master/");
    }

    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    const int maxRetries = 3;
    int retryCount = 0;

    while (retryCount < maxRetries) {
        // curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, encodedUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                // Log first 16 bytes for debugging
                std::stringstream debugSs;
                debugSs << std::hex << std::setfill('0');
                for (size_t i = 0; i < std::min(readBuffer.size(), size_t(16)); ++i) {
                    debugSs << std::setw(2) << (unsigned int)(unsigned char)readBuffer[i];
                }
                LOG_DEBUG("Downloaded .vbs from " + encodedUrl + ", first 16 bytes: " + debugSs.str());

                std::ofstream outFile(savePath, std::ios::binary);
                if (outFile.is_open()) {
                    outFile.write(readBuffer.data(), readBuffer.size());
                    outFile.close();
                    LOG_INFO("Successfully saved .vbs to " + savePath);
                    curl_easy_cleanup(curl);
                    // curl_global_cleanup();
                    return;
                } else {
                    LOG_ERROR("Failed to open file for writing at " + savePath);
                }
            } else {
                LOG_ERROR("Failed to download .vbs from " + encodedUrl + ": " + std::string(curl_easy_strerror(res)));
            }
            curl_easy_cleanup(curl);
        } else {
            LOG_ERROR("Failed to initialize curl for .vbs download");
        }
        // curl_global_cleanup();
        retryCount++;
        if (retryCount < maxRetries) {
            LOG_INFO("Retrying download for " + encodedUrl + ", attempt " + std::to_string(retryCount + 1));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    LOG_ERROR("Failed to download .vbs from " + encodedUrl + " after " + std::to_string(maxRetries) + " attempts");
}

void TablePatcher::patchTables(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    std::string jsonContent = downloadHashesJson(settings);
    if (jsonContent.empty()) {
        LOG_ERROR("Aborting patch process due to empty hashes.json content");
        return;
    }

    nlohmann::json hashes = parseHashesJson(jsonContent);
    if (hashes.is_null() || !hashes.is_array()) {
        LOG_ERROR("Aborting patch process due to invalid hashes.json");
        return;
    }

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = 0;
        progress->currentTask = "Patching tables...";
    }

    for (auto& table : tables) {
        if (needsPatch(table, hashes)) {
            for (const auto& entry : hashes) {
                if (entry["sha256"] == table.hashFromVpx) {
                    std::string url = entry["patched"]["url"];
                    std::string savePath = table.folder + "/" + table.title + ".vbs";
                    LOG_INFO("Patching " + table.title + " with .vbs from " + url);
                    downloadAndSaveVbs(url, savePath);

                    std::string computedHash = compute_file_sha256(savePath);
                    if (!computedHash.empty()) {
                        std::string expectedHash = entry["patched"]["sha256"];
                        if (computedHash == expectedHash) {
                            table.hashFromVbs = computedHash;
                            table.isPatched = true;
                            LOG_DEBUG("Updated hashFromVbs for " + table.title + ": " + table.hashFromVbs);
                        } else {
                            LOG_ERROR("Hash mismatch for downloaded .vbs for " + table.title + ", computed: " + computedHash + ", expected: " + expectedHash);
                        }
                    } else {
                        LOG_ERROR("Failed to compute hash for downloaded .vbs: " + savePath);
                    }
                    break;
                }
            }
        }
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded++;
        }
    }
    LOG_INFO("Patch process completed");
}


/** * Applies the patch to a single table.
 * The table must already have the VPX file hash populated in table.hashFromVpx.
 */
bool TablePatcher::patchSingleTable(const Settings& settings, TableData& table) {
    // 1. Download and Parse the Hash Database (Same as bulk method, but we only do it once)
    std::string jsonContent = downloadHashesJson(settings);
    if (jsonContent.empty()) {
        LOG_ERROR("Aborting single patch for " + table.title + " due to empty hashes.json content");
        return false;
    }

    nlohmann::json hashes = parseHashesJson(jsonContent);
    if (hashes.is_null() || !hashes.is_array()) {
        LOG_ERROR("Aborting single patch for " + table.title + " due to invalid hashes.json");
        return false;
    }

    // 2. Check if the single table needs a patch
    if (!needsPatch(table, hashes)) {
        LOG_INFO(table.title + " is already patched or does not require a patch.");
        return table.isPatched; // Returns true if it was already patched
    }

    // 3. Find the patch entry and apply it
    for (const auto& entry : hashes) {
        if (entry.is_object() && entry.contains("sha256") && entry["sha256"] == table.hashFromVpx) {
            std::string url = entry["patched"]["url"];

            // Construct the path to the VBS file (same folder as the VPX)
            std::string savePath = table.folder + "/" + table.title + ".vbs";

            LOG_INFO("Single Patch: Downloading .vbs for " + table.title + " from " + url);
            downloadAndSaveVbs(url, savePath);

            // 4. Verify the patch (Recalculate VBS hash)
            std::string computedHash = compute_file_sha256(savePath);
            if (!computedHash.empty()) {
                std::string expectedHash = entry["patched"]["sha256"];
                if (computedHash == expectedHash) {
                    table.hashFromVbs = computedHash;
                    table.isPatched = true;
                    LOG_DEBUG("Single Patch Success: Updated hashFromVbs for " + table.title);
                    return true;
                } else {
                    LOG_ERROR("Single Patch Failed: Hash mismatch for downloaded .vbs for " + table.title);
                }
            } else {
                LOG_ERROR("Single Patch Failed: Could not compute hash for downloaded .vbs: " + savePath);
            }
            return false; // Patch failed after download attempt
        }
    }

    // If we reach here, no patch entry was found.
    LOG_INFO(table.title + " hash (" + table.hashFromVpx + ") not found in database.");
    return false;
}
