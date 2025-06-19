#include "render/asset_manager.h"
#include "render/video_players/video_player_factory.h"
#include "config/iconfig_service.h"
#include "log/logging.h"
#include <SDL_image.h>
#include <chrono>

// Constructor: Initializes renderers, font, and nulls out pointers
AssetManager::AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager)
    : playfieldTexture(nullptr, SDL_DestroyTexture),
      playfieldWheelTexture(nullptr, SDL_DestroyTexture),
      playfieldTitleTexture(nullptr, SDL_DestroyTexture),
      backglassTexture(nullptr, SDL_DestroyTexture),
      backglassWheelTexture(nullptr, SDL_DestroyTexture),
      backglassTitleTexture(nullptr, SDL_DestroyTexture),
      dmdTexture(nullptr, SDL_DestroyTexture),
      dmdWheelTexture(nullptr, SDL_DestroyTexture),
      dmdTitleTexture(nullptr, SDL_DestroyTexture),
      topperTexture(nullptr, SDL_DestroyTexture),
      topperWheelTexture(nullptr, SDL_DestroyTexture),
      topperTitleTexture(nullptr, SDL_DestroyTexture),
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
        std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>& texture;
        const std::string& windowName;
    };

    WindowTitleInfo windows[] = {
        {playfieldRenderer, playfieldTitleTexture, "playfield"},
        {backglassRenderer, backglassTitleTexture, "backglass"},
        {dmdRenderer, dmdTitleTexture, "dmd"},
        {topperRenderer, topperTitleTexture, "topper"}
    };

    for (auto& w : windows) {
        w.texture.reset(); // Always clear old title texture
        if (w.renderer && font && settings.showTitle && settings.titleWindow == w.windowName) {
            // Use the AssetManager's member titleRect for rendering.
            // The `titleRect` parameter is used to communicate the final rendered size back.
            this->titleRect.x = titleRect.x;
            this->titleRect.y = titleRect.y;
            this->titleRect.w = 0; // Reset width/height for renderText to calculate
            this->titleRect.h = 0;

            w.texture.reset(renderText(w.renderer, font, title, color, this->titleRect));
            int texWidth = 0, texHeight = 0;
            if (w.texture) {
                SDL_QueryTexture(w.texture.get(), nullptr, nullptr, &texWidth, &texHeight);
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

    // This is the primary function responsible for loading/reloading all table-specific assets
    // The previous cleanupVideoPlayers() call here is no longer needed as loadTableAssets handles it.
    loadTableAssets(index, tables);
    LOG_DEBUG("AssetManager: Completed asset reload for index " << index);
}

void AssetManager::clearVideoCache() {
    LOG_DEBUG("AssetManager: Clearing video player cache (including active ones)");
    // This method should clear the entire cache and active players,
    // potentially used for a full reset.
    // Ensure all players are stopped and moved to oldVideoPlayers_ or destroyed directly.

    // Move all cached players to oldVideoPlayers_ to be safely destructed later.
    for (auto& pair : videoPlayerCache_) {
        if (pair.second.player) {
            pair.second.player->stop();
            oldVideoPlayers_.push_back(std::move(pair.second.player));
        }
    }
    videoPlayerCache_.clear();

    // Reset current active players as well, adding them to oldVideoPlayers_
    if (playfieldVideoPlayer) { playfieldVideoPlayer->stop(); oldVideoPlayers_.push_back(std::move(playfieldVideoPlayer)); }
    if (backglassVideoPlayer) { backglassVideoPlayer->stop(); oldVideoPlayers_.push_back(std::move(backglassVideoPlayer)); }
    if (dmdVideoPlayer) { dmdVideoPlayer->stop(); oldVideoPlayers_.push_back(std::move(dmdVideoPlayer)); }
    if (topperVideoPlayer) { topperVideoPlayer->stop(); oldVideoPlayers_.push_back(std::move(topperVideoPlayer)); }

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
    // The 'current...' paths (w.videoPath, w.imagePath etc.) MUST be valid here,
    // as they hold the state from the previous load and are used to form cache keys.
    WindowAssetInfo windowsForCleanup[] = {
        // Note: For 'show' parameter, use the *current* settings, not necessarily 'true'
        // This ensures if a window was hidden previously, its player is still considered for caching if it existed.
        {playfieldRenderer, playfieldTexture, playfieldWheelTexture, playfieldTitleTexture, playfieldVideoPlayer,
         currentPlayfieldImagePath_, currentPlayfieldWheelImagePath_, currentPlayfieldVideoPath_,
         currentPlayfieldMediaWidth_, currentPlayfieldMediaHeight_, true, "playfield", "", ""},
        {backglassRenderer, backglassTexture, backglassWheelTexture, backglassTitleTexture, backglassVideoPlayer,
         currentBackglassImagePath_, currentBackglassWheelImagePath_, currentBackglassVideoPath_,
         currentBackglassMediaWidth_, currentBackglassMediaHeight_, lastShowBackglass, "backglass", "", ""}, // Use lastShow...
        {dmdRenderer, dmdTexture, dmdWheelTexture, dmdTitleTexture, dmdVideoPlayer,
         currentDmdImagePath_, currentDmdWheelImagePath_, currentDmdVideoPath_,
         currentDmdMediaWidth_, currentDmdMediaHeight_, lastShowDMD, "dmd", "", ""}, // Use lastShow...
        {topperRenderer, topperTexture, topperWheelTexture, topperTitleTexture, topperVideoPlayer,
         currentTopperImagePath_, currentTopperWheelImagePath_, currentTopperVideoPath_,
         currentTopperMediaWidth_, currentTopperMediaHeight_, lastShowTopper, "topper", "", ""} // Use lastShow...
    };

    for (auto& w : windowsForCleanup) {
        // --- Video Player Caching/Cleanup ---
        if (w.videoPlayer) {
            w.videoPlayer->stop(); // Always stop the video first

            // Only attempt to cache if it's a valid video that was playing
            if (!w.videoPath.empty() && w.mediaWidth > 0 && w.mediaHeight > 0 && w.renderer) {
                std::string cacheKey = w.videoPath + "_" + std::to_string(w.mediaWidth) + "x" + std::to_string(w.mediaHeight);

                // Attempt to insert. If successful, it's a new cache entry.
                auto [it, inserted] = videoPlayerCache_.emplace(cacheKey, VideoPlayerCacheEntry(w.renderer, w.mediaWidth, w.mediaHeight, std::move(w.videoPlayer)));
                if (inserted) {
                    LOG_DEBUG("AssetManager: Cached video player for " << w.name << ": " << w.videoPath << " (" << cacheKey << ")");
                } else {
                    // This means a player for the same path/dimensions is already in cache.
                    // This scenario is less common if unique_ptr is properly moved.
                    // Add the current player to the `oldVideoPlayers_` queue for eventual destruction.
                    addOldVideoPlayer(std::move(w.videoPlayer));
                    LOG_WARN("AssetManager: Duplicate video player key found for " << w.name << ": " << w.videoPath << ". Discarding current player.");
                }
            } else {
                // If the videoPath was empty or dimensions invalid, just add to old players queue
                // to ensure proper destruction.
                addOldVideoPlayer(std::move(w.videoPlayer));
                LOG_DEBUG("AssetManager: Discarded inactive/invalid video player for " << w.name << " (no valid path/dimensions).");
            }
            w.videoPlayer.reset(); // The unique_ptr has been moved, so reset the member
        }

        // --- Texture Cleanup ---
        // Textures (std::unique_ptr<SDL_Texture, ...>) will automatically call SDL_DestroyTexture
        // when `reset()` is called or when the unique_ptr goes out of scope (e.g., at end of function if not re-assigned).
        // No explicit caching of textures here; `loadTexture` is responsible for adding to `textureCache_`.
        if (w.texture) w.texture.reset();
        if (w.wheelTexture) w.wheelTexture.reset();
        if (w.titleTexture) w.titleTexture.reset();

        // --- Clear AssetManager's Member Paths/Dimensions for the NEW Table ---
        // These are the *actual member variables* of AssetManager, cleared for the new table data.
        w.imagePath.clear();
        w.wheelImagePath.clear();
        w.videoPath.clear(); // This was the crucial one that was cleared too early previously
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
    // These WindowAssetInfo structs now refer to the cleared AssetManager members
    // and use the new `table` data.
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
        // If the renderer is null or the window is configured not to show, skip loading assets for it
        if (!w.renderer || !w.show) {
            LOG_DEBUG("AssetManager: Skipping asset load for " << w.name << " (renderer missing or window not shown).");
            continue; // No need to load any assets for this window
        }

        // --- Texture Loading ---
        if (!w.imagePath.empty()) {
            w.texture.reset(loadTexture(w.renderer, w.imagePath)); // loadTexture adds to cache if new
            if (w.texture) {
                w.imagePath = w.imagePath; // Update the AssetManager's member path
            } else {
                w.imagePath.clear();
            }
        } else {
            w.imagePath.clear();
        }

        // --- Wheel Loading ---
        LOG_DEBUG("AssetManager: Loading wheel texture for " << w.name << ": " << table.wheelImage);
        if (settings.showWheel && settings.wheelWindow == w.name && !table.wheelImage.empty()) {
            w.wheelTexture.reset(loadTexture(w.renderer, table.wheelImage)); // loadTexture adds to cache if new
            if (w.wheelTexture) {
                w.wheelImagePath = table.wheelImage; // Update the AssetManager's member path
            } else {
                LOG_WARN("AssetManager: Failed to load wheel texture for " << w.name << ": " << table.wheelImage);
                w.wheelImagePath.clear();
            }
        } else {
            w.wheelTexture.reset(); // Ensure it's null if not loading
        }

        // --- Title Loading ---
        if (font && settings.showTitle && settings.titleWindow == w.name) {
            // Note: `this->titleRect` is the AssetManager's member, used to manage renderText's output dimensions
            SDL_Rect currentTitleRenderRect = {settings.titleX, settings.titleY, 0, 0}; // Use local for function call
            std::string title = table.title.empty() ? "Unknown Title" : table.title;
            w.titleTexture.reset(renderText(w.renderer, font, title, settings.fontColor, currentTitleRenderRect));
            // Update AssetManager's member titleRect with the dimensions calculated by renderText
            if (w.titleTexture) {
                this->titleRect.x = currentTitleRenderRect.x;
                this->titleRect.y = currentTitleRenderRect.y;
                this->titleRect.w = currentTitleRenderRect.w;
                this->titleRect.h = currentTitleRenderRect.h;
            }
        } else {
            w.titleTexture.reset(); // Ensure it's null if not loading
        }

        // --- Video Loading ---
        int mediaWidth = 0;
        int mediaHeight = 0;
        // Determine media dimensions based on window name and settings
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

        // Only try to load video if not forced to images only, path is valid, and dimensions are positive
        if (!settings.forceImagesOnly && !w.tableVideo.empty() && mediaWidth > 0 && mediaHeight > 0) {
            std::string cacheKey = w.tableVideo + "_" + std::to_string(mediaWidth) + "x" + std::to_string(mediaHeight);
            auto it = videoPlayerCache_.find(cacheKey);

            if (it != videoPlayerCache_.end() && it->second.renderer == w.renderer &&
                it->second.width == mediaWidth && it->second.height == mediaHeight) {
                // Found a matching player in cache, reuse it
                w.videoPlayer = std::move(it->second.player); // Transfer ownership from cache to AssetManager member
                videoPlayerCache_.erase(it); // Remove from cache as it's now active
                w.videoPlayer->play();
                // Update AssetManager's member paths/dimensions with the *new* table's info
                w.videoPath = w.tableVideo;
                w.mediaWidth = mediaWidth;
                w.mediaHeight = mediaHeight;
                LOG_DEBUG("AssetManager: Reused cached video player for " << w.name << ": " << w.tableVideo);
            } else {
                // Not found in cache or cache entry is invalid/mismatch, create a new one
                auto newPlayer = VideoPlayerFactory::createVideoPlayer(w.renderer, w.tableVideo, mediaWidth, mediaHeight, configManager_);
                if (newPlayer) {
                    w.videoPlayer = std::move(newPlayer); // Transfer ownership to AssetManager member
                    w.videoPlayer->play();
                    // Update AssetManager's member paths/dimensions
                    w.videoPath = w.tableVideo;
                    w.mediaWidth = mediaWidth;
                    w.mediaHeight = mediaHeight;
                    LOG_DEBUG("AssetManager: Created new video player for " << w.name << ": " << w.tableVideo);
                } else {
                    LOG_WARN("AssetManager: Failed to create video player for " << w.name << ": " << w.tableVideo);
                    w.videoPlayer.reset(); // Ensure no partial player is held
                    w.videoPath.clear();
                    w.mediaWidth = 0;
                    w.mediaHeight = 0;
                }
            }
        } else {
            // No video is desired for this window based on settings or invalid parameters
            // Ensure player is reset and paths cleared (already handled in Phase 1, but as safeguard)
            w.videoPlayer.reset();
            w.videoPath.clear();
            w.mediaWidth = 0;
            w.mediaHeight = 0;
            LOG_DEBUG("AssetManager: No video desired for " << w.name << " or configuration prevents video playback (forceImagesOnly or invalid params).");
        }
    }

    // Apply audio settings to all currently active video players (which are now the new ones)
    applyVideoAudioSettings();

    // Play table music (assuming this method is implemented and works)
    playTableMusic(index, tables);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO("Loaded " << table.title << " in " << duration << "ms");
}

void AssetManager::addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player) {
    if (player) {
        player->stop(); // Ensure it's stopped before adding to discard list
        oldVideoPlayers_.push_back(std::move(player));
        // Limit the size to avoid excessive memory use, remove the oldest if full
        if (oldVideoPlayers_.size() > 10) { // Example limit: keep last 10
            oldVideoPlayers_.erase(oldVideoPlayers_.begin());
            LOG_DEBUG("AssetManager: Removed oldest video player from queue (oldVideoPlayers_ size: " << oldVideoPlayers_.size() << ")");
        }
    }
}

void AssetManager::clearOldVideoPlayers() {
    // No manual deletion loop needed! unique_ptr handles it when the vector clears or elements are removed.
    oldVideoPlayers_.clear();
    //LOG_DEBUG("AssetManager: Cleared all old video players from queue");
}

SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    if (!renderer || path.empty()) {
        LOG_ERROR("AssetManager: Invalid renderer or empty path for texture: " << path);
        return nullptr;
    }

    auto it = textureCache_.find(path); // Using path as key
    if (it != textureCache_.end() && it->second.renderer == renderer) {
        // Found in cache, move it out and return the raw pointer.
        // The unique_ptr in cache entry will be empty after this move.
        // The caller (e.g., w.texture.reset()) will then take ownership.
        LOG_DEBUG("AssetManager: Reusing cached texture: " << path);
        std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> cachedTexture = std::move(it->second.texture);
        textureCache_.erase(it); // Remove the entry from cache after moving
        return cachedTexture.release(); // Return raw pointer, ownership now transferred to caller
    }

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
            // Move to oldVideoPlayers_ instead of direct reset for safer deletion management
            addOldVideoPlayer(std::move(p.player));
            p.player.reset(); // Clear the member unique_ptr
            p.videoPath.clear();
            p.mediaWidth = 0;
            p.mediaHeight = 0;
            LOG_DEBUG("AssetManager: Moved " << p.name << " video player to oldVideoPlayers_ for cleanup.");
        }
    }

    // Also clear the explicit video player cache, moving all to old players.
    // This part is crucial if cleanupVideoPlayers is called outside of `loadTableAssets`
    // (e.g., on application shutdown or major state change).
    for (auto& pair : videoPlayerCache_) {
        if (pair.second.player) {
            pair.second.player->stop();
            addOldVideoPlayer(std::move(pair.second.player));
        }
    }
    videoPlayerCache_.clear(); // Clear the map

    clearOldVideoPlayers(); // Ensure the old players queue is pruned
    LOG_DEBUG("AssetManager: All video players and cache entries processed for cleanup.");
}