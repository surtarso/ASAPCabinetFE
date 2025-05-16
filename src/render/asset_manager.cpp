#include "render/asset_manager.h"
#include "config/iconfig_service.h"
#include "utils/logging.h"
#include <SDL_image.h>
#include <chrono>

// Constructor: Initializes renderers, font, and nulls out pointers
AssetManager::AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, TTF_Font* f)
    : playfieldTexture(nullptr, SDL_DestroyTexture),
      wheelTexture(nullptr, SDL_DestroyTexture),
      backglassTexture(nullptr, SDL_DestroyTexture),
      dmdTexture(nullptr, SDL_DestroyTexture),
      titleTexture(nullptr, SDL_DestroyTexture),
      titleRect{0, 0, 0, 0},
      playfieldVideoPlayer(nullptr),
      backglassVideoPlayer(nullptr),
      dmdVideoPlayer(nullptr),
      playfieldRenderer(playfield),
      backglassRenderer(backglass),
      dmdRenderer(dmd),
      currentPlayfieldVideoPath_(),
      currentBackglassVideoPath_(),
      currentDmdVideoPath_(),
      font(f),
      configManager_(nullptr) {}

void AssetManager::setSettingsManager(IConfigService* configService) {
    configManager_ = configService;
    LOG_DEBUG("AssetManager: Settings manager set to " << configService);
}

void AssetManager::setTitlePosition(int x, int y) {
    if (titleTexture) {
        titleRect.x = x;
        titleRect.y = y;
        LOG_DEBUG("AssetManager: Updated title position to x=" << x << ", y=" << y);
    }
}

void AssetManager::setFont(TTF_Font* font) {
    this->font = font;
    LOG_DEBUG("AssetManager: Font set to " << font);
}

void AssetManager::reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) {
    if (playfieldRenderer && font) {
        // Reset titleTexture to force recreation
        titleTexture.reset();
        // Use input position, but reset dimensions
        this->titleRect.x = titleRect.x;
        this->titleRect.y = titleRect.y;
        this->titleRect.w = 0;
        this->titleRect.h = 0;
        titleTexture.reset(renderText(playfieldRenderer, font, title, color, this->titleRect));
        int texWidth = 0, texHeight = 0;
        if (titleTexture) {
            SDL_QueryTexture(titleTexture.get(), nullptr, nullptr, &texWidth, &texHeight);
            // Update caller's rect for consistency
            titleRect = this->titleRect;
        }
        LOG_DEBUG("AssetManager: Title texture reloaded, font=" << font << ", font_height=" 
                  << (font ? TTF_FontHeight(font) : 0) << ", width=" << texWidth << ", height=" << texHeight);
    } else {
        LOG_DEBUG("AssetManager: Skipping title texture reload: no renderer or font");
        titleTexture.reset();
        this->titleRect.w = 0;
        this->titleRect.h = 0;
        titleRect.w = 0;
        titleRect.h = 0;
    }
}

void AssetManager::reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableLoader>& tables, size_t index) {
    if (index >= tables.size()) {
        LOG_ERROR("AssetManager: Invalid table index " << index);
        return;
    }
    LOG_DEBUG("AssetManager: Reloading assets for table index " << index);
    
    // Update renderers and font
    playfieldRenderer = windowManager->getPlayfieldRenderer();
    backglassRenderer = windowManager->getBackglassRenderer();
    dmdRenderer = windowManager->getDMDRenderer();
    this->font = font;

    // Clean up existing video players
    cleanupVideoPlayers();

    // Reload table assets
    loadTableAssets(index, tables);

    LOG_DEBUG("AssetManager: Completed asset reload");
}

void AssetManager::clearVideoCache() {
    LOG_DEBUG("AssetManager: Clearing video path cache");
    currentPlayfieldVideoPath_.clear();
    currentBackglassVideoPath_.clear();
    currentDmdVideoPath_.clear();
}

