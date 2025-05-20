#include "render/asset_manager.h"
#include "render/table_loader.h"
#include "render/ivideo_player.h"
#include "render/video_player_factory.h"
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
      configManager_(nullptr),
      currentPlayfieldImagePath_(),
      currentWheelImagePath_(),
      currentBackglassImagePath_(),
      currentDmdImagePath_(),
      currentPlayfieldMediaWidth_(0),
      currentPlayfieldMediaHeight_(0),
      currentBackglassMediaWidth_(0),
      currentBackglassMediaHeight_(0),
      currentDmdMediaWidth_(0),
      currentDmdMediaHeight_(0) {}

void AssetManager::setSettingsManager(IConfigService* configService) {
    configManager_ = configService;
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
        titleTexture.reset();
        this->titleRect.x = titleRect.x;
        this->titleRect.y = titleRect.y;
        this->titleRect.w = 0;
        this->titleRect.h = 0;
        titleTexture.reset(renderText(playfieldRenderer, font, title, color, this->titleRect));
        int texWidth = 0, texHeight = 0;
        if (titleTexture) {
            SDL_QueryTexture(titleTexture.get(), nullptr, nullptr, &texWidth, &texHeight);
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

void AssetManager::reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) {
    if (index >= tables.size()) {
        LOG_ERROR("AssetManager: Invalid table index " << index);
        return;
    }
    LOG_DEBUG("AssetManager: Reloading assets for table index " << index);
    
    playfieldRenderer = windowManager->getPlayfieldRenderer();
    backglassRenderer = windowManager->getBackglassRenderer();
    dmdRenderer = windowManager->getDMDRenderer();
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
    currentPlayfieldMediaWidth_ = 0;
    currentPlayfieldMediaHeight_ = 0;
    currentBackglassMediaWidth_ = 0;
    currentBackglassMediaHeight_ = 0;
    currentDmdMediaWidth_ = 0;
    currentDmdMediaHeight_ = 0;
}

void AssetManager::loadTableAssets(size_t index, const std::vector<TableData>& tables) {
    auto start = std::chrono::high_resolution_clock::now();
    if (index >= tables.size()) {
        LOG_ERROR("AssetManager: Invalid table index: " << index << ", table count: " << tables.size());
        return;
    }
    const auto& table = tables[index];
    const Settings& settings = configManager_ ? configManager_->getSettings() : Settings();
    static bool lastShowBackglass = settings.showBackglass;
    static bool lastShowDMD = settings.showDMD;
    static size_t lastIndex = -1;

    if (index != lastIndex) {
        clearVideoCache();
        lastIndex = index;
    }

    LOG_DEBUG("AssetManager: Loading table: " << table.title
              << ", playfieldVideo: " << table.playfieldVideo
              << ", backglassVideo: " << table.backglassVideo
              << ", dmdVideo: " << table.dmdVideo
              << ", playfieldImage: " << table.playfieldImage
              << ", backglassImage: " << table.backglassImage
              << ", dmdImage: " << table.dmdImage
              << ", wheelImage: " << table.wheelImage);

    if (playfieldRenderer) {
        if (table.playfieldImage != currentPlayfieldImagePath_ || !playfieldTexture) {
            playfieldTexture.reset(loadTexture(playfieldRenderer, table.playfieldImage));
            currentPlayfieldImagePath_ = table.playfieldImage;
        }
        if (table.wheelImage != currentWheelImagePath_ || !wheelTexture) {
            wheelTexture.reset(loadTexture(playfieldRenderer, table.wheelImage));
            currentWheelImagePath_ = table.wheelImage;
        }
        if (font) {
            titleRect = {settings.titleX, settings.titleY, 0, 0};
            std::string title = table.title.empty() ? "Unknown Title" : table.title;
            titleTexture.reset(renderText(playfieldRenderer, font, title, settings.fontColor, titleRect));
        } else {
            titleTexture.reset();
        }
    } else {
        playfieldTexture.reset();
        wheelTexture.reset();
        titleTexture.reset();
        currentPlayfieldImagePath_.clear();
        currentWheelImagePath_.clear();
    }

    if (backglassRenderer && settings.showBackglass) {
        if (table.backglassImage != currentBackglassImagePath_ || !backglassTexture) {
            playfieldTexture.reset(loadTexture(playfieldRenderer, table.playfieldImage));
            currentPlayfieldImagePath_ = table.playfieldImage;
        }
        if (table.wheelImage != currentWheelImagePath_ || !wheelTexture) {
            wheelTexture.reset(loadTexture(playfieldRenderer, table.wheelImage));
            currentWheelImagePath_ = table.wheelImage;
        }
        if (font) {
            titleRect = {settings.titleX, settings.titleY, 0, 0};
            std::string title = table.title.empty() ? "Unknown Title" : table.title;
            titleTexture.reset(renderText(playfieldRenderer, font, title, settings.fontColor, titleRect));
        } else {
            titleTexture.reset();
        }
    } else {
        playfieldTexture.reset();
        wheelTexture.reset();
        titleTexture.reset();
        currentPlayfieldImagePath_.clear();
        currentWheelImagePath_.clear();
    }

    if (backglassRenderer && settings.showBackglass) {
        if (table.backglassImage != currentBackglassImagePath_ || !backglassTexture) {
            backglassTexture.reset(loadTexture(backglassRenderer, table.backglassImage));
            currentBackglassImagePath_ = table.backglassImage;
        }
    } else {
        backglassTexture.reset();
        currentBackglassImagePath_.clear();
    }

    if (dmdRenderer && settings.showDMD) {
        if (table.dmdImage != currentDmdImagePath_ || !dmdTexture) {
            dmdTexture.reset(loadTexture(dmdRenderer, table.dmdImage));
            currentDmdImagePath_ = table.dmdImage;
        }
    } else {
        dmdTexture.reset();
        currentDmdImagePath_.clear();
    }

    auto stopAndMove = [this](std::unique_ptr<IVideoPlayer>& current, std::string& currentPath) {
        if (current) {
            current->stop();
            addOldVideoPlayer(current.release());
            current = nullptr;
            currentPath.clear();
        }
    };

    if (playfieldRenderer && !table.playfieldVideo.empty() &&
        settings.playfieldMediaWidth > 0 && settings.playfieldMediaHeight > 0) {
        if (table.playfieldVideo != currentPlayfieldVideoPath_ ||
            settings.playfieldMediaWidth != currentPlayfieldMediaWidth_ ||
            settings.playfieldMediaHeight != currentPlayfieldMediaHeight_) {
            auto newPlayer = VideoPlayerFactory::createVideoPlayer(
                playfieldRenderer,
                table.playfieldVideo,
                settings.playfieldMediaWidth,
                settings.playfieldMediaHeight,
                configManager_);
            if (newPlayer) {
                stopAndMove(playfieldVideoPlayer, currentPlayfieldVideoPath_);
                playfieldVideoPlayer = std::move(newPlayer);
                playfieldVideoPlayer->play();
                currentPlayfieldVideoPath_ = table.playfieldVideo;
                currentPlayfieldMediaWidth_ = settings.playfieldMediaWidth;
                currentPlayfieldMediaHeight_ = settings.playfieldMediaHeight;
            } else {
                LOG_ERROR("AssetManager: Failed to setup playfield video: " << table.playfieldVideo);
            }
        } else if (playfieldVideoPlayer && !playfieldVideoPlayer->isPlaying()) {
            playfieldVideoPlayer->play();
        }
    } else if (playfieldVideoPlayer) {
        stopAndMove(playfieldVideoPlayer, currentPlayfieldVideoPath_);
        currentPlayfieldMediaWidth_ = 0;
        currentPlayfieldMediaHeight_ = 0;
    }

    if (backglassRenderer && !table.backglassVideo.empty() &&
        settings.backglassMediaWidth > 0 && settings.backglassMediaHeight > 0 &&
        settings.showBackglass) {
        if (table.backglassVideo != currentBackglassVideoPath_ ||
            settings.backglassMediaWidth != currentBackglassMediaWidth_ ||
            settings.backglassMediaHeight != currentBackglassMediaHeight_ ||
            settings.showBackglass != lastShowBackglass) {
            auto newPlayer = VideoPlayerFactory::createVideoPlayer(
                backglassRenderer,
                table.backglassVideo,
                settings.backglassMediaWidth,
                settings.backglassMediaHeight,
                configManager_);
            if (newPlayer) {
                stopAndMove(backglassVideoPlayer, currentBackglassVideoPath_);
                backglassVideoPlayer = std::move(newPlayer);
                backglassVideoPlayer->play();
                currentBackglassVideoPath_ = table.backglassVideo;
                currentBackglassMediaWidth_ = settings.backglassMediaWidth;
                currentBackglassMediaHeight_ = settings.backglassMediaHeight;
            } else {
                LOG_ERROR("AssetManager: Failed to setup backglass video: " << table.backglassVideo);
            }
        } else if (backglassVideoPlayer && !playfieldVideoPlayer->isPlaying()) {
            backglassVideoPlayer->play();
        }
    } else if (backglassVideoPlayer) {
        if (!settings.showBackglass) {
            backglassVideoPlayer->stop();
        } else {
            stopAndMove(backglassVideoPlayer, currentBackglassVideoPath_);
            currentBackglassMediaWidth_ = 0;
            currentBackglassMediaHeight_ = 0;
        }
    }

    if (dmdRenderer && !table.dmdVideo.empty() &&
        settings.dmdMediaWidth > 0 && settings.dmdMediaHeight > 0 &&
        settings.showDMD) {
        if (table.dmdVideo != currentDmdVideoPath_ ||
            settings.dmdMediaWidth != currentDmdMediaWidth_ ||
            settings.dmdMediaHeight != currentDmdMediaHeight_ ||
            settings.showDMD != lastShowDMD) {
            auto newPlayer = VideoPlayerFactory::createVideoPlayer(
                dmdRenderer,
                table.dmdVideo,
                settings.dmdMediaWidth,
                settings.dmdMediaHeight,
                configManager_);
            if (newPlayer) {
                stopAndMove(dmdVideoPlayer, currentDmdVideoPath_);
                dmdVideoPlayer = std::move(newPlayer);
                dmdVideoPlayer->play();
                currentDmdVideoPath_ = table.dmdVideo;
                currentDmdMediaWidth_ = settings.dmdMediaWidth;
                currentDmdMediaHeight_ = settings.dmdMediaHeight;
            } else {
                LOG_ERROR("AssetManager: Failed to setup DMD video: " << table.dmdVideo);
            }
        } else if (dmdVideoPlayer && !dmdVideoPlayer->isPlaying()) {
            dmdVideoPlayer->play();
        }
    } else if (dmdVideoPlayer) {
        if (!settings.showDMD) {
            dmdVideoPlayer->stop();
        } else {
            stopAndMove(dmdVideoPlayer, currentDmdVideoPath_);
            currentDmdMediaWidth_ = 0;
            currentDmdMediaHeight_ = 0;
        }
    }

    lastShowBackglass = settings.showBackglass;
    lastShowDMD = settings.showDMD;
    clearOldVideoPlayers();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO("AssetManager: Loaded assets for table: " << table.title << ", took " << duration << "ms");
}

void AssetManager::cleanupVideoPlayers() {
    LOG_INFO("AssetManager: Cleaning up video players");
    if (playfieldVideoPlayer) {
        playfieldVideoPlayer->stop();
        playfieldVideoPlayer.reset();
        currentPlayfieldVideoPath_.clear();
        currentPlayfieldMediaWidth_ = 0;
        currentPlayfieldMediaHeight_ = 0;
    }
    if (backglassVideoPlayer) {
        backglassVideoPlayer->stop();
        backglassVideoPlayer.reset();
        currentBackglassVideoPath_.clear();
        currentBackglassMediaWidth_ = 0;
        currentBackglassMediaHeight_ = 0;
    }
    if (dmdVideoPlayer) {
        dmdVideoPlayer->stop();
        dmdVideoPlayer.reset();
        currentDmdVideoPath_.clear();
        currentDmdMediaWidth_ = 0;
        currentDmdMediaHeight_ = 0;
    }
    clearOldVideoPlayers();
}

void AssetManager::addOldVideoPlayer(IVideoPlayer* player) {
    if (player) {
        oldVideoPlayers_.push_back(player);
        if (oldVideoPlayers_.size() > 10) {
            delete oldVideoPlayers_.front();
            oldVideoPlayers_.erase(oldVideoPlayers_.begin());
            LOG_DEBUG("AssetManager: Removed oldest video player from queue");
        }
    }
}

void AssetManager::clearOldVideoPlayers() {
    for (IVideoPlayer* player : oldVideoPlayers_) {
        delete player;
    }
    oldVideoPlayers_.clear();
}

SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    if (!renderer || path.empty()) {
        LOG_ERROR("AssetManager: Invalid renderer or empty path for texture: " << path);
        return nullptr;
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