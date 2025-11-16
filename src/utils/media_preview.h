#pragma once
#include <string>
#include <unordered_map>
#include <SDL2/SDL.h>

/**
 * @brief Centralized persistent thumbnail loader & texture cache.
 *
 * - Generates scaled thumbnails using ffmpeg (once).
 * - Stores thumbnails under data/cache/thumbs/.
 * - Loads thumbnails into SDL_Textures on demand.
 * - RAM textures persist until shutdown (no auto-eviction).
 * - Disk thumbnails persist until user selects "Clear Cache".
 */
class MediaPreview {
public:
    /**
     * @brief Retrieve a thumbnail texture for an image path.
     *
     * @param renderer    SDL renderer to create textures for.
     * @param imagePath   The original HDD image path (full resolution).
     * @param maxHeight   Requested thumbnail display height.
     *
     * @return SDL_Texture* or nullptr on failure.
     */
    SDL_Texture* getThumbnail(SDL_Renderer* renderer,
                              const std::string& imagePath,
                              int maxHeight);

    /**
     * @brief Clear RAM textures only (used only on shutdown).
     */
    void clearMemoryCache();

    /**
     * @brief Persistent singleton instance.
     */
    static MediaPreview& instance();

    void setExeDir(const std::string& exeDir) { exeDir_ = exeDir; }

private:
    MediaPreview() = default;
    ~MediaPreview() = default;

    struct ThumbEntry {
        SDL_Texture* texture = nullptr;
        int height = 0;
        std::string thumbPath;    // disk thumbnail path (persistent)
    };

    std::unordered_map<std::string, ThumbEntry> memoryCache_;

    std::string computeThumbPath(const std::string& imagePath, int maxHeight);
    bool ensureThumbnail(const std::string& imagePath,
                         const std::string& thumbPath,
                         int maxHeight);

    SDL_Texture* loadTextureFromFile(SDL_Renderer* renderer,
                                     const std::string& path);

    std::string exeDir_;
};
