#include "vpsdb_catalog.h"
#include "log/logging.h"
#include <SDL2/SDL_image.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace vpsdb {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    file->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void VpsdbCatalog::loadThumbnails() {

    LOG_DEBUG("VpsdbCatalog: loadThumbnails called for table ID: " << currentTable_.id);
    if (!currentTable_.tableFiles.empty()) {
        LOG_DEBUG("VpsdbCatalog: Playfield URL: " << currentTable_.tableFiles[0].imgUrl);
    } else {
        LOG_DEBUG("VpsdbCatalog: No playfield URL available");
    }
    
    // Get image URLs (use first available tableFiles and b2sFiles)
    std::string playfieldUrl, backglassUrl;
    if (!currentTable_.tableFiles.empty() && !currentTable_.tableFiles[0].imgUrl.empty()) {
        playfieldUrl = currentTable_.tableFiles[0].imgUrl;
    }
    if (!currentTable_.b2sFiles.empty() && !currentTable_.b2sFiles[0].imgUrl.empty()) {
        backglassUrl = currentTable_.b2sFiles[0].imgUrl;
    }

    // Generate cache paths
    std::string playfieldCachePath = "data/cache/" + currentTable_.id + "_playfield.webp";
    std::string backglassCachePath = "data/cache/" + currentTable_.id + "_backglass.webp";

    // Load playfield image
    if (!playfieldUrl.empty()) {
        if (fs::exists(playfieldCachePath)) {
            playfieldTexture_.reset(loadTexture(playfieldCachePath));
            currentPlayfieldPath_ = playfieldCachePath;
            LOG_DEBUG("VpsdbCatalog: Loaded playfield image from cache: " << playfieldCachePath);
        } else if (downloadImage(playfieldUrl, playfieldCachePath)) {
            playfieldTexture_.reset(loadTexture(playfieldCachePath));
            currentPlayfieldPath_ = playfieldCachePath;
            LOG_DEBUG("VpsdbCatalog: Downloaded and loaded playfield image: " << playfieldCachePath);
        }
    }

    // Load backglass image
    if (!backglassUrl.empty()) {
        if (fs::exists(backglassCachePath)) {
            backglassTexture_.reset(loadTexture(backglassCachePath));
            currentBackglassPath_ = backglassCachePath;
            LOG_DEBUG("VpsdbCatalog: Loaded backglass image from cache: " << backglassCachePath);
        } else if (downloadImage(backglassUrl, backglassCachePath)) {
            backglassTexture_.reset(loadTexture(backglassCachePath));
            currentBackglassPath_ = backglassCachePath;
            LOG_DEBUG("VpsdbCatalog: Downloaded and loaded backglass image: " << backglassCachePath);
        }
    }
}

void VpsdbCatalog::clearThumbnails() {
    playfieldTexture_.reset();
    backglassTexture_.reset();
    currentPlayfieldPath_.clear();
    currentBackglassPath_.clear();
}

bool VpsdbCatalog::downloadImage(const std::string& url, const std::string& cachePath) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("VpsdbCatalog: Failed to initialize curl for URL: " << url);
        return false;
    }

    std::ofstream file(cachePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("VpsdbCatalog: Failed to open cache file: " << cachePath);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    file.close();
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("VpsdbCatalog: Failed to download image from " << url << ": " << curl_easy_strerror(res));
        fs::remove(cachePath); // Clean up partial download
        return false;
    }

    return true;
}

SDL_Texture* VpsdbCatalog::loadTexture(const std::string& path) {
    if (!renderer_ || path.empty()) {
        LOG_ERROR("VpsdbCatalog: Invalid renderer or empty path for texture: " << path);
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        LOG_ERROR("VpsdbCatalog: Failed to load image " << path << ": " << IMG_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        LOG_ERROR("VpsdbCatalog: Failed to create texture from surface: " << SDL_GetError());
    }

    return texture;
}

} // namespace vpsdb