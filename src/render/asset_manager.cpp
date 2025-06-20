#include "render/asset_manager.h"
#include "render/video_players/video_player_factory.h"
#include "config/iconfig_service.h"
#include "log/logging.h"
#include <SDL_image.h>
#include <chrono>
#include <algorithm> // For std::remove

// Define maximum sizes for the caches
const size_t MAX_VIDEO_PLAYER_CACHE_SIZE = 48; // Increased to allow more players to stay in cache
const size_t MAX_TEXTURE_CACHE_SIZE = 100; // Increased for textures (e.g., wheels, backgrounds)

// Constructor: Initializes renderers, font, and nulls out pointers
AssetManager::AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager)
    : playfieldTexture(nullptr), // Now raw pointer
      playfieldWheelTexture(nullptr), // Now raw pointer
      playfieldTitleTexture(nullptr), // Now raw pointer
      backglassTexture(nullptr), // Now raw pointer
      backglassWheelTexture(nullptr), // Now raw pointer
      backglassTitleTexture(nullptr), // Now raw pointer
      dmdTexture(nullptr), // Now raw pointer
      dmdWheelTexture(nullptr), // Now raw pointer
      dmdTitleTexture(nullptr), // Now raw pointer
      topperTexture(nullptr), // Now raw pointer
      topperWheelTexture(nullptr), // Now raw pointer
      topperTitleTexture(nullptr), // Now raw pointer
      titleRect{0, 0, 0, 0},
      playfieldVideoPlayer(nullptr),
      backglassVideoPlayer(nullptr),
      dmdVideoPlayer(nullptr),
      topperVideoPlayer(nullptr),
      playfieldRenderer(playfield),
      backglassRenderer(backglass),
      dmdRenderer(dmd),
      topperRenderer(topper),
      soundManager_(soundManager),
      currentPlayfieldVideoPath_(),
      currentBackglassVideoPath_(),
      currentDmdVideoPath_(),
      currentTopperVideoPath_(),
      font(f),
      configManager_(nullptr),
      currentPlayfieldImagePath_(),
      currentPlayfieldWheelImagePath_(),
      currentBackglassImagePath_(),
      currentBackglassWheelImagePath_(),
      currentDmdImagePath_(),
      currentDmdWheelImagePath_(),
      currentTopperImagePath_(),
      currentTopperWheelImagePath_(),
      currentPlayfieldMediaWidth_(0),
      currentPlayfieldMediaHeight_(0),
      currentBackglassMediaWidth_(0),
      currentBackglassMediaHeight_(0),
      currentDmdMediaWidth_(0),
      currentDmdMediaHeight_(0),
      currentTopperMediaWidth_(0),
      currentTopperMediaHeight_(0) {}

void AssetManager::setSoundManager(ISoundManager* soundManager) {
    soundManager_ = soundManager;
    LOG_DEBUG("AssetManager: Sound manager set to " << soundManager);
}

void AssetManager::playTableMusic(size_t index, const std::vector<TableData>& tables) {
    if (!soundManager_ || index >= tables.size()) {
        LOG_ERROR("AssetManager: Cannot play table music: invalid soundManager or index " << index);
        return;
    }
    const std::string& musicPath = tables[index].music;
    soundManager_->playTableMusic(musicPath); // Assuming this method handles stopping previous music.
    if (!musicPath.empty()) {
        LOG_DEBUG("AssetManager: Playing table music: " << musicPath);
    } else {
        LOG_DEBUG("AssetManager: No music path for table, stopping table music (if any was playing)");
    }
}

void AssetManager::setSettingsManager(IConfigService* configService) {
    configManager_ = configService;
}

void AssetManager::setTitlePosition(int x, int y) {
        titleRect.x = x;
        titleRect.y = y;
        LOG_DEBUG("AssetManager: Updated title position to x=" << x << ", y=" << y);
}

