#include "vpsdb_catalog_image.h"
#include "vpsdb_catalog_manager.h"
#include "log/logging.h"
#include <SDL2/SDL_image.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;

namespace vpsdb {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    file->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void VpsdbImage::loadThumbnails(VpsdbCatalog& catalog) {
    LOG_DEBUG("loadThumbnails called for table ID: " + catalog.currentTable_.id);
    if (!catalog.currentTable_.tableFiles.empty()) {
        LOG_DEBUG("Playfield URL: " + catalog.currentTable_.tableFiles[0].imgUrl);
    } else {
        LOG_DEBUG("No playfield URL available");
    }

    // Get image URLs (use first available tableFiles and b2sFiles)
    std::string playfieldUrl, backglassUrl;
    if (!catalog.currentTable_.tableFiles.empty() && !catalog.currentTable_.tableFiles[0].imgUrl.empty()) {
        playfieldUrl = catalog.currentTable_.tableFiles[0].imgUrl;
    }
    if (!catalog.currentTable_.b2sFiles.empty() && !catalog.currentTable_.b2sFiles[0].imgUrl.empty()) {
        backglassUrl = catalog.currentTable_.b2sFiles[0].imgUrl;
    }

    // Ensure cache directory exists
    fs::path cacheDir = catalog.settings_.vpsdbImageCacheDir;  // "data/cache/vpsdb_thumbs"
    if (!fs::exists(cacheDir)) {
        try {
            fs::create_directories(cacheDir);
            LOG_DEBUG("Created cache directory " + cacheDir.string());
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR("Failed to create cache directory " + cacheDir.string() + ": " + std::string(e.what()));
            return;
        }
    }

    // Generate cache paths
    std::string playfieldCachePath = cacheDir.string() + "/" + catalog.currentTable_.id + "_playfield.webp";
    std::string backglassCachePath = cacheDir.string() + "/" + catalog.currentTable_.id + "_backglass.webp";

    // Load playfield image
    if (!playfieldUrl.empty()) {
        if (fs::exists(playfieldCachePath)) {
            catalog.playfieldTexture_.reset(loadTexture(catalog, playfieldCachePath));
            catalog.currentPlayfieldPath_ = playfieldCachePath;
            LOG_DEBUG("Loaded playfield image from cache: " + playfieldCachePath);
        } else {
            if (downloadImage(playfieldUrl, playfieldCachePath)) {
                catalog.playfieldTexture_.reset(loadTexture(catalog, playfieldCachePath));
                catalog.currentPlayfieldPath_ = playfieldCachePath;
                LOG_DEBUG("Downloaded and loaded playfield image: " + playfieldCachePath);
            } else {
                LOG_INFO("Failed to download playfield image from " + playfieldUrl + ", using no texture");
            }
        }
    }

    // Load backglass image
    if (!backglassUrl.empty()) {
        if (fs::exists(backglassCachePath)) {
            catalog.backglassTexture_.reset(loadTexture(catalog, backglassCachePath));
            catalog.currentBackglassPath_ = backglassCachePath;
            LOG_DEBUG("Loaded backglass image from cache: " + backglassCachePath);
        } else {
            if (downloadImage(backglassUrl, backglassCachePath)) {
                catalog.backglassTexture_.reset(loadTexture(catalog, backglassCachePath));
                catalog.currentBackglassPath_ = backglassCachePath;
                LOG_DEBUG("Downloaded and loaded backglass image: " + backglassCachePath);
            } else {
                LOG_INFO("Failed to download backglass image from " + backglassUrl + ", using no texture");
            }
        }
    }
}

void VpsdbImage::clearThumbnails(VpsdbCatalog& catalog) {
    catalog.playfieldTexture_.reset();
    catalog.backglassTexture_.reset();
    catalog.currentPlayfieldPath_.clear();
    catalog.currentBackglassPath_.clear();
}

bool VpsdbImage::downloadImage(const std::string& url, const std::string& cachePath) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize curl for URL: " + url);
        return false;
    }

    // Ensure parent directory exists
    fs::path cacheDir = fs::path(cachePath).parent_path();
    if (!fs::exists(cacheDir)) {
        try {
            fs::create_directories(cacheDir);
            LOG_DEBUG("Created cache directory " + cacheDir.string());
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR("Failed to create cache directory " + cacheDir.string() + ": " + std::string(e.what()));
            curl_easy_cleanup(curl);
            return false;
        }
    }

    std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open cache file: " + cachePath);
        curl_easy_cleanup(curl);
        return false;
    }

    const int maxRetries = 3;
    CURLcode res;
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "ASAPCabinetFE/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode == 200) {
                file.close();
                curl_easy_cleanup(curl);
                return true;
            } else {
                LOG_INFO("HTTP error " + std::to_string(httpCode) + " for URL: " + url);
            }
        } else {
            LOG_INFO("Download attempt " + std::to_string(attempt) + " failed for " + url + ": " + std::string(curl_easy_strerror(res)));
        }

        if (attempt < maxRetries) {
            LOG_INFO("Retrying download (attempt " + std::to_string((attempt + 1)) + " of " + std::to_string(maxRetries) + ")...");
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before retry
            file.clear(); // Reset error state
            file.seekp(0); // Rewind file
            file.close();
            file.open(cachePath, std::ios::binary | std::ios::trunc); // Reopen file
            if (!file.is_open()) {
                LOG_ERROR("Failed to reopen cache file: " + cachePath);
                curl_easy_cleanup(curl);
                return false;
            }
        }
    }

    file.close();
    fs::remove(cachePath); // Clean up partial download
    curl_easy_cleanup(curl);
    LOG_ERROR("Failed to download image from " + url + " after " + std::to_string(maxRetries) + " attempts");
    return false;
}

SDL_Texture* VpsdbImage::loadTexture(VpsdbCatalog& catalog, const std::string& path) {
    if (!catalog.renderer_ || path.empty()) {
        LOG_ERROR("Invalid renderer or empty path for texture: " + path);
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        LOG_ERROR("Failed to load image " + path + ": " + IMG_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(catalog.renderer_, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        LOG_ERROR("Failed to create texture from surface: " + std::string(SDL_GetError()));
    }

    return texture;
}

} // namespace vpsdb
