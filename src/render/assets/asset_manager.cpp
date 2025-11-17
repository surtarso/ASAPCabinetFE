/**
 * @file asset_manager.cpp
 * @brief Implementation of the AssetManager class for managing and rendering assets in ASAPCabinetFE.
 */

#include "render/assets/asset_manager.h"
#include "render/assets/texture_cache.h"
#include "render/assets/video_player_cache.h"
#include "render/assets/title_renderer.h"
#include "render/video_players/video_player_factory.h"
#include "render/video_players/vlc/vlc_player.h"
#include "render/video_players/ffmpeg/ffmpeg_player.h"
#include "render/video_players/default_media_player.h"
#include "config/iconfig_service.h"
#include "log/logging.h"
#include <SDL_image.h>
#include <chrono>
#include <thread>
#include <algorithm>

AssetManager::AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager)
    : playfieldTexture(nullptr),
      playfieldWheelTexture(nullptr),
      playfieldTitleTexture(nullptr),
      backglassTexture(nullptr),
      backglassWheelTexture(nullptr),
      backglassTitleTexture(nullptr),
      dmdTexture(nullptr),
      dmdWheelTexture(nullptr),
      dmdTitleTexture(nullptr),
      topperTexture(nullptr),
      topperWheelTexture(nullptr),
      topperTitleTexture(nullptr),
      playfieldVideoPlayer(nullptr),
      backglassVideoPlayer(nullptr),
      dmdVideoPlayer(nullptr),
      topperVideoPlayer(nullptr),
      playfieldRenderer(playfield),
      backglassRenderer(backglass),
      dmdRenderer(dmd),
      topperRenderer(topper),
      soundManager_(soundManager),
      configManager_(nullptr),
      textureCache_(std::make_unique<TextureCache>()),
      videoPlayerCache_(std::make_unique<VideoPlayerCache>()),
      titleRenderer_(std::make_unique<TitleRenderer>(nullptr)) {
    titleRenderer_->setFont(f);
    LOG_INFO("AssetManager constructed.");
}

void AssetManager::setSoundManager(ISoundManager* soundManager) {
    soundManager_ = soundManager;
    LOG_DEBUG("Sound manager set to " + std::to_string(reinterpret_cast<uintptr_t>(soundManager)));
}

void AssetManager::playTableMusic(size_t index, const std::vector<TableData>& tables) {
    if (!soundManager_ || index >= tables.size()) {
        LOG_ERROR("Cannot play table music: invalid soundManager or index " + std::to_string(index));
        return;
    }
    const std::string& musicPath = tables[index].music;
    soundManager_->playTableMusic(musicPath);
    if (!musicPath.empty()) {
        LOG_DEBUG("Playing table music: " + musicPath);
    } else {
        LOG_DEBUG("No music path for table, stopping table music");
    }
}

void AssetManager::setSettingsManager(IConfigService* configService) {
    configManager_ = configService;
    titleRenderer_->setFont(nullptr);
    titleRenderer_->setTitlePosition(0, 0);
    titleRenderer_ = std::make_unique<TitleRenderer>(configService);
    //LOG_DEBUG("Settings manager set to " + std::to_string(reinterpret_cast<uintptr_t>(configService)));
}

void AssetManager::reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) {
    titleRenderer_->reloadTitleTexture(title, color, titleRect,
                                      playfieldRenderer, playfieldTitleTexture,
                                      backglassRenderer, backglassTitleTexture,
                                      dmdRenderer, dmdTitleTexture,
                                      topperRenderer, topperTitleTexture);
    LOG_DEBUG("Reloaded title texture for: " + title);
}

void AssetManager::reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) {
    if (index >= tables.size()) {
        LOG_ERROR("Invalid table index " + std::to_string(index));
        return;
    }
    LOG_DEBUG("Reloading assets for table index " + std::to_string(index));

        // Detect renderer pointer changes. If any renderer was recreated, clear caches
        SDL_Renderer* oldPlayfield = playfieldRenderer;
        SDL_Renderer* oldBackglass = backglassRenderer;
        SDL_Renderer* oldDmd = dmdRenderer;
        SDL_Renderer* oldTopper = topperRenderer;

        playfieldRenderer = windowManager->getPlayfieldRenderer();
        backglassRenderer = windowManager->getBackglassRenderer();
        dmdRenderer = windowManager->getDMDRenderer();
        topperRenderer = windowManager->getTopperRenderer();

        bool rendererChanged = (oldPlayfield && oldPlayfield != playfieldRenderer) ||
                               (oldBackglass && oldBackglass != backglassRenderer) ||
                               (oldDmd && oldDmd != dmdRenderer) ||
                               (oldTopper && oldTopper != topperRenderer);
        if (rendererChanged) {
            LOG_DEBUG("Renderer pointer changed - clearing texture & video caches to avoid invalid textures");
            // Stop and clear active video players and cached players referencing old renderers
            clearVideoCache();
            // Clear texture cache so textures created on the old renderers are not reused
            clearTextureCache();
        }
    titleRenderer_->setFont(font);

        LOG_DEBUG("AssetManager::reloadAssets - playfieldRenderer=" + std::to_string(reinterpret_cast<uintptr_t>(playfieldRenderer)) +
                  ", backglassRenderer=" + std::to_string(reinterpret_cast<uintptr_t>(backglassRenderer)) +
                  ", dmdRenderer=" + std::to_string(reinterpret_cast<uintptr_t>(dmdRenderer)) +
                  ", topperRenderer=" + std::to_string(reinterpret_cast<uintptr_t>(topperRenderer)));

    loadTableAssets(index, tables);
    LOG_DEBUG("Completed asset reload for index " + std::to_string(index));
}

