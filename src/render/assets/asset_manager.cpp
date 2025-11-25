// ===== ./src/render/assets/asset_manager.cpp =====
#include "render/assets/asset_manager.h"
#include "render/assets/texture_cache.h"
#include "render/assets/video_player_cache.h"
#include "render/assets/title_renderer.h"
#include "render/video_players/video_player_factory.h"
#include "config/iconfig_service.h"
#include "log/logging.h"
#include <SDL_image.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

AssetManager::AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager)
    : playfieldRenderer(playfield), backglassRenderer(backglass), dmdRenderer(dmd), topperRenderer(topper)
    , soundManager_(soundManager), configManager_(nullptr)
    , textureCache_(std::make_unique<TextureCache>())
    , videoPlayerCache_(std::make_unique<VideoPlayerCache>())
    , titleRenderer_(std::make_unique<TitleRenderer>(nullptr))
{
    titleRenderer_->setFont(f);
    LOG_INFO("AssetManager constructed.");
}

void AssetManager::setSoundManager(ISoundManager* soundManager) { soundManager_ = soundManager; }

void AssetManager::playTableMusic(size_t index, const std::vector<TableData>& tables)
{
    if (!soundManager_ || index >= tables.size()) return;
    soundManager_->playTableMusic(tables[index].music);
}

void AssetManager::setSettingsManager(IConfigService* configService)
{
    configManager_ = configService;
    titleRenderer_ = std::make_unique<TitleRenderer>(configService);

    if (dmdRenderer && configManager_) {
#ifdef DEBUG_LOGGING
        dmdContentRenderer_.loadAssetsFromDirectory("assets/img/dmd_still", dmdRenderer);
#else
        dmdContentRenderer_.loadAssetsFromDirectory(configManager_->getSettings().dmdStillImages, dmdRenderer);
#endif
    }
}

void AssetManager::reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect)
{
    titleRenderer_->reloadTitleTexture(title, color, titleRect,
        playfieldRenderer, playfieldTitleTexture,
        backglassRenderer, backglassTitleTexture,
        dmdRenderer, dmdTitleTexture,
        topperRenderer, topperTitleTexture);
}

void AssetManager::reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index)
{
    if (index >= tables.size()) return;

    SDL_Renderer* old[] = {playfieldRenderer, backglassRenderer, dmdRenderer, topperRenderer};
    playfieldRenderer = windowManager->getPlayfieldRenderer();
    backglassRenderer = windowManager->getBackglassRenderer();
    dmdRenderer      = windowManager->getDMDRenderer();
    topperRenderer   = windowManager->getTopperRenderer();

    bool rendererChanged = false;
    SDL_Renderer* cur[] = {playfieldRenderer, backglassRenderer, dmdRenderer, topperRenderer};
    for (int i = 0; i < 4; ++i) if (old[i] && old[i] != cur[i]) rendererChanged = true;

    if (rendererChanged) {
        clearVideoCache();
        clearTextureCache();
    }

    titleRenderer_->setFont(font);
    loadTableAssets(index, tables);
}

// Fast path comparison – only relevant fields
static bool settingsEqualForAssets(const Settings& a, const Settings& b)
{
    return a.forceImagesOnly == b.forceImagesOnly &&
           a.useGenArt == b.useGenArt &&
           a.videoBackend == b.videoBackend &&
           a.showBackglass == b.showBackglass &&
           a.showDMD == b.showDMD &&
           a.showTopper == b.showTopper &&
           a.showTitle == b.showTitle &&
           a.showWheel == b.showWheel &&
           a.titleWindow == b.titleWindow &&
           a.wheelWindow == b.wheelWindow;
}

