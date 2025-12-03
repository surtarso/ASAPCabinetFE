#include "version_checker.h"
#include <curl/curl.h>
#include <iostream>
#include <algorithm>
#include <cctype>

// --- CURL WRITE CALLBACK ---
static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// --- Constructor ---
VersionChecker::VersionChecker(const std::string& currentVersion, const std::string& versionUrl)
    : currentVersion_(normalizeVersion(currentVersion)), versionUrl_(versionUrl) {}

// --- Normalize version string ---
std::string VersionChecker::normalizeVersion(const std::string& version) {
    std::string v = version;

    // Strip leading 'v'
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V'))
        v = v.substr(1);

    // Strip anything after '-'
    size_t dashPos = v.find('-');
    if (dashPos != std::string::npos)
        v = v.substr(0, dashPos);

    // Trim whitespace
    v.erase(v.begin(), std::find_if(v.begin(), v.end(), [](int ch){ return !std::isspace(ch); }));
    v.erase(std::find_if(v.rbegin(), v.rend(), [](int ch){ return !std::isspace(ch); }).base(), v.end());

    return v;
}

// --- Check for update ---
bool VersionChecker::checkForUpdate() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[VersionChecker] Failed to initialize curl." << std::endl;
        return false;
    }

    latestVersion_.clear();

    curl_easy_setopt(curl, CURLOPT_URL, versionUrl_.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &latestVersion_);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[VersionChecker] Failed to fetch latest version: "
                  << curl_easy_strerror(res) << std::endl;
        return false;
    }

    if (http_code != 200) {
        std::cerr << "[VersionChecker] Failed to fetch latest version: HTTP " << http_code << std::endl;
        return false;
    }

    // Normalize latest version
    latestVersion_ = normalizeVersion(latestVersion_);
    // latestVersion_ = "test";  // to force pop-up (debug)

    if (latestVersion_.empty()) {
        std::cerr << "[VersionChecker] Warning: remote version is empty." << std::endl;
        return false;
    }

    if (latestVersion_ != currentVersion_) {
        if (updateCallback_) {
            updateCallback_(latestVersion_);
        } else {
            std::cout << "\nA new version is available!\n"
                      << "Current: " << currentVersion_ << "\n"
                      << "Latest: " << latestVersion_ << "\n"
                      << "Download: https://github.com/surtarso/ASAPCabinetFE/releases/latest\n"
                      << std::endl;
        }
        return true;
    } else {
        std::cout << "\nYou are running the latest version.\n";
    }

    return false; // up-to-date
}