void AssetManager::setFont(TTF_Font* font) {
    this->font = font;
    LOG_DEBUG("AssetManager: Font set to " << font);
}

void AssetManager::reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) {
    const Settings& settings = configManager_ ? configManager_->getSettings() : Settings();

    struct WindowTitleInfo {
        SDL_Renderer* renderer;
        SDL_Texture*& texture; // Now raw pointer
        const std::string& windowName;
    };

    WindowTitleInfo windows[] = {
        {playfieldRenderer, playfieldTitleTexture, "playfield"},
        {backglassRenderer, backglassTitleTexture, "backglass"},
        {dmdRenderer, dmdTitleTexture, "dmd"},
        {topperRenderer, topperTitleTexture, "topper"}
    };

    for (auto& w : windows) {
        w.texture = nullptr; // Clear old title texture (raw pointer)
        if (w.renderer && font && settings.showTitle && settings.titleWindow == w.windowName) {
            // Use the AssetManager's member titleRect for rendering.
            // The `titleRect` parameter is used to communicate the final rendered size back.
            this->titleRect.x = titleRect.x;
            this->titleRect.y = titleRect.y;
            this->titleRect.w = 0; // Reset width/height for renderText to calculate
            this->titleRect.h = 0;

            w.texture = renderText(w.renderer, font, title, color, this->titleRect); // Assign raw pointer
            int texWidth = 0, texHeight = 0;
            if (w.texture) {
                SDL_QueryTexture(w.texture, nullptr, nullptr, &texWidth, &texHeight);
                titleRect.w = this->titleRect.w; // Update the passed-in titleRect with actual dimensions
                titleRect.h = this->titleRect.h;
            }
            LOG_DEBUG("AssetManager: " << w.windowName << " title texture reloaded, font=" << font
                      << ", font_height=" << (font ? TTF_FontHeight(font) : 0)
                      << ", width=" << texWidth << ", height=" << texHeight);
        }
    }
}

void AssetManager::reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) {
    if (index >= tables.size()) {
        LOG_ERROR("AssetManager: Invalid table index " << index);
        return;
    }
    LOG_DEBUG("AssetManager: Reloading assets for table index " << index);

    // Update renderers from WindowManager as they might have changed (e.g., window recreation)
    playfieldRenderer = windowManager->getPlayfieldRenderer();
    backglassRenderer = windowManager->getBackglassRenderer();
    dmdRenderer = windowManager->getDMDRenderer();
    topperRenderer = windowManager->getTopperRenderer();
    this->font = font;

    loadTableAssets(index, tables);
    LOG_DEBUG("AssetManager: Completed asset reload for index " << index);
}

void AssetManager::clearVideoCache() {
    LOG_DEBUG("AssetManager: Clearing video player cache (including active ones)");
    // Move all cached players to oldVideoPlayers_ to be safely destructed later.
    for (auto& pair : videoPlayerCache_) {
        if (pair.second.player) {
            pair.second.player->stop();
            addOldVideoPlayer(std::move(pair.second.player)); // Use addOldVideoPlayer for consistent cleanup
        }
    }
    videoPlayerCache_.clear();
    lru_video_keys_.clear(); // Clear LRU keys list

    // Reset current active players as well, adding them to oldVideoPlayers_
    if (playfieldVideoPlayer) { playfieldVideoPlayer->stop(); addOldVideoPlayer(std::move(playfieldVideoPlayer)); }
    if (backglassVideoPlayer) { backglassVideoPlayer->stop(); addOldVideoPlayer(std::move(backglassVideoPlayer)); }
    if (dmdVideoPlayer) { dmdVideoPlayer->stop(); addOldVideoPlayer(std::move(dmdVideoPlayer)); }
    if (topperVideoPlayer) { topperVideoPlayer->stop(); addOldVideoPlayer(std::move(topperVideoPlayer)); }

    // Clear current paths and dimensions
    currentPlayfieldVideoPath_.clear();
    currentBackglassVideoPath_.clear();
    currentDmdVideoPath_.clear();
    currentTopperVideoPath_.clear();
    currentPlayfieldMediaWidth_ = 0;
    currentPlayfieldMediaHeight_ = 0;
    currentBackglassMediaWidth_ = 0;
    currentBackglassMediaHeight_ = 0;
    currentDmdMediaWidth_ = 0;
    currentDmdMediaHeight_ = 0;
    currentTopperMediaWidth_ = 0;
    currentTopperMediaHeight_ = 0;

    clearOldVideoPlayers(); // Ensure the discard list is also managed
    LOG_DEBUG("AssetManager: Video player cache and active players cleared.");
}

