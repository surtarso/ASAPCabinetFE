#include "texture_cache.h"
#include "log/logging.h"
#include <SDL_image.h>
#include <algorithm>

TextureCache::TextureCache() {}

TextureCache::~TextureCache() {
    clearCache();
}

SDL_Texture* TextureCache::getTexture(SDL_Renderer* renderer, const std::string& path) {
    if (!renderer || path.empty()) {
        LOG_ERROR("TextureCache: Invalid renderer or empty path for texture: " << path);
        return nullptr;
    }

    auto it = cache_.find(path);
    if (it != cache_.end() && it->second.renderer == renderer) {
        lruKeys_.remove(path);
        lruKeys_.push_front(path);
        LOG_DEBUG("TextureCache: Reusing cached texture: " << path);
        return it->second.texture.get();
    }

    std::FILE* originalStderr = stderr;
    std::FILE* nullFile = fopen("/dev/null", "w");
    if (nullFile) {
        stderr = nullFile;
    } else {
        LOG_WARN("TextureCache: Failed to open null device for suppressing IMG_LoadTexture errors.");
    }

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());

    if (nullFile) {
        stderr = originalStderr;
        fclose(nullFile);
    }

    if (!tex) {
        LOG_ERROR("TextureCache: Failed to load texture " << path << ": " << IMG_GetError());
        return nullptr;
    }

    cache_.emplace(path, CacheEntry(renderer, tex));
    lruKeys_.push_front(path);
    LOG_DEBUG("TextureCache: Loaded new texture and added to cache: " << path);

    if (lruKeys_.size() > MAX_CACHE_SIZE) {
        evictOldest();
    }

    return tex;
}

void TextureCache::clearCache() {
    cache_.clear();
    lruKeys_.clear();
    LOG_DEBUG("TextureCache: Texture cache cleared.");
}

void TextureCache::evictOldest() {
    if (!lruKeys_.empty()) {
        std::string keyToEvict = lruKeys_.back();
        lruKeys_.pop_back();
        cache_.erase(keyToEvict);
        LOG_DEBUG("TextureCache: Evicted oldest cached texture for key: " << keyToEvict);
    }
}