void AssetManager::loadTableAssets(size_t index, const std::vector<TableLoader>& tables) {
    auto start = std::chrono::high_resolution_clock::now();
    LOG_DEBUG("AssetManager: loadTableAssets -> called with index: " << index);
    if (index >= tables.size()) {
        LOG_ERROR("AssetManager: Invalid table index: " << index << ", table count: " << tables.size());
        return;
    }
    const TableLoader& table = tables[index];
    const Settings& settings = configManager_ ? configManager_->getSettings() : Settings();
    static bool lastShowBackglass = settings.showBackglass;
    static bool lastShowDMD = settings.showDMD;
    static size_t lastIndex = -1;

    // Clear cache when table index changes
    if (index != lastIndex) {
        LOG_DEBUG("AssetManager: Table index changed from " << lastIndex << " to " << index << ", clearing cache");
        clearVideoCache();
        lastIndex = index;
    }

    // Log table data for debugging
    LOG_DEBUG("AssetManager: Loading table: " << table.title
              << ", playfieldVideo: " << table.playfieldVideo
              << ", backglassVideo: " << table.backglassVideo
              << ", dmdVideo: " << table.dmdVideo
              << ", playfieldImage: " << table.playfieldImage
              << ", backglassImage: " << table.backglassImage
              << ", dmdImage: " << table.dmdImage
              << ", wheelImage: " << table.wheelImage);

    // Load static textures only for valid renderers
    if (playfieldRenderer) {
        playfieldTexture.reset(loadTexture(playfieldRenderer, table.playfieldImage));
        wheelTexture.reset(loadTexture(playfieldRenderer, table.wheelImage));
        if (font) {
            titleRect = {settings.titleX, settings.titleY, 0, 0};
            titleTexture.reset(renderText(playfieldRenderer, font, table.title, settings.fontColor, titleRect));
        }
    } else {
        LOG_DEBUG("AssetManager: loadTableAssets -> Playfield renderer is null, skipping playfield textures");
        playfieldTexture.reset();
        wheelTexture.reset();
        titleTexture.reset();
    }

    if (backglassRenderer && settings.showBackglass) {
        backglassTexture.reset(loadTexture(backglassRenderer, table.backglassImage));
    } else {
        LOG_DEBUG("AssetManager: loadTableAssets -> Backglass renderer null or showBackglass false, skipping");
        backglassTexture.reset();
    }

    if (dmdRenderer && settings.showDMD) {
        dmdTexture.reset(loadTexture(dmdRenderer, table.dmdImage));
    } else {
        LOG_DEBUG("AssetManager: loadTableAssets -> DMD renderer null or showDMD false, skipping");
        dmdTexture.reset();
    }

    // Helper lambda to stop and queue old video players
    auto stopAndMove = [this](VideoContext*& current, std::string& currentPath) {
        if (current && current->player) {
            LOG_DEBUG("AssetManager: loadTableAssets -> Stopping and queuing old video player");
            libvlc_media_player_stop(current->player);
            // while (libvlc_media_player_is_playing(current->player)) {
            //     SDL_Delay(10);
            // }
            addOldVideoPlayer(current);
            current = nullptr;
            currentPath.clear();
        }
    };

    // Load playfield video
    if (playfieldRenderer && !table.playfieldVideo.empty() &&
        settings.playfieldMediaWidth > 0 && settings.playfieldMediaHeight > 0 &&
        table.playfieldVideo != currentPlayfieldVideoPath_) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Playfield video path changed, reloading");
        stopAndMove(playfieldVideoPlayer, currentPlayfieldVideoPath_);
        playfieldVideoPlayer = setupVideoPlayer(playfieldRenderer, table.playfieldVideo,
                                               settings.playfieldMediaWidth, settings.playfieldMediaHeight);
        if (playfieldVideoPlayer && libvlc_media_player_play(playfieldVideoPlayer->player) != 0) {
            LOG_ERROR("AssetManager: loadTableAssets -> Failed to play table video: " << table.playfieldVideo);
            cleanupVideoContext(playfieldVideoPlayer);
            playfieldVideoPlayer = nullptr;
        } else {
            currentPlayfieldVideoPath_ = table.playfieldVideo;
        }
    } else if (playfieldVideoPlayer && !table.playfieldVideo.empty() &&
               !libvlc_media_player_is_playing(playfieldVideoPlayer->player)) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Resuming playfield video");
        libvlc_media_player_play(playfieldVideoPlayer->player);
    } else if (table.playfieldVideo.empty()) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Playfield video empty, clearing");
        stopAndMove(playfieldVideoPlayer, currentPlayfieldVideoPath_);
    }

    // Load backglass video
    if (backglassRenderer && !table.backglassVideo.empty() &&
        settings.backglassMediaWidth > 0 && settings.backglassMediaHeight > 0 &&
        (table.backglassVideo != currentBackglassVideoPath_ || settings.showBackglass != lastShowBackglass)) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Backglass video path or showBackglass changed, reloading");
        stopAndMove(backglassVideoPlayer, currentBackglassVideoPath_);
        if (settings.showBackglass) {
            backglassVideoPlayer = setupVideoPlayer(backglassRenderer, table.backglassVideo,
                                                   settings.backglassMediaWidth, settings.backglassMediaHeight);
            if (backglassVideoPlayer && libvlc_media_player_play(backglassVideoPlayer->player) != 0) {
                LOG_ERROR("AssetManager: loadTableAssets -> Failed to play backglass video: " << table.backglassVideo);
                cleanupVideoContext(backglassVideoPlayer);
                backglassVideoPlayer = nullptr;
            } else {
                currentBackglassVideoPath_ = table.backglassVideo;
            }
        }
    } else if (backglassVideoPlayer && settings.showBackglass && !table.backglassVideo.empty() &&
               !libvlc_media_player_is_playing(backglassVideoPlayer->player)) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Resuming backglass video");
        libvlc_media_player_play(backglassVideoPlayer->player);
    } else if (backglassVideoPlayer && !settings.showBackglass) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Pausing backglass video due to showBackglass false");
        libvlc_media_player_pause(backglassVideoPlayer->player);
    } else if (table.backglassVideo.empty()) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Backglass video empty, clearing");
        stopAndMove(backglassVideoPlayer, currentBackglassVideoPath_);
    }

    // Load DMD video
    if (dmdRenderer && !table.dmdVideo.empty() &&
        settings.dmdMediaWidth > 0 && settings.dmdMediaHeight > 0 &&
        (table.dmdVideo != currentDmdVideoPath_ || settings.showDMD != lastShowDMD)) {
        LOG_DEBUG("AssetManager: loadTableAssets -> DMD video path or showDMD changed, reloading");
        stopAndMove(dmdVideoPlayer, currentDmdVideoPath_);
        if (settings.showDMD) {
            dmdVideoPlayer = setupVideoPlayer(dmdRenderer, table.dmdVideo,
                                             settings.dmdMediaWidth, settings.dmdMediaHeight);
            if (dmdVideoPlayer && libvlc_media_player_play(dmdVideoPlayer->player) != 0) {
                LOG_ERROR("AssetManager: loadTableAssets -> Failed to play DMD video: " << table.dmdVideo);
                cleanupVideoContext(dmdVideoPlayer);
                dmdVideoPlayer = nullptr;
            } else {
                currentDmdVideoPath_ = table.dmdVideo;
            }
        }
    } else if (dmdVideoPlayer && settings.showDMD && !table.dmdVideo.empty() &&
               !libvlc_media_player_is_playing(dmdVideoPlayer->player)) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Resuming DMD video");
        libvlc_media_player_play(dmdVideoPlayer->player);
    } else if (dmdVideoPlayer && !settings.showDMD) {
        LOG_DEBUG("AssetManager: loadTableAssets -> Pausing DMD video due to showDMD false");
        libvlc_media_player_pause(dmdVideoPlayer->player);
    } else if (table.dmdVideo.empty()) {
        LOG_DEBUG("AssetManager: loadTableAssets -> DMD video empty, clearing");
        stopAndMove(dmdVideoPlayer, currentDmdVideoPath_);
    }

    // Update last known settings
    lastShowBackglass = settings.showBackglass;
    lastShowDMD = settings.showDMD;

    // Clean up old video players immediately to free resources
    clearOldVideoPlayers();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO("AssetManager: Loaded assets for table: " << table.title << ", took " << duration << "ms");
}

