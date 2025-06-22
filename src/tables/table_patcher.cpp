#include "table_patcher.h"
#include "utils/sha_utils.h"
#include "log/logging.h"
#include <curl/curl.h>
#include <fstream>
#include <thread>
#include <chrono>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t totalSize = size * nmemb;
    s->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string TablePatcher::downloadHashesJson() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/jsm174/vpx-standalone-scripts/master/hashes.json");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOG_ERROR("TablePatcher: Failed to download hashes.json: " << curl_easy_strerror(res));
            readBuffer.clear();
        } else {
            LOG_INFO("TablePatcher: Successfully downloaded hashes.json");
        }
        curl_easy_cleanup(curl);
    } else {
        LOG_ERROR("TablePatcher: Failed to initialize curl");
    }
    curl_global_cleanup();
    return readBuffer;
}

nlohmann::json TablePatcher::parseHashesJson(const std::string& jsonContent) {
    if (jsonContent.empty()) {
        LOG_ERROR("TablePatcher: No content to parse from hashes.json");
        return nlohmann::json();
    }
    try {
        nlohmann::json parsed = nlohmann::json::parse(jsonContent);
        LOG_INFO("TablePatcher: Successfully parsed hashes.json");
        return parsed;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("TablePatcher: Failed to parse hashes.json: " << e.what());
        return nlohmann::json();
    }
}

bool TablePatcher::needsPatch(const TableData& table, const nlohmann::json& hashes) {
    if (table.hashFromVpx.empty()) {
        LOG_DEBUG("TablePatcher: No hashFromVpx for table " << table.title << ", skipping patch check");
        return false;
    }

    for (const auto& entry : hashes) {
        if (entry.is_object() && entry.contains("sha256") && entry["sha256"] == table.hashFromVpx) {
            std::string patchedHash = entry["patched"]["sha256"];
            if (table.hashFromVbs.empty()) {
                LOG_INFO("TablePatcher: No sidecar .vbs for " << table.title << ", patch needed");
                return true;
            } else if (table.hashFromVbs != patchedHash) {
                LOG_WARN("TablePatcher: Sidecar .vbs hash mismatch for " << table.title << ", computed: " << table.hashFromVbs << ", expected: " << patchedHash);
                return true;
            } else {
                LOG_INFO("TablePatcher: Sidecar .vbs for " << table.title << " is already patched");
                return false;
            }
        }
    }
    LOG_DEBUG("TablePatcher: No matching hash entry found for " << table.title);
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
        curl_global_init(CURL_GLOBAL_DEFAULT);
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
                LOG_DEBUG("TablePatcher: Downloaded .vbs from " << encodedUrl << ", first 16 bytes: " << debugSs.str());

                std::ofstream outFile(savePath, std::ios::binary);
                if (outFile.is_open()) {
                    outFile.write(readBuffer.data(), readBuffer.size());
                    outFile.close();
                    LOG_INFO("TablePatcher: Successfully saved .vbs to " << savePath);
                    curl_easy_cleanup(curl);
                    curl_global_cleanup();
                    return;
                } else {
                    LOG_ERROR("TablePatcher: Failed to open file for writing at " << savePath);
                }
            } else {
                LOG_ERROR("TablePatcher: Failed to download .vbs from " << encodedUrl << ": " << curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl);
        } else {
            LOG_ERROR("TablePatcher: Failed to initialize curl for .vbs download");
        }
        curl_global_cleanup();
        retryCount++;
        if (retryCount < maxRetries) {
            LOG_INFO("TablePatcher: Retrying download for " << encodedUrl << ", attempt " << retryCount + 1);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    LOG_ERROR("TablePatcher: Failed to download .vbs from " << encodedUrl << " after " << maxRetries << " attempts");
}

void TablePatcher::patchTables(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    (void)settings;

    std::string jsonContent = downloadHashesJson();
    if (jsonContent.empty()) {
        LOG_ERROR("TablePatcher: Aborting patch process due to empty hashes.json content");
        return;
    }

    nlohmann::json hashes = parseHashesJson(jsonContent);
    if (hashes.is_null() || !hashes.is_array()) {
        LOG_ERROR("TablePatcher: Aborting patch process due to invalid hashes.json");
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
                    LOG_INFO("TablePatcher: Patching " << table.title << " with .vbs from " << url);
                    downloadAndSaveVbs(url, savePath);

                    std::string computedHash = compute_file_sha256(savePath);
                    if (!computedHash.empty()) {
                        std::string expectedHash = entry["patched"]["sha256"];
                        if (computedHash == expectedHash) {
                            table.hashFromVbs = computedHash;
                            LOG_INFO("TablePatcher: Updated hashFromVbs for " << table.title << ": " << table.hashFromVbs);
                        } else {
                            LOG_ERROR("TablePatcher: Hash mismatch for downloaded .vbs for " << table.title << ", computed: " << computedHash << ", expected: " << expectedHash);
                        }
                    } else {
                        LOG_ERROR("TablePatcher: Failed to compute hash for downloaded .vbs: " << savePath);
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
    LOG_INFO("TablePatcher: Patch process completed");
}