void AssetManager::clearVideoCache() {
    LOG_DEBUG("Clearing video player cache");
    videoPlayerCache_->clearCache();

    if (playfieldVideoPlayer) {
        playfieldVideoPlayer->stop();
        videoPlayerCache_->addOldVideoPlayer(std::move(playfieldVideoPlayer));
    }
    if (backglassVideoPlayer) {
        backglassVideoPlayer->stop();
        videoPlayerCache_->addOldVideoPlayer(std::move(backglassVideoPlayer));
    }
    if (dmdVideoPlayer) {
        dmdVideoPlayer->stop();
        videoPlayerCache_->addOldVideoPlayer(std::move(dmdVideoPlayer));
    }
    if (topperVideoPlayer) {
        topperVideoPlayer->stop();
        videoPlayerCache_->addOldVideoPlayer(std::move(topperVideoPlayer));
    }

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

    videoPlayerCache_->clearOldVideoPlayers();
    LOG_INFO("Video player cache and active players cleared.");
}

void AssetManager::clearTextureCache() {
    textureCache_->clearCache();
    LOG_INFO("Texture cache cleared.");
}

void AssetManager::applyVideoAudioSettings() {
    if (!configManager_) {
        LOG_ERROR("Cannot apply video audio settings: configManager is null");
        return;
    }
    const Settings& settings = configManager_->getSettings();
    bool effective_mute = settings.masterMute || settings.mediaAudioMute;
    float effective_volume = (settings.mediaAudioVol / 100.0f) * (settings.masterVol / 100.0f);

    LOG_DEBUG("Applying video audio settings: mediaAudioVol=" + std::to_string(settings.mediaAudioVol) +
              ", mediaAudioMute=" + std::to_string(settings.mediaAudioMute) +
              ", masterVol=" + std::to_string(settings.masterVol) +
              ", masterMute=" + std::to_string(settings.masterMute) +
              ", effective volume=" + std::to_string(effective_volume * 100.0f) + "%, effective mute=" + std::to_string(effective_mute));

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
            LOG_DEBUG("Applied audio settings to " + std::string(p.name) + " video player: effective volume=" +
                      std::to_string(effective_volume * 100.0f) + ", effective mute=" + std::to_string(effective_mute));
        }
    }
}

