#include "media_preview.h"
#include "log/logging.h"
#include <SDL2/SDL_image.h>
#include <filesystem>
#include <cstdlib>      // system()
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <algorithm>

namespace fs = std::filesystem;

MediaPreview& MediaPreview::instance() {
    static MediaPreview inst;
    return inst;
}

/* ------------------------------------------------------------
   Compute persistent thumbnail location via SHA1(original_path)
   ------------------------------------------------------------ */
std::string MediaPreview::computeThumbPath(const std::string& imagePath, int maxHeight) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(imagePath.c_str()),
         imagePath.size(),
         hash);

    std::ostringstream hex;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        hex << std::hex << std::setw(2) << std::setfill('0')
            << (int)hash[i];

    // Use exeDir_ instead of relative path
    fs::path dir = fs::path(exeDir_) / "data" / "cache" / "thumbs";
    if (!fs::exists(dir))
        fs::create_directories(dir);

    return (dir / (hex.str() + "_" + std::to_string(maxHeight) + ".jpg")).string();
}


/* ------------------------------------------------------------
   Generate thumbnail (jpg) using ffmpeg
   ------------------------------------------------------------ */
bool MediaPreview::ensureThumbnail(const std::string& srcPath,
                                   const std::string& thumbPath,
                                   int maxHeight)
{
    if (fs::exists(thumbPath))
        return true;

    // Detect video by extension
    std::string ext = fs::path(srcPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool isVideo =
        (ext == ".mp4"  || ext == ".avi"  || ext == ".mov" ||
         ext == ".mkv"  || ext == ".webm" || ext == ".mpeg" ||
         ext == ".mpg");

    // ffmpeg command
    std::string cmd;
    if (isVideo) {
        cmd =
            "ffmpeg -y -hide_banner -loglevel error "
            "-ss 00:00:01 -i \"" + srcPath + "\" "
            "-vframes 1 "
            "-vf \"scale=-1:" + std::to_string(maxHeight) + "\" "
            "\"" + thumbPath + "\"";
    } else {
        cmd =
            "ffmpeg -y -hide_banner -loglevel error "
            "-i \"" + srcPath + "\" "
            "-vf \"scale=-1:" + std::to_string(maxHeight) + "\" "
            "\"" + thumbPath + "\"";
    }

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        LOG_ERROR("ffmpeg thumbnail creation failed: " + srcPath);
        return false;
    }

    return fs::exists(thumbPath);
}


/* ------------------------------------------------------------
   Load texture from thumbnail file
   ------------------------------------------------------------ */
SDL_Texture* MediaPreview::loadTextureFromFile(SDL_Renderer* renderer,
                                               const std::string& path)
{
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) {
        LOG_ERROR("IMG_Load failed: " + std::string(IMG_GetError()));
        return nullptr;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    if (!tex) {
        LOG_ERROR("SDL_CreateTextureFromSurface failed: " + std::string(SDL_GetError()));
    }

    return tex;
}

/* ------------------------------------------------------------
   Public API: getThumbnail()
   ------------------------------------------------------------ */
SDL_Texture* MediaPreview::getThumbnail(SDL_Renderer* renderer,
                                        const std::string& imagePath,
                                        int maxHeight)
{
    if (!renderer || imagePath.empty() || !fs::exists(imagePath))
        return nullptr;

    auto it = memoryCache_.find(imagePath);

    // Use cached RAM texture
    if (it != memoryCache_.end() && it->second.height == maxHeight)
        return it->second.texture;

    // Compute thumbnail location
    std::string thumbPath = computeThumbPath(imagePath, maxHeight);

    // Generate thumbnail if missing
    if (!ensureThumbnail(imagePath, thumbPath, maxHeight))
        return nullptr;

    // Load into RAM
    SDL_Texture* tex = loadTextureFromFile(renderer, thumbPath);
    if (!tex)
        return nullptr;

    // Store in cache
    memoryCache_[imagePath] = { tex, maxHeight, thumbPath };
    return tex;
}

/* ------------------------------------------------------------
   Clear RAM textures only (used on shutdown)
   ------------------------------------------------------------ */
void MediaPreview::clearMemoryCache() {
    for (auto& [key, entry] : memoryCache_) {
        if (entry.texture)
            SDL_DestroyTexture(entry.texture);
    }
    memoryCache_.clear();
}
