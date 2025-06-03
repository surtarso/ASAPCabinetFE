#include "vps_database_updater.h"
#include <curl/curl.h>
#include <filesystem>
#include <json.hpp>
#include "utils/logging.h"

namespace fs = std::filesystem;

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t realsize = size * nmemb;
    userp->append((char*)contents, realsize);
    return realsize;
}

static size_t headerCallback(char* buffer, size_t size, size_t nitems, std::string* userp) {
    size_t realsize = size * nitems;
    userp->append(buffer, realsize);
    return realsize;
}

VpsDatabaseUpdater::VpsDatabaseUpdater(const std::string& vpsDbPath) : vpsDbPath_(vpsDbPath) {}

bool VpsDatabaseUpdater::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency) {
    if (updateFrequency != "startup") {
        LOG_INFO("VpsDatabaseUpdater: VpsDb update skipped, frequency set to: " << updateFrequency);
        return fs::exists(vpsDbPath_);
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
            lastUpdatedFile >> lastUpdatedJson;
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
            curl_easy_setopt(curl, CURLOPT_URL, lastUpdatedUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &lastUpdatedContent);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &lastUpdatedHeaders);
            CURLcode res = curl_easy_perform(curl);
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
            remoteLastUpdated = nlohmann::json::parse(lastUpdatedContent);
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
            std::string vpsDbContent, vpsDbHeaders;
            for (const auto& url : vpsDbUrls) {
                httpCode = 0;
                vpsDbContent.clear();
                vpsDbHeaders.clear();
                curl = curl_easy_init();
                if (curl) {
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &vpsDbContent);
                    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
                    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vpsDbHeaders);
                    CURLcode res = curl_easy_perform(curl);
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
                    curl_easy_cleanup(curl);
                    if (res != CURLE_OK) {
                        LOG_ERROR("VpsDatabaseUpdater: Failed to download vpsdb.json from " << url << ": " << curl_easy_strerror(res));
                        continue;
                    }
                    if (httpCode != 200) {
                        LOG_ERROR("VpsDatabaseUpdater: Failed to download vpsdb.json from " << url << ", HTTP status: " << httpCode);
                        continue;
                    }
                    if (vpsDbHeaders.find("application/json") == std::string::npos) {
                        LOG_ERROR("VpsDatabaseUpdater: vpsdb.json from " << url << " has invalid content-type, headers: " << vpsDbHeaders);
                        continue;
                    }
                    LOG_DEBUG("VpsDatabaseUpdater: vpsdb.json content (first 100 chars) from " << url << ": " << vpsDbContent.substr(0, 100));
                    try {
                        nlohmann::json parsed = nlohmann::json::parse(vpsDbContent);
                    } catch (const std::exception& e) {
                        LOG_ERROR("VpsDatabaseUpdater: Downloaded vpsdb.json from " << url << " is invalid JSON: " << e.what());
                        continue;
                    }
                    try {
                        fs::create_directories(fs::path(vpsDbPath_).parent_path());
                        std::ofstream out(vpsDbPath_);
                        if (!out.is_open()) {
                            LOG_ERROR("VpsDatabaseUpdater: Failed to open " << vpsDbPath_ << " for writing");
                            continue;
                        }
                        out << vpsDbContent;
                        out.close();
                        downloadSuccess = true;
                        break;
                    } catch (const std::exception& e) {
                        LOG_ERROR("VpsDatabaseUpdater: Failed to save vpsdb.json: " << e.what());
                        continue;
                    }
                } else {
                    LOG_ERROR("VpsDatabaseUpdater: Failed to initialize curl for " << url);
                    continue;
                }
            }

            if (downloadSuccess) {
                try {
                    std::ofstream lastUpdatedOut(lastUpdatedPath);
                    if (!lastUpdatedOut.is_open()) {
                        LOG_ERROR("VpsDatabaseUpdater: Failed to open " << lastUpdatedPath << " for writing");
                        return true;
                    }
                    lastUpdatedOut << remoteLastUpdated.dump();
                    lastUpdatedOut.close();
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
            LOG_INFO("VpsDatabaseUpdater: vpsdb.json is up-to-date");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("VpsDatabaseUpdater: Error checking vpsdb update: " << e.what());
        return fs::exists(vpsDbPath_);
    }
    return true;
}

bool VpsDatabaseUpdater::downloadVpsDb(const std::string& url) {
    std::string vpsDbContent, vpsDbHeaders;
    long httpCode = 0;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &vpsDbContent);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vpsDbHeaders);
        CURLcode res = curl_easy_perform(curl);
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
            nlohmann::json parsed = nlohmann::json::parse(vpsDbContent);
            fs::create_directories(fs::path(vpsDbPath_).parent_path());
            std::ofstream out(vpsDbPath_);
            if (!out.is_open()) {
                LOG_ERROR("VpsDatabaseUpdater: Failed to open " << vpsDbPath_ << " for writing");
                return false;
            }
            out << vpsDbContent;
            out.close();
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("VpsDatabaseUpdater: Failed to process downloaded vpsdb.json from " << url << ": " << e.what());
            return false;
        }
    }
    LOG_ERROR("VpsDatabaseUpdater: Failed to initialize curl");
    return false;
}