void AssetManager::loadTableAssets(size_t index, const std::vector<TableData>& tables)
{
    auto loadStart = std::chrono::high_resolution_clock::now();

    const auto& table = tables[index];
    const Settings& s = configManager_ ? configManager_->getSettings() : Settings();

    // FAST PATH: same table + same relevant settings → everything is cached
    if (lastIndex == index && settingsEqualForAssets(lastSettings, s)) {
        resumeVideos();  // Just in case paused

        auto loadEnd = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(loadEnd - loadStart).count();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << duration_ms;
        LOG_INFO("Loaded " + table.title + " in " + oss.str() + " ms (cached)");
        return;
    }

    // SLOW PATH: need to load or reload something
    stopAllVideos();

    if (lastIndex != index || !settingsEqualForAssets(lastSettings, s)) {
        cachePreviousPlayers();
        clearPreviousAssets();
    }

    lastIndex = index;
    lastSettings = s;

    // Actually load all assets
    loadWindowAssets(playfieldRenderer,  playfieldTexture,  playfieldWheelTexture,  playfieldTitleTexture,  playfieldVideoPlayer,  s.playfieldMediaWidth,  s.playfieldMediaHeight,  true,        "playfield", table.playfieldImage, table.playfieldVideo, table);
    loadWindowAssets(backglassRenderer, backglassTexture, backglassWheelTexture, backglassTitleTexture, backglassVideoPlayer, s.backglassMediaWidth, s.backglassMediaHeight, s.showBackglass, "backglass", table.backglassImage, table.backglassVideo, table);
    loadWindowAssets(dmdRenderer,       dmdTexture,       dmdWheelTexture,       dmdTitleTexture,       dmdVideoPlayer,       s.dmdMediaWidth,       s.dmdMediaHeight,       s.showDMD,      "dmd",       table.dmdImage,       table.dmdVideo,       table);
    loadWindowAssets(topperRenderer,    topperTexture,    topperWheelTexture,    topperTitleTexture,    topperVideoPlayer,    s.topperMediaWidth,    s.topperMediaHeight,    s.showTopper,   "topper",    table.topperImage,    table.topperVideo,    table);

    applyVideoAudioSettings();
    playTableMusic(index, tables);

    auto loadEnd = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(loadEnd - loadStart).count();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << duration_ms;
    LOG_INFO("Loaded " + table.title + " in " + oss.str() + " ms");
}


