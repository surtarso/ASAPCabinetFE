#include "table_patcher.h"
#include "utils/sha_utils.h"
#include "log/logging.h"
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string TablePatcher::downloadHashesJson(const Settings& settings) {
    // 1. IN-MEMORY CACHE — INSTANT RETURN
    if (hasCachedHashes) {
        LOG_DEBUG("Using in-memory cached hashes.json");
        return cachedHashesJson;
    }

    fs::path cacheFilePath = settings.vbsHashPath;
    fs::create_directories(cacheFilePath.parent_path());

    std::string buffer;
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    curl_easy_setopt(curl, CURLOPT_URL, settings.vpxPatchesUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // Use cache + 304 Not Modified
    if (!settings.forceRebuildMetadata && fs::exists(cacheFilePath)) {
        auto ftime = fs::last_write_time(cacheFilePath);
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(
            ftime.time_since_epoch()).count();
        curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
        curl_easy_setopt(curl, CURLOPT_TIMEVALUE, (long)secs);
    }

    long response = 0;
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
    curl_easy_cleanup(curl);

    // 200 = fresh download
    if (res == CURLE_OK && response == 200) {
        std::ofstream(cacheFilePath, std::ios::binary) << buffer;
        cachedHashesJson = std::move(buffer);
        hasCachedHashes = true;
        LOG_INFO("Downloaded fresh hashes.json");
        return cachedHashesJson;
    }

    // 304 = not modified → use cache
    if (res == CURLE_OK && response == 304 && fs::exists(cacheFilePath)) {
        std::ifstream f(cacheFilePath);
        cachedHashesJson.assign((std::istreambuf_iterator<char>(f)), {});
        hasCachedHashes = true;
        LOG_INFO("Using cached hashes.json (304 Not Modified)");
        return cachedHashesJson;
    }

    // Fallback: try cache even if network failed
    if (fs::exists(cacheFilePath)) {
        std::ifstream f(cacheFilePath);
        cachedHashesJson.assign((std::istreambuf_iterator<char>(f)), {});
        hasCachedHashes = true;
        LOG_WARN("Using stale cache due to network failure");
        return cachedHashesJson;
    }

    return "";
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
        LOG_ERROR("Failed to obtain hashes.json");
        return;
    }

    nlohmann::json hashes = parseHashesJson(jsonContent);
    if (hashes.is_null() || !hashes.is_array()) {
        LOG_ERROR("Invalid hashes.json format");
        return;
    }

    size_t patched = 0;
    size_t checked = 0;

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Patching tables...";
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = 0;
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
                    if (!computedHash.empty() && computedHash == entry["patched"]["sha256"].get<std::string>()) {
                        table.hashFromVbs = computedHash;
                        table.isPatched = true;
                        patched++;
                    }
                    break;
                }
            }
        }

        checked++;
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded = checked;
        }
    }

    LOG_INFO("Patch process completed: " + std::to_string(patched) + " tables patched");
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