void AssetManager::clearTextureCache() {
    LOG_DEBUG("AssetManager: Clearing texture cache.");
    // unique_ptr's in the map will be destructed, calling SDL_DestroyTexture
    textureCache_.clear();
    lru_texture_keys_.clear();
    LOG_DEBUG("AssetManager: Texture cache cleared.");
}

void AssetManager::applyVideoAudioSettings() {
    if (!configManager_) {
        LOG_ERROR("AssetManager: Cannot apply video audio settings: configManager is null");
        return;
    }
    const Settings& settings = configManager_->getSettings();

    // Compute effective mute and volume with master settings
    bool effective_mute = settings.masterMute || settings.mediaAudioMute;
    float effective_volume = (settings.mediaAudioVol / 100.0f) * (settings.masterVol / 100.0f);

    LOG_DEBUG("AssetManager: Applying video audio settings: mediaAudioVol=" << settings.mediaAudioVol
              << ", mediaAudioMute=" << settings.mediaAudioMute
              << ", masterVol=" << settings.masterVol
              << ", masterMute=" << settings.masterMute
              << ", effective volume=" << effective_volume * 100.0f << "%, effective mute=" << effective_mute);

    struct VideoPlayerInfo {
        std::unique_ptr<IVideoPlayer>& player;
        const char* name;
    };

    VideoPlayerInfo players[] = {
        {playfieldVideoPlayer, "playfield"},
        {backglassVideoPlayer, "backglass"},
        {dmdVideoPlayer, "DMD"},
        {topperVideoPlayer, "topper"}
    };

    for (const auto& p : players) {
        if (p.player) {
            p.player->setVolume(effective_volume * 100.0f); // Assuming setVolume expects 0-100
            p.player->setMute(effective_mute);
            LOG_DEBUG("AssetManager: Applied audio settings to " << p.name << " video player: effective volume="
                      << effective_volume * 100.0f << ", effective mute=" << effective_mute);
        } else {
            LOG_DEBUG("AssetManager: No " << p.name << " video player to apply audio settings");
        }
    }
}

