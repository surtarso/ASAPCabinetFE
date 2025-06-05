/**
 * @file vps_database_updater.cpp
 * @brief Implements the VpsDatabaseUpdater class for updating the VPS database in ASAPCabinetFE.
 *
 * This file provides the implementation of the VpsDatabaseUpdater class, which checks
 * and downloads updates for the VPS database (vpsdb.json) from a remote source using
 * CURL. It compares local and remote timestamps via lastUpdated.json, supports multiple
 * fallback URLs, validates JSON content, and tracks progress with LoadingProgress. The
 * process is configurable via settings (e.g., update frequency), with potential for
 * configUI enhancements (e.g., custom URLs or frequency settings) in the future.
 */

#include "vps_database_updater.h"
#include <curl/curl.h>
#include <filesystem>
#include <json.hpp>
#include "utils/logging.h"

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t realsize = size * nmemb;
    userp->append((char*)contents, realsize); // Append received data to string
    return realsize;
}

static size_t headerCallback(char* buffer, size_t size, size_t nitems, std::string* userp) {
    size_t realsize = size * nitems;
    userp->append(buffer, realsize); // Append received headers to string
    return realsize;
}

VpsDatabaseUpdater::VpsDatabaseUpdater(const std::string& vpsDbPath) : vpsDbPath_(vpsDbPath) {}

bool VpsDatabaseUpdater::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress) {
    if (updateFrequency != "startup") {
        LOG_INFO("VpsDatabaseUpdater: VpsDb update skipped, frequency set to: " << updateFrequency);
        return fs::exists(vpsDbPath_);
    }

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Checking VPSDB update...";
        progress->currentTablesLoaded = 0;
        // Do not set totalTablesToLoad to preserve local table count
    }

    std::vector<std::string> vpsDbUrls = {
        "https://virtualpinballspreadsheet.github.io/vps-db/db/vpsdb.json"
    };
    std::string lastUpdatedUrl = "https://virtualpinballspreadsheet.github.io/vps-db/lastUpdated.json";

    try {
        std::ifstream lastUpdatedFile(lastUpdatedPath);
        long localTimestamp = 0;
        if (lastUpdatedFile.is_open()) {
            nlohmann::json lastUpdatedJson;
            lastUpdatedFile >> lastUpdatedJson; // Parse local lastUpdated.json
            if (lastUpdatedJson.contains("updatedAt")) {
                if (lastUpdatedJson["updatedAt"].is_number()) {
                    localTimestamp = lastUpdatedJson["updatedAt"].get<long>();
                } else if (lastUpdatedJson["updatedAt"].is_string()) {
                    try {
                        localTimestamp = std::stol(lastUpdatedJson["updatedAt"].get<std::string>());
                    } catch (const std::exception& e) {
                        LOG_DEBUG("VpsDatabaseUpdater: Invalid updatedAt string format: " << e.what());
                    }
                }
            }
            lastUpdatedFile.close();
        }

        std::string lastUpdatedContent, lastUpdatedHeaders;
        long httpCode = 0;
        CURL* curl = curl_easy_init();
        if (curl) {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Fetching lastUpdated.json...";
            }
            curl_easy_setopt(curl, CURLOPT_URL, lastUpdatedUrl.c_str()); // Set URL for lastUpdated.json
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &lastUpdatedContent);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &lastUpdatedHeaders);
            CURLcode res = curl_easy_perform(curl); // Perform the HTTP request
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                LOG_ERROR("VpsDatabaseUpdater: Failed to fetch lastUpdated.json: " << curl_easy_strerror(res));
                return fs::exists(vpsDbPath_);
            }
            if (httpCode != 200) {
                LOG_ERROR("VpsDatabaseUpdater: Failed to fetch lastUpdated.json, HTTP status: " << httpCode);
                return fs::exists(vpsDbPath_);
            }
            if (lastUpdatedHeaders.find("application/json") == std::string::npos) {
                LOG_ERROR("VpsDatabaseUpdater: lastUpdated.json has invalid content-type, headers: " << lastUpdatedHeaders);
                return fs::exists(vpsDbPath_);
            }
            LOG_DEBUG("VpsDatabaseUpdater: lastUpdated.json content (first 100 chars): " << lastUpdatedContent.substr(0, 100));
        } else {
            LOG_ERROR("VpsDatabaseUpdater: Failed to initialize curl");
            return fs::exists(vpsDbPath_);
        }

        nlohmann::json remoteLastUpdated;
        try {
            remoteLastUpdated = nlohmann::json::parse(lastUpdatedContent); // Parse remote lastUpdated.json
        } catch (const std::exception& e) {
            LOG_ERROR("VpsDatabaseUpdater: Failed to parse remote lastUpdated.json: " << e.what());
            return fs::exists(vpsDbPath_);
        }

        long remoteTimestamp = 0;
        if (remoteLastUpdated.is_number()) {
            remoteTimestamp = remoteLastUpdated.get<long>();
        } else if (remoteLastUpdated.is_object() && remoteLastUpdated.contains("updatedAt")) {
            if (remoteLastUpdated["updatedAt"].is_number()) {
                remoteTimestamp = remoteLastUpdated["updatedAt"].get<long>();
            } else if (remoteLastUpdated["updatedAt"].is_string()) {
                try {
                    remoteTimestamp = std::stol(remoteLastUpdated["updatedAt"].get<std::string>());
                } catch (const std::exception& e) {
                    LOG_DEBUG("VpsDatabaseUpdater: Invalid remote updatedAt string format: " << e.what());
                }
            }
        } else {
            LOG_ERROR("VpsDatabaseUpdater: Invalid lastUpdated.json format; expected number or object with 'updatedAt'");
            return fs::exists(vpsDbPath_);
        }

        if (remoteTimestamp > localTimestamp || !fs::exists(vpsDbPath_)) {
            bool downloadSuccess = false;
            for (size_t i = 0; i < vpsDbUrls.size(); ++i) {
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTask = "Downloading vpsdb.json (" + std::to_string(i + 1) + "/" + std::to_string(vpsDbUrls.size()) + ")...";
                    progress->currentTablesLoaded = i;
                    // Do not set totalTablesToLoad to preserve local table count
                }
                if (downloadVpsDb(vpsDbUrls[i], progress)) {
                    downloadSuccess = true;
                    break;
                }
            }

            if (downloadSuccess) {
                try {
                    std::ofstream lastUpdatedOut(lastUpdatedPath);
                    if (!lastUpdatedOut.is_open()) {
                        LOG_ERROR("VpsDatabaseUpdater: Failed to open " << lastUpdatedPath << " for writing");
                        return true;
                    }
                    lastUpdatedOut << remoteLastUpdated.dump(); // Save updated timestamp
                    lastUpdatedOut.close();
                    if (progress) {
                        std::lock_guard<std::mutex> lock(progress->mutex);
                        progress->currentTask = "Updated VPSDB and lastUpdated.json";
                    }
                    LOG_INFO("VpsDatabaseUpdater: Updated vpsdb.json and lastUpdated.json");
                } catch (const std::exception& e) {
                    LOG_ERROR("VpsDatabaseUpdater: Failed to save lastUpdated.json: " << e.what());
                    return true;
                }
            } else {
                LOG_ERROR("VpsDatabaseUpdater: Failed to download valid vpsdb.json from all URLs");
                return fs::exists(vpsDbPath_);
            }
        } else {
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "VPSDB is up-to-date";
            }
            LOG_INFO("VpsDatabaseUpdater: vpsdb.json is up-to-date");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("VpsDatabaseUpdater: Error checking vpsdb update: " << e.what());
        return fs::exists(vpsDbPath_);
    }
    return true;
}

