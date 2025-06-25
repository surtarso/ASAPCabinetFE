#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include <SDL.h>
#include <unordered_map>
#include <list>
#include <string>
#include <memory>

class TextureCache {
public:
    TextureCache();
    ~TextureCache();

    SDL_Texture* getTexture(SDL_Renderer* renderer, const std::string& path);
    void clearCache();

private:
    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const {
            SDL_DestroyTexture(texture);
        }
    };

    struct CacheEntry {
        SDL_Renderer* renderer;
        std::unique_ptr<SDL_Texture, TextureDeleter> texture;
        CacheEntry(SDL_Renderer* r, SDL_Texture* t) : renderer(r), texture(t) {}
    };

    std::unordered_map<std::string, CacheEntry> cache_;
    std::list<std::string> lruKeys_;
    static const size_t MAX_CACHE_SIZE = 100;

    void evictOldest();
};

#endif // TEXTURE_CACHE_H