void AssetManager::loadTableAssets(size_t index, const std::vector<TableData>& tables) {
    auto start = std::chrono::high_resolution_clock::now();

    if (tables.empty()) {
        LOG_DEBUG("AssetManager: Tables not yet loaded, skipping asset reload");
        return;
    }
    if (index >= tables.size()) {
        LOG_ERROR("AssetManager: Invalid table index: " << index << ", table count: " << tables.size());
        return;
    }

    const Settings& settings = configManager_ ? configManager_->getSettings() : Settings();
    // Static variables to track settings changes and last loaded index
    // Using -1 for lastIndex to ensure the first call triggers a full load
    static size_t lastIndex = static_cast<size_t>(-1);
    static bool lastShowBackglass = settings.showBackglass; // Initialized to current settings on first call
    static bool lastShowDMD = settings.showDMD;
    static bool lastShowTopper = settings.showTopper;

    // Stop all active video players to release textures
    if (playfieldVideoPlayer) {
        playfieldVideoPlayer->stop();
        LOG_DEBUG("AssetManager: Stopped playfield video player");
    }
    if (backglassVideoPlayer) {
        backglassVideoPlayer->stop();
        LOG_DEBUG("AssetManager: Stopped backglass video player");
    }
    if (dmdVideoPlayer) {
        dmdVideoPlayer->stop();
        LOG_DEBUG("AssetManager: Stopped DMD video player");
    }
    if (topperVideoPlayer) {
        topperVideoPlayer->stop();
        LOG_DEBUG("AssetManager: Stopped topper video player");
    }

    // --- Optimization: Early Exit if Same Table and Settings ---
    // If the index is the same and relevant display settings haven't changed,
    // we only need to ensure any existing video players are still playing.
    if (index == lastIndex &&
        settings.showBackglass == lastShowBackglass &&
        settings.showDMD == lastShowDMD &&
        settings.showTopper == lastShowTopper) {

        // Use a temporary WindowAssetInfo to iterate over currently held players
        WindowAssetInfo currentPlayers[] = {
            {playfieldRenderer, playfieldTexture, playfieldWheelTexture, playfieldTitleTexture, playfieldVideoPlayer,
             currentPlayfieldImagePath_, currentPlayfieldWheelImagePath_, currentPlayfieldVideoPath_,
             currentPlayfieldMediaWidth_, currentPlayfieldMediaHeight_, true, "playfield", "", ""},
            {backglassRenderer, backglassTexture, backglassWheelTexture, backglassTitleTexture, backglassVideoPlayer,
             currentBackglassImagePath_, currentBackglassWheelImagePath_, currentBackglassVideoPath_,
             currentBackglassMediaWidth_, currentBackglassMediaHeight_, settings.showBackglass, "backglass", "", ""},
            {dmdRenderer, dmdTexture, dmdWheelTexture, dmdTitleTexture, dmdVideoPlayer,
             currentDmdImagePath_, currentDmdWheelImagePath_, currentDmdVideoPath_,
             currentDmdMediaWidth_, currentDmdMediaHeight_, settings.showDMD, "dmd", "", ""},
            {topperRenderer, topperTexture, topperWheelTexture, topperTitleTexture, topperVideoPlayer,
             currentTopperImagePath_, currentTopperWheelImagePath_, currentTopperVideoPath_,
             currentTopperMediaWidth_, currentTopperMediaHeight_, settings.showTopper, "topper", "", ""}
        };

        for (auto& w : currentPlayers) {
            if (w.videoPlayer && !w.videoPlayer->isPlaying()) {
                w.videoPlayer->play();
                LOG_DEBUG("AssetManager: Resumed playing active video for " << w.name << ": " << w.videoPath);
            }
        }
        LOG_INFO("AssetManager: Table " << tables[index].title << " already loaded and settings unchanged. Ensured videos are playing.");
        return; // Exit early as no reload/re-caching is needed
    }

    // --- Phase 1: Cleanup and Cache Assets from the PREVIOUS Table ---
    // This loop processes the *currently held* assets (from the table loaded by lastIndex).
    WindowAssetInfo windowsForCleanup[] = {
        {playfieldRenderer, playfieldTexture, playfieldWheelTexture, playfieldTitleTexture, playfieldVideoPlayer,
         currentPlayfieldImagePath_, currentPlayfieldWheelImagePath_, currentPlayfieldVideoPath_,
         currentPlayfieldMediaWidth_, currentPlayfieldMediaHeight_, true, "playfield", "", ""},
        {backglassRenderer, backglassTexture, backglassWheelTexture, backglassTitleTexture, backglassVideoPlayer,
         currentBackglassImagePath_, currentBackglassWheelImagePath_, currentBackglassVideoPath_,
         currentBackglassMediaWidth_, currentBackglassMediaHeight_, lastShowBackglass, "backglass", "", ""},
        {dmdRenderer, dmdTexture, dmdWheelTexture, dmdTitleTexture, dmdVideoPlayer,
         currentDmdImagePath_, currentDmdWheelImagePath_, currentDmdVideoPath_,
         currentDmdMediaWidth_, currentDmdMediaHeight_, lastShowDMD, "dmd", "", ""},
        {topperRenderer, topperTexture, topperWheelTexture, topperTitleTexture, topperVideoPlayer,
         currentTopperImagePath_, currentTopperWheelImagePath_, currentTopperVideoPath_,
         currentTopperMediaWidth_, currentTopperMediaHeight_, lastShowTopper, "topper", "", ""}
    };

    for (auto& w : windowsForCleanup) {
        // --- Video Player Caching/Cleanup ---
        if (w.videoPlayer) {
            w.videoPlayer->stop();

            if (!w.videoPath.empty() && w.mediaWidth > 0 && w.mediaHeight > 0 && w.renderer) {
                std::string cacheKey = w.videoPath + "_" + std::to_string(w.mediaWidth) + "x" + std::to_string(w.mediaHeight);

                auto [it, inserted] = videoPlayerCache_.emplace(cacheKey, VideoPlayerCacheEntry(w.renderer, w.mediaWidth, w.mediaHeight, std::move(w.videoPlayer)));
                if (inserted) {
                    lru_video_keys_.push_front(cacheKey); // Add new cached key to the front of LRU list
                    LOG_DEBUG("AssetManager: Cached video player for " << w.name << ": " << w.videoPath << " (" << cacheKey << ")");

                    // LRU eviction logic
                    if (lru_video_keys_.size() > MAX_VIDEO_PLAYER_CACHE_SIZE) {
                        std::string keyToEvict = lru_video_keys_.back();
                        lru_video_keys_.pop_back(); // Remove from LRU list
                        auto evictedEntryIt = videoPlayerCache_.find(keyToEvict);
                        if (evictedEntryIt != videoPlayerCache_.end()) {
                            addOldVideoPlayer(std::move(evictedEntryIt->second.player)); // Move player to discard queue
                            videoPlayerCache_.erase(evictedEntryIt); // Remove from cache map
                            LOG_DEBUG("AssetManager: Evicted oldest cached video player for key: " << keyToEvict);
                        }
                    }
                } else {
                    addOldVideoPlayer(std::move(w.videoPlayer));
                    LOG_WARN("AssetManager: Duplicate video player key found for " << w.name << ": " << w.videoPath << ". Discarding current player.");
                }
            } else {
                addOldVideoPlayer(std::move(w.videoPlayer));
                LOG_DEBUG("AssetManager: Discarded inactive/invalid video player for " << w.name << " (no valid path/dimensions).");
            }
            w.videoPlayer.reset(); // The unique_ptr has been moved, so reset the member
        }

        // --- Texture Cleanup (for raw pointers from previous table) ---
        // The textures themselves are owned by textureCache_. We just set the raw pointers to null.
        w.texture = nullptr;
        w.wheelTexture = nullptr;
        w.titleTexture = nullptr;

        // Clear AssetManager's Member Paths/Dimensions for the NEW Table
        w.imagePath.clear();
        w.wheelImagePath.clear();
        w.videoPath.clear();
        w.mediaWidth = 0;
        w.mediaHeight = 0;
    }

    // Update static variables for the next call
    lastIndex = index;
    lastShowBackglass = settings.showBackglass;
    lastShowDMD = settings.showDMD;
    lastShowTopper = settings.showTopper;

    const auto& table = tables[index]; // Get the table data for the current index

    LOG_DEBUG("AssetManager: Loading assets for table: " << table.title);

    // --- Phase 2: Load Assets for the NEW Table ---
    WindowAssetInfo windowsToLoad[] = {
        {playfieldRenderer, playfieldTexture, playfieldWheelTexture, playfieldTitleTexture, playfieldVideoPlayer,
         currentPlayfieldImagePath_, currentPlayfieldWheelImagePath_, currentPlayfieldVideoPath_,
         currentPlayfieldMediaWidth_, currentPlayfieldMediaHeight_, true, "playfield",
         table.playfieldImage, table.playfieldVideo},
        {backglassRenderer, backglassTexture, backglassWheelTexture, backglassTitleTexture, backglassVideoPlayer,
         currentBackglassImagePath_, currentBackglassWheelImagePath_, currentBackglassVideoPath_,
         currentBackglassMediaWidth_, currentBackglassMediaHeight_, settings.showBackglass, "backglass",
         table.backglassImage, table.backglassVideo},
        {dmdRenderer, dmdTexture, dmdWheelTexture, dmdTitleTexture, dmdVideoPlayer,
         currentDmdImagePath_, currentDmdWheelImagePath_, currentDmdVideoPath_,
         currentDmdMediaWidth_, currentDmdMediaHeight_, settings.showDMD, "dmd",
         table.dmdImage, table.dmdVideo},
        {topperRenderer, topperTexture, topperWheelTexture, topperTitleTexture, topperVideoPlayer,
         currentTopperImagePath_, currentTopperWheelImagePath_, currentTopperVideoPath_,
         currentTopperMediaWidth_, currentTopperMediaHeight_, settings.showTopper, "topper",
         table.topperImage, table.topperVideo}
    };

    for (auto& w : windowsToLoad) {
        if (!w.renderer || !w.show) {
            LOG_DEBUG("AssetManager: Skipping asset load for " << w.name << " (renderer missing or window not shown).");
            continue;
        }

        // --- Texture Loading ---
        if (!w.tableImage.empty()) {
            w.texture = loadTexture(w.renderer, w.tableImage); // loadTexture now handles caching and returns raw ptr
            if (w.texture) {
                w.imagePath = w.tableImage;
            } else {
                w.imagePath.clear();
            }
        } else {
            w.imagePath.clear();
        }

        // --- Wheel Loading ---
        LOG_DEBUG("AssetManager: Loading wheel texture for " << w.name << ": " << table.wheelImage);
        if (settings.showWheel && settings.wheelWindow == w.name && !table.wheelImage.empty()) {
            w.wheelTexture = loadTexture(w.renderer, table.wheelImage); // loadTexture now handles caching and returns raw ptr
            if (w.wheelTexture) {
                w.wheelImagePath = table.wheelImage;
            } else {
                LOG_WARN("AssetManager: Failed to load wheel texture for " << w.name << ": " << table.wheelImage);
                w.wheelImagePath.clear();
            }
        } else {
            w.wheelTexture = nullptr;
        }

        // --- Title Loading ---
        if (font && settings.showTitle && settings.titleWindow == w.name) {
            SDL_Rect currentTitleRenderRect = {settings.titleX, settings.titleY, 0, 0};
            std::string title = table.title.empty() ? "Unknown Title" : table.title;
            w.titleTexture = renderText(w.renderer, font, title, settings.fontColor, currentTitleRenderRect); // Returns new texture, not cached
            if (w.titleTexture) {
                this->titleRect.x = currentTitleRenderRect.x;
                this->titleRect.y = currentTitleRenderRect.y;
                this->titleRect.w = currentTitleRenderRect.w;
                this->titleRect.h = currentTitleRenderRect.h;
            }
        } else {
            w.titleTexture = nullptr;
        }

        // --- Video Loading ---
        int mediaWidth = 0;
        int mediaHeight = 0;
        if (w.name == std::string("playfield")) {
            mediaWidth = settings.playfieldMediaWidth;
            mediaHeight = settings.playfieldMediaHeight;
        } else if (w.name == std::string("backglass")) {
            mediaWidth = settings.backglassMediaWidth;
            mediaHeight = settings.backglassMediaHeight;
        } else if (w.name == std::string("dmd")) {
            mediaWidth = settings.dmdMediaWidth;
            mediaHeight = settings.dmdMediaHeight;
        } else if (w.name == std::string("topper")) {
            mediaWidth = settings.topperMediaWidth;
            mediaHeight = settings.topperMediaHeight;
        }

        LOG_DEBUG("AssetManager: Checking video for " << w.name << ": tableVideo=" << w.tableVideo
                  << ", desired media=" << mediaWidth << "x" << mediaHeight);

        if (!settings.forceImagesOnly && !w.tableVideo.empty() && mediaWidth > 0 && mediaHeight > 0) {
            std::string cacheKey = w.tableVideo + "_" + std::to_string(mediaWidth) + "x" + std::to_string(mediaHeight);
            auto it = videoPlayerCache_.find(cacheKey);

            if (it != videoPlayerCache_.end() && it->second.renderer == w.renderer &&
                it->second.width == mediaWidth && it->second.height == mediaHeight) {
                w.videoPlayer = std::move(it->second.player);
                videoPlayerCache_.erase(it);

                lru_video_keys_.remove(cacheKey);
                lru_video_keys_.push_front(cacheKey);

                w.videoPlayer->play();
                w.videoPath = w.tableVideo;
                w.mediaWidth = mediaWidth;
                w.mediaHeight = mediaHeight;
                LOG_DEBUG("AssetManager: Reused cached video player for " << w.name << ": " << w.tableVideo);
            } else {
                auto newPlayer = VideoPlayerFactory::createVideoPlayer(w.renderer, w.tableVideo, mediaWidth, mediaHeight, configManager_);
                if (newPlayer) {
                    w.videoPlayer = std::move(newPlayer);
                    w.videoPlayer->play();
                    w.videoPath = w.tableVideo;
                    w.mediaWidth = mediaWidth;
                    w.mediaHeight = mediaHeight;
                    LOG_DEBUG("AssetManager: Created new video player for " << w.name << ": " << w.tableVideo);
                } else {
                    LOG_WARN("AssetManager: Failed to create video player for " << w.name << ": " << w.tableVideo);
                    w.videoPlayer.reset();
                    w.videoPath.clear();
                    w.mediaWidth = 0;
                    w.mediaHeight = 0;
                }
            }
        } else {
            w.videoPlayer.reset();
            w.videoPath.clear();
            w.mediaWidth = 0;
            w.mediaHeight = 0;
            LOG_DEBUG("AssetManager: No video desired for " << w.name << " or configuration prevents video playback (forceImagesOnly or invalid params).");
        }
    }

    applyVideoAudioSettings();
    playTableMusic(index, tables);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO("Loaded " << table.title << " in " << duration << "ms");
}