void AssetManager::loadWindowAssets(SDL_Renderer* renderer, SDL_Texture*& tex, SDL_Texture*& wheelTex, SDL_Texture*& /*titleTex*/,
                                    std::unique_ptr<IVideoPlayer>& player, int forcedW, int forcedH, bool show,
                                    const std::string& name, const std::string& imagePath, const std::string& videoPath,
                                    const TableData& table)
{
    if (!renderer || !show) return;

    int rw, rh;
    SDL_GetRendererOutputSize(renderer, &rw, &rh);
    int w = (forcedW > 0 && forcedH > 0) ? forcedW : rw;
    int h = (forcedH > 0 && forcedH > 0) ? forcedH : rh;

    const Settings& s = configManager_->getSettings();

    auto fileExists = [](const std::string& p) -> bool {
        return !p.empty() && std::filesystem::exists(p) && std::filesystem::is_regular_file(p);
    };

    // === TITLE ===
    if (s.showTitle && s.titleWindow == name) {
        SDL_Rect r{s.titleX, s.titleY, 0, 0};
        titleRenderer_->reloadTitleTexture(table.title, s.fontColor, r,
            playfieldRenderer, playfieldTitleTexture,
            backglassRenderer, backglassTitleTexture,
            dmdRenderer, dmdTitleTexture,
            topperRenderer, topperTitleTexture);
    }

    // === WHEEL - with path tracking ===
    if (s.showWheel && s.wheelWindow == name && !table.wheelImage.empty()) {
        std::string& currentWheelPath =
            name == "playfield" ? currentPlayfieldWheelImagePath_ :
            name == "backglass" ? currentBackglassWheelImagePath_ :
            name == "dmd" ? currentDmdWheelImagePath_ :
            currentTopperWheelImagePath_;

        if (currentWheelPath != table.wheelImage) {
            wheelTex = textureCache_->getTexture(renderer, table.wheelImage);
            if (wheelTex) currentWheelPath = table.wheelImage;
        }
    }

    // === VIDEO FIRST — PRIORITY OVER IMAGE ===
    if (!s.forceImagesOnly && fileExists(videoPath)) {
        std::string key = s.videoBackend + "_" + name + "_" + videoPath + "_" + std::to_string(w) + "x" + std::to_string(h);

        player = videoPlayerCache_->getVideoPlayer(key, renderer, w, h);
        if (!player) {
            auto newPlayer = VideoPlayerFactory::createVideoPlayer(renderer, videoPath, w, h, configManager_);
            if (newPlayer) {
                videoPlayerCache_->cacheVideoPlayer(key, std::move(newPlayer), renderer, w, h);
                player = videoPlayerCache_->getVideoPlayer(key, renderer, w, h);
            }
        }

        if (player) {
            player->play();

            // Store path + size
            if (name == "playfield") {
                currentPlayfieldVideoPath_ = videoPath;
                currentPlayfieldMediaWidth_ = w;
                currentPlayfieldMediaHeight_ = h;
            }
            else if (name == "backglass") {
                currentBackglassVideoPath_ = videoPath;
                currentBackglassMediaWidth_ = w;
                currentBackglassMediaHeight_ = h;
            }
            else if (name == "dmd") {
                currentDmdVideoPath_ = videoPath;
                currentDmdMediaWidth_ = w;
                currentDmdMediaHeight_ = h;
            }
            else if (name == "topper") {
                currentTopperVideoPath_ = videoPath;
                currentTopperMediaWidth_ = w;
                currentTopperMediaHeight_ = h;
            }
            return;
        }
    }

    // === ONLY FALLBACK TO IMAGE IF NO VIDEO ===
    if (fileExists(imagePath)) {
        std::string& currentImagePath =
            name == "playfield" ? currentPlayfieldImagePath_ :
            name == "backglass" ? currentBackglassImagePath_ :
            name == "dmd" ? currentDmdImagePath_ :
            currentTopperImagePath_;

        if (currentImagePath != imagePath) {
            tex = textureCache_->getTexture(renderer, imagePath);
            if (tex) currentImagePath = imagePath;
        }
        return;
    }

    // === FALLBACK ===
    loadFallbackMedia(renderer, player, name, table, w, h);
}
void AssetManager::loadFallbackMedia(SDL_Renderer* renderer, std::unique_ptr<IVideoPlayer>& player,
                                      const std::string& name, const TableData& table, int w, int h)
{
    const Settings& s = configManager_->getSettings();
    std::string text = (name == "dmd" ? table.manufacturer :
                       name == "topper" ? table.year :
                       name == "backglass" ? table.title : "ASAPCabinetFE");

    std::string pseudo = "__ALTERNATIVE_MEDIA__" + (name == "dmd" ? ":" + text : "");
    std::string key = s.videoBackend + "_" + name + "_" + pseudo + "_" + std::to_string(w) + "x" + std::to_string(h);

    player = videoPlayerCache_->getVideoPlayer(key, renderer, w, h);
    if(player) { player->play(); return; }

    if (s.useGenArt) {
        auto p = VideoPlayerFactory::createAlternativeMediaPlayer(renderer, w, h, s.fontPath, name, text,
            name == "dmd" ? &dmdContentRenderer_ : nullptr);
        if (p) {
            videoPlayerCache_->cacheVideoPlayer(key, std::move(p), renderer, w, h);
            player = videoPlayerCache_->getVideoPlayer(key, renderer, w, h);
            if (player) player->play();
            return;
        }
    }

    player = VideoPlayerFactory::createDefaultMediaPlayer(renderer, w, h, s.fontPath, name);
    if (player) player->play();
}

void AssetManager::stopAllVideos()
{
    if (playfieldVideoPlayer) playfieldVideoPlayer->stop();
    if (backglassVideoPlayer) backglassVideoPlayer->stop();
    if (dmdVideoPlayer) dmdVideoPlayer->stop();
    if (topperVideoPlayer) topperVideoPlayer->stop();
}

