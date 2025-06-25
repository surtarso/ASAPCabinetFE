#include "video_player_cache.h"
#include "log/logging.h"

VideoPlayerCache::VideoPlayerCache() {}

VideoPlayerCache::~VideoPlayerCache() {
    clearCache();
    clearOldVideoPlayers();
}

std::unique_ptr<IVideoPlayer> VideoPlayerCache::getVideoPlayer(const std::string& key, SDL_Renderer* renderer, int width, int height) {
    auto it = cache_.find(key);
    if (it != cache_.end() && it->second.renderer == renderer && it->second.width == width && it->second.height == height) {
        std::unique_ptr<IVideoPlayer> player = std::move(it->second.player);
        cache_.erase(it);
        lruKeys_.remove(key);
        LOG_DEBUG("VideoPlayerCache: Reused cached video player for key: " << key);
        return player;
    }
    return nullptr;
}

void VideoPlayerCache::cacheVideoPlayer(const std::string& key, std::unique_ptr<IVideoPlayer> player, SDL_Renderer* renderer, int width, int height) {
    if (!player) {
        return;
    }
    auto [it, inserted] = cache_.emplace(key, CacheEntry(renderer, width, height, std::move(player)));
    if (inserted) {
        lruKeys_.push_front(key);
        LOG_DEBUG("VideoPlayerCache: Cached video player for key: " << key);
        if (lruKeys_.size() > MAX_CACHE_SIZE) {
            evictOldest();
        }
    } else {
        addOldVideoPlayer(std::move(player));
        LOG_WARN("VideoPlayerCache: Duplicate key found for video player: " << key << ". Discarding.");
    }
}

void VideoPlayerCache::addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player) {
    if (player) {
        player->stop();
        oldVideoPlayers_.push_back(std::move(player));
        if (oldVideoPlayers_.size() > MAX_CACHE_SIZE * 2) {
            oldVideoPlayers_.pop_front();
            LOG_DEBUG("VideoPlayerCache: Removed oldest video player from oldVideoPlayers_ queue (size: " << oldVideoPlayers_.size() << ")");
        }
    }
}

void VideoPlayerCache::clearOldVideoPlayers() {
    oldVideoPlayers_.clear();
    //LOG_DEBUG("VideoPlayerCache: Cleared all old video players from queue");
}

void VideoPlayerCache::clearCache() {
    for (auto& pair : cache_) {
        if (pair.second.player) {
            pair.second.player->stop();
            addOldVideoPlayer(std::move(pair.second.player));
        }
    }
    cache_.clear();
    lruKeys_.clear();
    LOG_DEBUG("VideoPlayerCache: Video player cache cleared.");
}

void VideoPlayerCache::evictOldest() {
    if (!lruKeys_.empty()) {
        std::string keyToEvict = lruKeys_.back();
        lruKeys_.pop_back();
        auto it = cache_.find(keyToEvict);
        if (it != cache_.end()) {
            addOldVideoPlayer(std::move(it->second.player));
            cache_.erase(it);
            LOG_DEBUG("VideoPlayerCache: Evicted oldest cached video player for key: " << keyToEvict);
        }
    }
}