void AssetManager::loadTableAssets(size_t index, const std::vector<TableData>& tables) {
    auto start = std::chrono::high_resolution_clock::now();

    // make sure we switch everything on renderer change
    static size_t lastIndex = static_cast<size_t>(-1);
    const std::string currentBackend = configManager_ ? configManager_->getSettings().videoBackend : std::string("vlc");
    static std::string lastBackend;
    if (lastBackend != currentBackend) {
        LOG_DEBUG("Video backend changed from " + lastBackend + " to " + currentBackend + ", forcing full reload");
        lastBackend = currentBackend;
        // Invalidate so we skip the 'already loaded' early return
        lastIndex = static_cast<size_t>(-1);
    }

    if (index >= tables.size()) {
        LOG_ERROR("Invalid table index: " + std::to_string(index) + ", table count: " + std::to_string(tables.size()));
        return;
    }

    const Settings& settings = configManager_ ? configManager_->getSettings() : Settings();
    static bool lastShowBackglass = settings.showBackglass;
    static bool lastShowDMD = settings.showDMD;
    static bool lastShowTopper = settings.showTopper;

    if (playfieldVideoPlayer) {
        playfieldVideoPlayer->stop();
        LOG_DEBUG("Stopped playfield video player");
    }
    if (backglassVideoPlayer) {
        backglassVideoPlayer->stop();
        LOG_DEBUG("Stopped backglass video player");
    }
    if (dmdVideoPlayer) {
        dmdVideoPlayer->stop();
        LOG_DEBUG("Stopped DMD video player");
    }
    if (topperVideoPlayer) {
        topperVideoPlayer->stop();
        LOG_DEBUG("Stopped topper video player");
    }

    if (index == lastIndex &&
        settings.showBackglass == lastShowBackglass &&
        settings.showDMD == lastShowDMD &&
        settings.showTopper == lastShowTopper) {

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
                LOG_DEBUG("Resumed playing active video for " + std::string(w.name) + ": " + w.videoPath);
            }
        }
        LOG_INFO("Table " + tables[index].title + " already loaded.");
        return;
    }

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
        if (w.videoPlayer) {
            w.videoPlayer->stop();

            if (!w.videoPath.empty() && w.mediaWidth > 0 && w.mediaHeight > 0 && w.renderer) {
                // Retrieve backend for consistent cache key generation
                std::string backend = configManager_
                    ? configManager_->getSettings().videoBackend
                    : "ffmpeg";

                // Use exactly the same key format as in loadTableAssets()
                std::string cacheKey =
                    backend + "_" +
                    w.name + "_" +          // playfield, backglass, etc
                    w.videoPath + "_" +
                    std::to_string(w.mediaWidth) + "x" +
                    std::to_string(w.mediaHeight);

                videoPlayerCache_->cacheVideoPlayer(
                    cacheKey,
                    std::move(w.videoPlayer),
                    w.renderer,
                    w.mediaWidth,
                    w.mediaHeight
                );
            }
            else {
                videoPlayerCache_->addOldVideoPlayer(std::move(w.videoPlayer));
            }

            w.videoPlayer.reset();
        }

        w.texture = nullptr;
        w.wheelTexture = nullptr;
        w.titleTexture = nullptr;
        w.imagePath.clear();
        w.wheelImagePath.clear();
        w.videoPath.clear();
        w.mediaWidth = 0;
        w.mediaHeight = 0;
    }

    lastIndex = index;
    lastShowBackglass = settings.showBackglass;
    lastShowDMD = settings.showDMD;
    lastShowTopper = settings.showTopper;

    const auto& table = tables[index];
    LOG_DEBUG("Loading assets for table: " + table.title);

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
            LOG_DEBUG("Skipping asset load for " + std::string(w.name) + " (renderer missing or window not shown).");
            continue;
        }

        if (!w.tableImage.empty()) {
            w.texture = textureCache_->getTexture(w.renderer, w.tableImage);
            if (w.texture) {
                w.imagePath = w.tableImage;
                LOG_DEBUG("Loaded texture for " + std::string(w.name) + ": " + w.tableImage);
            } else {
                w.imagePath.clear();
            }
        }

        if (settings.showWheel && settings.wheelWindow == w.name && !table.wheelImage.empty()) {
            w.wheelTexture = textureCache_->getTexture(w.renderer, table.wheelImage);
            if (w.wheelTexture) {
                w.wheelImagePath = table.wheelImage;
                LOG_DEBUG("Loaded wheel texture for " + std::string(w.name) + ": " + table.wheelImage);
            } else {
                LOG_WARN("Failed to load wheel texture for " + std::string(w.name) + ": " + table.wheelImage);
                w.wheelImagePath.clear();
            }
        }

        if (settings.showTitle && settings.titleWindow == w.name) {
            SDL_Rect currentTitleRenderRect = {settings.titleX, settings.titleY, 0, 0};
            std::string title = table.title.empty() ? "Unknown Title" : table.title;
            titleRenderer_->reloadTitleTexture(title, settings.fontColor, currentTitleRenderRect,
                                              playfieldRenderer, playfieldTitleTexture,
                                              backglassRenderer, backglassTitleTexture,
                                              dmdRenderer, dmdTitleTexture,
                                              topperRenderer, topperTitleTexture);
            LOG_DEBUG("Loaded title texture for " + std::string(w.name) + ": " + title);
        }

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

        // If the user didn't specify media size, fallback to window size
        if (mediaWidth <= 0 || mediaHeight <= 0) {
            int rw, rh;
            SDL_GetRendererOutputSize(w.renderer, &rw, &rh);
            mediaWidth = rw;
            mediaHeight = rh;
        }

        //--------------------------------------------------------------------------
        // REAL FILE EXISTENCE CHECK
        //--------------------------------------------------------------------------
        auto fileExists = [](const std::string& p) -> bool {
            if (p.empty()) return false;
            std::error_code ec;
            return std::filesystem::exists(p, ec) &&
                std::filesystem::is_regular_file(p, ec);
        };

        //--------------------------------------------------------------------------
        // PRIORITY FLAGS
        //--------------------------------------------------------------------------
        bool imagesOnly = settings.forceImagesOnly;

        // IMPORTANT: detect REAL usable files, not just non-empty strings
        bool hasUserVideo = fileExists(w.tableVideo);
        bool hasUserImage = fileExists(w.tableImage);

        //--------------------------------------------------------------------------
        // IMAGES ONLY = ON MODE
        //--------------------------------------------------------------------------
        // Priority: User Image → DefaultMedia

        if (imagesOnly) {
            if (hasUserImage) {
                // load user image
                w.texture = textureCache_->getTexture(w.renderer, w.tableImage);
                w.imagePath = w.tableImage;
                LOG_DEBUG("ImagesOnly=ON → Using USER IMAGE for " + std::string(w.name) + ": " + w.tableImage);
            } else {
                // load DefaultMedia fallback
                auto player = VideoPlayerFactory::createDefaultMediaPlayer(
                    w.renderer, mediaWidth, mediaHeight, configManager_->getSettings().fontPath, w.name);
                if (player) {
                    w.videoPlayer = std::move(player);
                    w.videoPlayer->play();
                    w.videoPath = "__DEFAULT_MEDIA__";
                    LOG_DEBUG("ImagesOnly=ON → Using DEFAULT MEDIA for " + std::string(w.name));
                }
            }
            continue;
        }

        //--------------------------------------------------------------------------
        // IMAGES ONLY = OFF MODE
        //--------------------------------------------------------------------------
        // Priority: User Video → User Image → DefaultMedia

        if (hasUserVideo && mediaWidth > 0 && mediaHeight > 0) {
            // Try load video from cache
            std::string backend = configManager_ ? configManager_->getSettings().videoBackend : "ffmpeg";
            std::string cacheKey = backend + "_" + w.name + "_" + w.tableVideo + "_" +
                                std::to_string(mediaWidth) + "x" + std::to_string(mediaHeight);

            w.videoPlayer = videoPlayerCache_->getVideoPlayer(cacheKey, w.renderer, mediaWidth, mediaHeight);
            if (w.videoPlayer) {
                w.videoPlayer->play();
                w.videoPath = w.tableVideo;
                w.mediaWidth = mediaWidth;
                w.mediaHeight = mediaHeight;
                LOG_DEBUG("ImagesOnly=OFF → Using CACHED VIDEO for " + std::string(w.name));
            } else {
                auto newPlayer = VideoPlayerFactory::createVideoPlayer(
                    w.renderer, w.tableVideo, mediaWidth, mediaHeight, configManager_);

                if (newPlayer) {
                    w.videoPlayer = std::move(newPlayer);
                    w.videoPlayer->play();
                    w.videoPath = w.tableVideo;
                    w.mediaWidth = mediaWidth;
                    w.mediaHeight = mediaHeight;
                    LOG_DEBUG("ImagesOnly=OFF → Loaded USER VIDEO for " + std::string(w.name));
                }
            }

            continue;
        }

        // No user video → Try user image
        if (hasUserImage) {
            w.texture = textureCache_->getTexture(w.renderer, w.tableImage);
            if (w.texture) {
                w.imagePath = w.tableImage;
                LOG_DEBUG("No USER VIDEO and ImagesOnly=OFF → Using USER IMAGE for " + std::string(w.name));
                continue;
            }
        }

        // No video and no image → DefaultMedia fallback
        {
            auto player = VideoPlayerFactory::createDefaultMediaPlayer(
                w.renderer, mediaWidth, mediaHeight, configManager_->getSettings().fontPath, w.name);
            if (player) {
                w.videoPlayer = std::move(player);
                w.videoPlayer->play();
                w.videoPath = "__DEFAULT_MEDIA__";
                LOG_DEBUG("No user media and ImagesOnly=OFF → Using DEFAULT MEDIA for " + std::string(w.name));
            }
        }
    }

    applyVideoAudioSettings();
    playTableMusic(index, tables);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO("Loaded " + table.title + " in " + std::to_string(duration) + "ms");
}

void AssetManager::clearOldVideoPlayers() {
    videoPlayerCache_->clearOldVideoPlayers();
    //LOG_DEBUG("Cleared old video players.");
}

void AssetManager::cleanupVideoPlayers() {
    //LOG_DEBUG("Cleaning up video players");
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
            videoPlayerCache_->addOldVideoPlayer(std::move(p.player));
            p.player.reset();
            p.videoPath.clear();
            p.mediaWidth = 0;
            p.mediaHeight = 0;
            //LOG_DEBUG("Moved " + std::string(p.name) + " video player to oldVideoPlayers_ for cleanup.");
        }
    }

    videoPlayerCache_->clearCache();
    videoPlayerCache_->clearOldVideoPlayers();
    LOG_DEBUG("All video players and cache entries processed for cleanup.");
}