void AssetManager::addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player) {
    if (player) {
        player->stop();
        oldVideoPlayers_.push_back(std::move(player));
        if (oldVideoPlayers_.size() > MAX_VIDEO_PLAYER_CACHE_SIZE * 2) {
            oldVideoPlayers_.pop_front();
            LOG_DEBUG("AssetManager: Removed oldest video player from oldVideoPlayers_ queue (size: " << oldVideoPlayers_.size() << ")");
        }
    }
}

void AssetManager::clearOldVideoPlayers() {
    oldVideoPlayers_.clear();
    //LOG_DEBUG("AssetManager: Cleared all old video players from queue");
}

SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    if (!renderer || path.empty()) {
        LOG_ERROR("AssetManager: Invalid renderer or empty path for texture: " << path);
        return nullptr;
    }

    auto it = textureCache_.find(path);
    if (it != textureCache_.end() && it->second.renderer == renderer) {
        // Found in cache, move to front of LRU list and return raw pointer
        lru_texture_keys_.remove(path); // Remove from current position
        lru_texture_keys_.push_front(path); // Add to front (most recently used)
        LOG_DEBUG("AssetManager: Reusing cached texture: " << path);
        return it->second.texture.get();
    }

    // Not found in cache, load new texture
    std::FILE* originalStderr = stderr;
    std::FILE* nullFile = nullptr;
    nullFile = fopen("/dev/null", "w");

    if (nullFile) {
        stderr = nullFile;
    } else {
        LOG_WARN("AssetManager: Failed to open null device for suppressing IMG_LoadTexture errors. Errors may appear on console.");
    }

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());

    // Restore stderr
    if (nullFile) {
        stderr = originalStderr;
        fclose(nullFile);
    }

    if (!tex) {
        LOG_ERROR("AssetManager: Failed to load texture " << path << ": " << IMG_GetError());
        return nullptr;
    }

    // Add new texture to cache and apply LRU eviction
    textureCache_.emplace(path, TextureCacheEntry(renderer, tex));
    lru_texture_keys_.push_front(path);
    LOG_DEBUG("AssetManager: Loaded new texture and added to cache: " << path);

    // LRU eviction logic for textures
    if (lru_texture_keys_.size() > MAX_TEXTURE_CACHE_SIZE) {
        std::string keyToEvict = lru_texture_keys_.back();
        lru_texture_keys_.pop_back();
        auto evictedEntryIt = textureCache_.find(keyToEvict);
        if (evictedEntryIt != textureCache_.end()) {
            // unique_ptr will handle SDL_DestroyTexture on erase
            textureCache_.erase(evictedEntryIt);
            LOG_DEBUG("AssetManager: Evicted oldest cached texture for key: " << keyToEvict);
        }
    }

    return tex;
}