void AssetManager::resumeVideos()
{
    if (playfieldVideoPlayer && !playfieldVideoPlayer->isPlaying()) playfieldVideoPlayer->play();
    if (backglassVideoPlayer && !backglassVideoPlayer->isPlaying()) backglassVideoPlayer->play();
    if (dmdVideoPlayer && !dmdVideoPlayer->isPlaying()) dmdVideoPlayer->play();
    if (topperVideoPlayer && !topperVideoPlayer->isPlaying()) topperVideoPlayer->play();
}

void AssetManager::cachePreviousPlayers()
{
    auto cache = [this](std::unique_ptr<IVideoPlayer>& p, const std::string& path, int w, int h, const std::string& name) {
        if (!p) return;
        if (!path.empty() && w && h) {
            std::string key = configManager_->getSettings().videoBackend + "_" + name + "_" + path + "_" +
                              std::to_string(w) + "x" + std::to_string(h);
            videoPlayerCache_->cacheVideoPlayer(key, std::move(p),
                name == "playfield" ? playfieldRenderer :
                name == "backglass" ? backglassRenderer :
                name == "dmd" ? dmdRenderer : topperRenderer,
                w, h);
        } else {
            videoPlayerCache_->addOldVideoPlayer(std::move(p));
        }
    };

    cache(playfieldVideoPlayer, currentPlayfieldVideoPath_, currentPlayfieldMediaWidth_, currentPlayfieldMediaHeight_, "playfield");
    cache(backglassVideoPlayer, currentBackglassVideoPath_, currentBackglassMediaWidth_, currentBackglassMediaHeight_, "backglass");
    cache(dmdVideoPlayer, currentDmdVideoPath_, currentDmdMediaWidth_, currentDmdMediaHeight_, "dmd");
    cache(topperVideoPlayer, currentTopperVideoPath_, currentTopperMediaWidth_, currentTopperMediaHeight_, "topper");
}

void AssetManager::clearPreviousAssets()
{
    playfieldTexture = backglassTexture = dmdTexture = topperTexture = nullptr;
    playfieldWheelTexture = backglassWheelTexture = dmdWheelTexture = topperWheelTexture = nullptr;
    playfieldTitleTexture = backglassTitleTexture = dmdTitleTexture = topperTitleTexture = nullptr;

    currentPlayfieldVideoPath_.clear(); currentBackglassVideoPath_.clear();
    currentDmdVideoPath_.clear(); currentTopperVideoPath_.clear();
    currentPlayfieldImagePath_.clear(); currentBackglassImagePath_.clear();
    currentDmdImagePath_.clear(); currentTopperImagePath_.clear();
    currentPlayfieldWheelImagePath_.clear(); currentBackglassWheelImagePath_.clear();
    currentDmdWheelImagePath_.clear(); currentTopperWheelImagePath_.clear();

    currentPlayfieldMediaWidth_ = currentPlayfieldMediaHeight_ = 0;
    currentBackglassMediaWidth_ = currentBackglassMediaHeight_ = 0;
    currentDmdMediaWidth_ = currentDmdMediaHeight_ = 0;
    currentTopperMediaWidth_ = currentTopperMediaHeight_ = 0;
}

void AssetManager::clearVideoCache() { videoPlayerCache_->clearCache(); videoPlayerCache_->clearOldVideoPlayers(); }
void AssetManager::clearTextureCache() { textureCache_->clearCache(); }
void AssetManager::clearOldVideoPlayers() { videoPlayerCache_->clearOldVideoPlayers(); }

void AssetManager::applyVideoAudioSettings()
{
    if (!configManager_) return;
    const Settings& s = configManager_->getSettings();
    float vol = (s.mediaAudioVol / 100.0f) * (s.masterVol / 100.0f);
    bool mute = s.masterMute || s.mediaAudioMute;

    auto apply = [&](auto& p) { if (p) { p->setVolume(vol * 100.0f); p->setMute(mute); } };
    apply(playfieldVideoPlayer); apply(backglassVideoPlayer);
    apply(dmdVideoPlayer); apply(topperVideoPlayer);
}

void AssetManager::cleanupVideoPlayers()
{
    stopAllVideos();
    cachePreviousPlayers();
    videoPlayerCache_->clearCache();
    videoPlayerCache_->clearOldVideoPlayers();
}