void AssetManager::cleanupVideoPlayers() {
    LOG_INFO("AssetManager: Cleaning up video players");
    if (playfieldVideoPlayer && playfieldVideoPlayer->player) {
        libvlc_media_player_stop(playfieldVideoPlayer->player);
        cleanupVideoContext(playfieldVideoPlayer);
        playfieldVideoPlayer = nullptr;
        currentPlayfieldVideoPath_.clear();
    }
    if (backglassVideoPlayer && backglassVideoPlayer->player) {
        libvlc_media_player_stop(backglassVideoPlayer->player);
        cleanupVideoContext(backglassVideoPlayer);
        backglassVideoPlayer = nullptr;
        currentBackglassVideoPath_.clear();
    }
    if (dmdVideoPlayer && dmdVideoPlayer->player) {
        libvlc_media_player_stop(dmdVideoPlayer->player);
        cleanupVideoContext(dmdVideoPlayer);
        dmdVideoPlayer = nullptr;
        currentDmdVideoPath_.clear();
    }
    clearOldVideoPlayers();
}

void AssetManager::addOldVideoPlayer(VideoContext* player) {
    if (player) {
        oldVideoPlayers_.push_back(player);
        // Limit queue size to prevent resource buildup
        if (oldVideoPlayers_.size() > 10) {
            cleanupVideoContext(oldVideoPlayers_.front());
            oldVideoPlayers_.erase(oldVideoPlayers_.begin());
            LOG_DEBUG("AssetManager: Removed oldest video player from queue");
        }
    }
}

void AssetManager::clearOldVideoPlayers() {
    for (VideoContext* player : oldVideoPlayers_) {
        if (player) {
            cleanupVideoContext(player);
        }
    }
    oldVideoPlayers_.clear();
}

SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    if (!renderer || path.empty()) {
        LOG_ERROR("AssetManager: Invalid renderer or empty path for texture: " << path);
        return nullptr;
    }

    // Temporarily redirect stderr to suppress SDL_image logs
    std::FILE* originalStderr = stderr;
    std::FILE* nullFile = fopen("/dev/null", "w");
    if (!nullFile) {
        LOG_ERROR("AssetManager: Failed to open /dev/null for texture load: " << path);
        return nullptr;
    }
    stderr = nullFile;

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());

    // Restore stderr
    stderr = originalStderr;
    fclose(nullFile);

    if (!tex) {
        LOG_ERROR("AssetManager: Failed to load texture " << path << ": " << IMG_GetError());
    } else {
        LOG_DEBUG("AssetManager: Successfully loaded texture: " << path);
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
        LOG_DEBUG("AssetManager: Rendered text, width=" << surf->w << ", height=" << surf->h);
    }

    SDL_FreeSurface(surf);
    return texture;
}