SDL_Texture* AssetManager::renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message,
                                     SDL_Color color, SDL_Rect& textRect) {
    if (!font || !renderer || message.empty()) {
        LOG_ERROR("AssetManager: Invalid font, renderer, or empty message for renderText");
        return nullptr;
    }

    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        LOG_ERROR("AssetManager: TTF_RenderUTF8_Blended error: " << TTF_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture) {
        LOG_ERROR("AssetManager: SDL_CreateTextureFromSurface error: " << SDL_GetError());
    } else {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textRect.w = surf->w;
        textRect.h = surf->h;
    }

    SDL_FreeSurface(surf);
    // Note: Rendered text textures are not currently cached by `loadTexture`
    // because their content depends on font, color, and message, not just path.
    // They are created fresh each time `reloadTitleTexture` is called.
    return texture;
}

void AssetManager::cleanupVideoPlayers() {
    LOG_DEBUG("AssetManager: Cleaning up video players (active ones)");

    struct VideoPlayerInfo {
        std::unique_ptr<IVideoPlayer>& player;
        std::string& videoPath;
        int& mediaWidth;
        int& mediaHeight;
        const char* name;
    };

    VideoPlayerInfo players[] = {
        {playfieldVideoPlayer, currentPlayfieldVideoPath_, currentPlayfieldMediaWidth_, currentPlayfieldMediaHeight_, "playfield"},
        {backglassVideoPlayer, currentBackglassVideoPath_, currentBackglassMediaWidth_, currentBackglassMediaHeight_, "backglass"},
        {dmdVideoPlayer, currentDmdVideoPath_, currentDmdMediaWidth_, currentDmdMediaHeight_, "DMD"},
        {topperVideoPlayer, currentTopperVideoPath_, currentTopperMediaWidth_, currentTopperMediaHeight_, "topper"}
    };

    for (auto& p : players) {
        if (p.player) {
            p.player->stop();
            addOldVideoPlayer(std::move(p.player));
            p.player.reset();
            p.videoPath.clear();
            p.mediaWidth = 0;
            p.mediaHeight = 0;
            LOG_DEBUG("AssetManager: Moved " << p.name << " video player to oldVideoPlayers_ for cleanup.");
        }
    }

    for (auto& pair : videoPlayerCache_) {
        if (pair.second.player) {
            pair.second.player->stop();
            addOldVideoPlayer(std::move(pair.second.player));
        }
    }
    videoPlayerCache_.clear();
    lru_video_keys_.clear();

    clearOldVideoPlayers();
    LOG_DEBUG("AssetManager: All video players and cache entries processed for cleanup.");
}