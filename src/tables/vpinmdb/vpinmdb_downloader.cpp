/**
 * @file vpinmdb_downloader.cpp
 * @brief Implements the download functionality for VpinMdb media.
 *
 * This file provides the implementation for downloading files using libcurl.
 */

#include "vpinmdb_downloader.h"
#include <fstream>

namespace vpinmdb {

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* out = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    out->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool downloadFile(const std::string& url, const fs::path& destPath) {
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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ASAPCabinetFE/1.0");

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

} // namespace vpinmdb