bool VpsDatabaseUpdater::downloadVpsDb(const std::string& url, LoadingProgress* progress) {
    std::string vpsDbContent, vpsDbHeaders;
    long httpCode = 0;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Set URL for vpsdb.json
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &vpsDbContent);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vpsDbHeaders);
        CURLcode res = curl_easy_perform(curl); // Perform the HTTP request
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK) {
            LOG_ERROR("VpsDatabaseUpdater: Failed to download vpsdb.json from " << url << ": " << curl_easy_strerror(res));
            return false;
        }
        if (httpCode != 200) {
            LOG_ERROR("VpsDatabaseUpdater: Failed to download vpsdb.json from " << url << ", HTTP status: " << httpCode);
            return false;
        }
        if (vpsDbHeaders.find("application/json") == std::string::npos) {
            LOG_ERROR("VpsDatabaseUpdater: vpsdb.json from " << url << " has invalid content-type, headers: " << vpsDbHeaders);
            return false;
        }
        try {
            nlohmann::json parsed = nlohmann::json::parse(vpsDbContent); // Validate JSON
            fs::create_directories(fs::path(vpsDbPath_).parent_path()); // Ensure parent directories exist
            std::ofstream out(vpsDbPath_);
            if (!out.is_open()) {
                LOG_ERROR("VpsDatabaseUpdater: Failed to open " << vpsDbPath_ << " for writing");
                return false;
            }
            out << vpsDbContent; // Save downloaded content
            out.close();
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "Saved vpsdb.json from " + url;
            }
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("VpsDatabaseUpdater: Failed to process downloaded vpsdb.json from " << url << ": " << e.what());
            return false;
        }
    }
    LOG_ERROR("VpsDatabaseUpdater: Failed to initialize curl");
    return false;
}