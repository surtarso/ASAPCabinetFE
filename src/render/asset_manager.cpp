#include "render/asset_manager.h"
#include "config/iconfig_service.h"  // Changed to iconfig_service.h
#include "utils/logging.h"
#include <SDL_image.h>
#include <iostream>
#include <stdio.h>

AssetManager::AssetManager(SDL_Renderer* primary, SDL_Renderer* secondary, TTF_Font* f)
    : tableTexture(nullptr, SDL_DestroyTexture),
      wheelTexture(nullptr, SDL_DestroyTexture),
      backglassTexture(nullptr, SDL_DestroyTexture),
      dmdTexture(nullptr, SDL_DestroyTexture),
      tableNameTexture(nullptr, SDL_DestroyTexture),
      tableNameRect{0, 0, 0, 0},
      tableVideoPlayer(nullptr),
      backglassVideoPlayer(nullptr),
      dmdVideoPlayer(nullptr),
      primaryRenderer(primary),
      secondaryRenderer(secondary),
      font(f),
      configManager_(nullptr) {}  // Initialize configManager_

void AssetManager::loadTableAssets(size_t index, const std::vector<TableLoader>& tables) {
    const TableLoader& table = tables[index];
    const Settings& settings = configManager_->getSettings();

    tableTexture.reset(loadTexture(primaryRenderer, table.tableImage));
    wheelTexture.reset(loadTexture(primaryRenderer, table.wheelImage));
    backglassTexture.reset(loadTexture(secondaryRenderer, table.backglassImage));
    dmdTexture.reset(loadTexture(secondaryRenderer, table.dmdImage));

    if (font) {
        tableNameTexture.reset(renderText(primaryRenderer, font, table.tableName, settings.fontColor, tableNameRect));
    }

    auto stopAndMove = [](VideoContext*& current) {
        if (current && current->player) {
            libvlc_media_player_stop(current->player);
            while (libvlc_media_player_is_playing(current->player)) {
                SDL_Delay(10);
            }
        }
    };

    stopAndMove(tableVideoPlayer);
    stopAndMove(backglassVideoPlayer);
    stopAndMove(dmdVideoPlayer);

    LOG_DEBUG("Loading table video: " << table.tableVideo);
    LOG_DEBUG("Loading backglass video: " << table.backglassVideo);
    LOG_DEBUG("Loading DMD video: " << table.dmdVideo);

    if (!table.tableVideo.empty() && settings.mainWindowWidth > 0 && settings.mainWindowHeight > 0) {
        tableVideoPlayer = setupVideoPlayer(primaryRenderer, table.tableVideo, settings.mainWindowWidth, settings.mainWindowHeight);
        if (tableVideoPlayer && libvlc_media_player_play(tableVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play table video: " << table.tableVideo);
        }
    } else {
        tableVideoPlayer = nullptr;
    }
    if (!table.backglassVideo.empty() && settings.backglassMediaWidth > 0 && settings.backglassMediaHeight > 0) {
        backglassVideoPlayer = setupVideoPlayer(secondaryRenderer, table.backglassVideo, settings.backglassMediaWidth, settings.backglassMediaHeight);
        if (backglassVideoPlayer && libvlc_media_player_play(backglassVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play backglass video: " << table.backglassVideo);
        }
    } else {
        backglassVideoPlayer = nullptr;
    }
    if (!table.dmdVideo.empty() && settings.dmdMediaWidth > 0 && settings.dmdMediaHeight > 0) {
        dmdVideoPlayer = setupVideoPlayer(secondaryRenderer, table.dmdVideo, settings.dmdMediaWidth, settings.dmdMediaHeight);
        if (dmdVideoPlayer && libvlc_media_player_play(dmdVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play DMD video: " << table.dmdVideo);
        }
    } else {
        dmdVideoPlayer = nullptr;
    }
}

void AssetManager::addOldVideoPlayer(VideoContext* player) {
    if (player) {
        oldVideoPlayers_.push_back(player);
    }
}

void AssetManager::clearOldVideoPlayers() {
    for (VideoContext* player : oldVideoPlayers_) {
        if (player && player->player) {
            libvlc_media_player_stop(player->player);
            cleanupVideoContext(player);
        }
    }
    oldVideoPlayers_.clear();
}

SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    //LOG_DEBUG("Attempting to load texture: " << path);

    FILE* redirected;
#ifdef _WIN32
    redirected = freopen("nul", "w", stderr);
    if (!redirected) {
        std::cerr << "Warning: Failed to redirect stderr to nul" << std::endl;
    }
#else
    redirected = freopen("/dev/null", "w", stderr);
    if (!redirected) {
        LOG_DEBUG("Warning: Failed to redirect stderr to /dev/null");
    }
#endif

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());

    FILE* restored;
#ifdef _WIN32
    restored = freopen("CON", "w", stderr);
    if (!restored) {
        std::cerr << "Warning: Failed to restore stderr to CON" << std::endl;
    }
#else
    restored = freopen("/dev/tty", "w", stderr);
    if (!restored) {
        LOG_DEBUG("Warning: Failed to restore stderr to /dev/tty");
    }
#endif

    if (!tex) {
        LOG_DEBUG("Failed to load texture " << path << ": " << IMG_GetError());
    } else {
        LOG_DEBUG("Successfully loaded texture: " << path);
    }
    return tex;
}

SDL_Texture* AssetManager::renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        LOG_DEBUG("TTF_RenderUTF8_Blended error: " << TTF_GetError());
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture) {
        LOG_DEBUG("SDL_CreateTextureFromSurface error: " << SDL_GetError());
        SDL_FreeSurface(surf);
        return nullptr;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    textRect.w = surf->w;
    textRect.h = surf->h;
    SDL_FreeSurface(surf);
    return texture;
}