#ifndef VIDEO_PLAYER_CACHE_H
#define VIDEO_PLAYER_CACHE_H

#include <unordered_map>
#include <list>
#include <string>
#include <memory>
#include <deque>
#include "render/ivideo_player.h"

class VideoPlayerCache {
public:
    VideoPlayerCache();
    ~VideoPlayerCache();

    std::unique_ptr<IVideoPlayer> getVideoPlayer(const std::string& key, SDL_Renderer* renderer, int width, int height);
    void cacheVideoPlayer(const std::string& key, std::unique_ptr<IVideoPlayer> player, SDL_Renderer* renderer, int width, int height);
    void addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player);
    void clearOldVideoPlayers();
    void clearCache();

private:
    struct CacheEntry {
        SDL_Renderer* renderer;
        int width;
        int height;
        std::unique_ptr<IVideoPlayer> player;
        CacheEntry(SDL_Renderer* r, int w, int h, std::unique_ptr<IVideoPlayer> p)
            : renderer(r), width(w), height(h), player(std::move(p)) {}
    };

    std::unordered_map<std::string, CacheEntry> cache_;
    std::list<std::string> lruKeys_;
    std::deque<std::unique_ptr<IVideoPlayer>> oldVideoPlayers_;
    static const size_t MAX_CACHE_SIZE = 48;

    void evictOldest();
};

#endif // VIDEO_PLAYER_CACHE_H