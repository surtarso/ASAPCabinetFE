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
    soundManager_->playTableMusic(musicPath);
    if (!musicPath.empty()) {
        LOG_DEBUG("AssetManager: Playing table music: " << musicPath);
    } else {
        LOG_DEBUG("AssetManager: No music path for table, stopping table music");
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
        w.texture.reset();
        if (w.renderer && font && settings.showTitle && settings.titleWindow == w.windowName) {
            this->titleRect.x = titleRect.x;
            this->titleRect.y = titleRect.y;
            this->titleRect.w = 0;
            this->titleRect.h = 0;
            w.texture.reset(renderText(w.renderer, font, title, color, this->titleRect));
            int texWidth = 0, texHeight = 0;
            if (w.texture) {
                SDL_QueryTexture(w.texture.get(), nullptr, nullptr, &texWidth, &texHeight);
                titleRect = this->titleRect;
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
    
    playfieldRenderer = windowManager->getPlayfieldRenderer();
    backglassRenderer = windowManager->getBackglassRenderer();
    dmdRenderer = windowManager->getDMDRenderer();
    topperRenderer = windowManager->getTopperRenderer();
    this->font = font;

    cleanupVideoPlayers();
    loadTableAssets(index, tables);
    LOG_DEBUG("AssetManager: Completed asset reload");
}

void AssetManager::clearVideoCache() {
    LOG_DEBUG("AssetManager: Clearing video path cache");
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
    // Only clear caches on settings change or memory pressure
    // textureCache_.clear();
    // videoPlayerCache_.clear();
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
            p.player->setVolume(effective_volume * 100.0f);
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
    const auto& table = tables[index];
    const Settings& settings = configManager_ ? configManager_->getSettings() : Settings();
    static bool lastShowBackglass = settings.showBackglass;
    static bool lastShowDMD = settings.showDMD;
    static bool lastShowTopper = settings.showTopper;
    static size_t lastIndex = -1;

    // Clear video paths for new table
    if (index != lastIndex) {
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
        lastIndex = index;
    }

    LOG_DEBUG("AssetManager: Loading table: " << table.title
              << ", playfieldVideo: " << table.playfieldVideo
              << ", backglassVideo: " << table.backglassVideo
              << ", dmdVideo: " << table.dmdVideo
              << ", topperVideo: " << table.topperVideo
              << ", playfieldImage: " << table.playfieldImage
              << ", backglassImage: " << table.backglassImage
              << ", dmdImage: " << table.dmdImage
              << ", topperImage: " << table.topperImage
              << ", wheelImage: " << table.wheelImage);

    WindowAssetInfo windows[] = {
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

    for (auto& w : windows) {
        if (!w.renderer || !w.show) {
            w.texture.reset();
            w.wheelTexture.reset();
            w.titleTexture.reset();
            if (w.videoPlayer) {
                w.videoPlayer->stop();
                if (!w.videoPath.empty()) {
                    std::string cacheKey = w.videoPath + "_" + std::to_string(w.mediaWidth) + "x" + std::to_string(w.mediaHeight);
                    if (videoPlayerCache_.find(cacheKey) == videoPlayerCache_.end()) {
                        videoPlayerCache_.emplace(cacheKey, VideoPlayerCacheEntry(w.renderer, w.mediaWidth, w.mediaHeight, std::move(w.videoPlayer)));
                        LOG_DEBUG("AssetManager: Cached video player for " << w.name << ": " << w.videoPath);
                    } else {
                        addOldVideoPlayer(std::move(w.videoPlayer));
                    }
                } else {
                    addOldVideoPlayer(std::move(w.videoPlayer));
                }
                w.videoPlayer.reset();
            }
            w.imagePath.clear();
            w.wheelImagePath.clear();
            w.videoPath.clear();
            w.mediaWidth = 0;
            w.mediaHeight = 0;
            continue;
        }

        // Cache existing video player before clearing paths
        if (w.videoPlayer && !w.videoPath.empty()) {
            w.videoPlayer->stop();
            std::string cacheKey = w.videoPath + "_" + std::to_string(w.mediaWidth) + "x" + std::to_string(w.mediaHeight);
            if (videoPlayerCache_.find(cacheKey) == videoPlayerCache_.end()) {
                videoPlayerCache_.emplace(cacheKey, VideoPlayerCacheEntry(w.renderer, w.mediaWidth, w.mediaHeight, std::move(w.videoPlayer)));
                LOG_DEBUG("AssetManager: Cached video player for " << w.name << ": " << w.videoPath);
            } else {
                addOldVideoPlayer(std::move(w.videoPlayer));
            }
            w.videoPlayer.reset();
        }

        // Reset paths after caching
        w.imagePath.clear();
        w.wheelImagePath.clear();
        w.videoPath.clear();

        // Texture
        LOG_DEBUG("AssetManager: Checking texture for " << w.name << ": tableImage=" << w.tableImage << ", current=" << w.imagePath);
        if (!w.tableImage.empty()) {
            auto it = textureCache_.find(w.tableImage);
            if (it != textureCache_.end() && it->second.renderer == w.renderer) {
                w.texture = std::move(it->second.texture);
                textureCache_.erase(it);
                w.imagePath = w.tableImage;
                LOG_DEBUG("AssetManager: Reused cached texture for " << w.name << ": " << w.tableImage);
            } else {
                w.texture.reset(loadTexture(w.renderer, w.tableImage));
                w.imagePath = w.tableImage;
                LOG_DEBUG("AssetManager: Loaded new texture for " << w.name << ": " << w.tableImage);
            }
        } else {
            w.texture.reset();
            w.imagePath.clear();
        }

        // Wheel
        LOG_DEBUG("AssetManager: Checking wheel for " << w.name << ": wheelImage=" << table.wheelImage << ", current=" << w.wheelImagePath);
        if (settings.showWheel && settings.wheelWindow == w.name && !table.wheelImage.empty()) {
            auto it = textureCache_.find(table.wheelImage);
            if (it != textureCache_.end() && it->second.renderer == w.renderer) {
                w.wheelTexture = std::move(it->second.texture);
                textureCache_.erase(it);
                w.wheelImagePath = table.wheelImage;
                LOG_DEBUG("AssetManager: Reused cached wheel texture for " << w.name << ": " << table.wheelImage);
            } else {
                w.wheelTexture.reset(loadTexture(w.renderer, table.wheelImage));
                w.wheelImagePath = table.wheelImage;
                LOG_DEBUG("AssetManager: Loaded new wheel texture for " << w.name << ": " << table.wheelImage);
            }
        } else {
            w.wheelTexture.reset();
            w.wheelImagePath.clear();
        }

        // Title
        if (font && settings.showTitle && settings.titleWindow == w.name) {
            titleRect = {settings.titleX, settings.titleY, 0, 0};
            std::string title = table.title.empty() ? "Unknown Title" : table.title;
            w.titleTexture.reset(renderText(w.renderer, font, title, settings.fontColor, titleRect));
        } else {
            w.titleTexture.reset();
        }

        // Video
        int mediaWidth = (w.name == std::string("playfield")) ? settings.playfieldMediaWidth :
                         (w.name == std::string("backglass")) ? settings.backglassMediaWidth :
                         (w.name == std::string("dmd")) ? settings.dmdMediaWidth :
                         settings.topperMediaWidth;
        int mediaHeight = (w.name == std::string("playfield")) ? settings.playfieldMediaHeight :
                          (w.name == std::string("backglass")) ? settings.backglassMediaHeight :
                          (w.name == std::string("dmd")) ? settings.dmdMediaHeight :
                          settings.topperMediaHeight;
        bool showChanged = (w.name == std::string("backglass") && settings.showBackglass != lastShowBackglass) ||
                           (w.name == std::string("dmd") && settings.showDMD != lastShowDMD) ||
                           (w.name == std::string("topper") && settings.showTopper != lastShowTopper);

        LOG_DEBUG("AssetManager: Checking video for " << w.name << ": tableVideo=" << w.tableVideo << ", current=" << w.videoPath
                  << ", media=" << mediaWidth << "x" << mediaHeight << ", showChanged=" << showChanged);
        if (!settings.forceImagesOnly && !w.tableVideo.empty() && mediaWidth > 0 && mediaHeight > 0) {
            std::string cacheKey = w.tableVideo + "_" + std::to_string(mediaWidth) + "x" + std::to_string(mediaHeight);
            auto it = videoPlayerCache_.find(cacheKey);
            if (it != videoPlayerCache_.end() && it->second.renderer == w.renderer &&
                it->second.width == mediaWidth && it->second.height == mediaHeight) {
                w.videoPlayer = std::move(it->second.player);
                videoPlayerCache_.erase(it);
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
                    LOG_DEBUG("AssetManager: Failed to setup " << w.name << " video: " << w.tableVideo);
                }
            }
        } else if (w.videoPlayer) {
            w.videoPlayer->stop();
            if (!w.videoPath.empty()) {
                std::string cacheKey = w.videoPath + "_" + std::to_string(w.mediaWidth) + "x" + std::to_string(w.mediaHeight);
                if (videoPlayerCache_.find(cacheKey) == videoPlayerCache_.end()) {
                    videoPlayerCache_.emplace(cacheKey, VideoPlayerCacheEntry(w.renderer, w.mediaWidth, w.mediaHeight, std::move(w.videoPlayer)));
                    LOG_DEBUG("AssetManager: Cached video player for " << w.name << ": " << w.videoPath);
                } else {
                    addOldVideoPlayer(std::move(w.videoPlayer));
                }
            } else {
                addOldVideoPlayer(std::move(w.videoPlayer));
            }
            w.videoPlayer.reset();
            w.videoPath.clear();
            w.mediaWidth = 0;
            w.mediaHeight = 0;
        }

        // Store video path for next iteration
        if (w.videoPlayer) {
            w.videoPath = w.tableVideo;
        }
    }

    // Apply audio settings to all video players
    applyVideoAudioSettings();

    lastShowBackglass = settings.showBackglass;
    lastShowDMD = settings.showDMD;
    lastShowTopper = settings.showTopper;

    // Play table music
    playTableMusic(index, tables);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO("Loaded " << table.title << " in " << duration << "ms");
}

void AssetManager::addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player) {
    if (player) {
        oldVideoPlayers_.push_back(std::move(player));
        if (oldVideoPlayers_.size() > 10) {
            oldVideoPlayers_.erase(oldVideoPlayers_.begin());
            LOG_DEBUG("AssetManager: Removed oldest video player from queue");
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

    auto it = textureCache_.find(path);
    if (it != textureCache_.end() && it->second.renderer == renderer) {
        auto texture = it->second.texture.release();
        textureCache_.erase(it);
        LOG_DEBUG("AssetManager: Reused cached texture: " << path);
        return texture;
    }

    std::FILE* originalStderr = stderr;
    std::FILE* nullFile = fopen("/dev/null", "w");
    if (!nullFile) {
        LOG_ERROR("AssetManager: Failed to open /dev/null for texture load: " << path);
        return nullptr;
    }
    stderr = nullFile;

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    stderr = originalStderr;
    fclose(nullFile);

    if (!tex) {
        LOG_ERROR("AssetManager: Failed to load texture " << path << ": " << IMG_GetError());
    } else if (textureCache_.size() < 100) {
        textureCache_.emplace(path, TextureCacheEntry(renderer, tex));
        LOG_DEBUG("AssetManager: Cached texture: " << path);
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
    LOG_DEBUG("AssetManager: Cleaning up video players");

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
            p.player.reset();
            p.videoPath.clear();
            p.mediaWidth = 0;
            p.mediaHeight = 0;
            LOG_DEBUG("AssetManager: Cleaned up " << p.name << " video player");
        }
    }

    clearOldVideoPlayers();
    LOG_DEBUG("AssetManager: Video players cleaned up");
}