#include "render/asset_manager.h"
#include "config/iconfig_service.h"
#include "utils/logging.h"
#include <SDL_image.h>
#include <iostream>
#include <stdio.h>

// Constructor: Initializes renderers, font, and nulls out pointers
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
      playfieldRenderer(primary),
      backglassRenderer(secondary),
      font(f),
      configManager_(nullptr) {}

// Loads all assets for a table at the given index
void AssetManager::loadTableAssets(size_t index, const std::vector<TableLoader>& tables) {
    const TableLoader& table = tables[index];
    const Settings& settings = configManager_->getSettings();

    // Load static textures
    tableTexture.reset(loadTexture(playfieldRenderer, table.tableImage));
    wheelTexture.reset(loadTexture(playfieldRenderer, table.wheelImage));
    backglassTexture.reset(loadTexture(backglassRenderer, table.backglassImage));
    dmdTexture.reset(loadTexture(backglassRenderer, table.dmdImage));

    // Render table name text if font is available
    if (font) {
        tableNameTexture.reset(renderText(playfieldRenderer, font, table.tableName, settings.fontColor, tableNameRect));
    }

    // Helper lambda to stop and queue old video players
    auto stopAndMove = [this](VideoContext*& current) {
        if (current && current->player) {
            libvlc_media_player_stop(current->player);
            while (libvlc_media_player_is_playing(current->player)) {
                SDL_Delay(10);  // Wait for stop to complete
            }
            addOldVideoPlayer(current);  // Queue for cleanup
            current = nullptr;           // Clear current pointer
        }
    };

    // Stop and move existing video players
    stopAndMove(tableVideoPlayer);
    stopAndMove(backglassVideoPlayer);
    stopAndMove(dmdVideoPlayer);

    // Load new video players if paths and dimensions are valid
    LOG_DEBUG("Loading table video: " << table.tableVideo);
    if (!table.tableVideo.empty() && settings.playfieldWindowWidth > 0 && settings.playfieldWindowHeight > 0) {
        tableVideoPlayer = setupVideoPlayer(playfieldRenderer, table.tableVideo, settings.playfieldWindowWidth, settings.playfieldWindowHeight);
        if (tableVideoPlayer && libvlc_media_player_play(tableVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play table video: " << table.tableVideo);
        }
    } else {
        tableVideoPlayer = nullptr;
    }

    LOG_DEBUG("Loading backglass video: " << table.backglassVideo);
    if (!table.backglassVideo.empty() && settings.backglassMediaWidth > 0 && settings.backglassMediaHeight > 0) {
        backglassVideoPlayer = setupVideoPlayer(backglassRenderer, table.backglassVideo, settings.backglassMediaWidth, settings.backglassMediaHeight);
        if (backglassVideoPlayer && libvlc_media_player_play(backglassVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play backglass video: " << table.backglassVideo);
        }
    } else {
        backglassVideoPlayer = nullptr;
    }

    LOG_DEBUG("Loading DMD video: " << table.dmdVideo);
    if (!table.dmdVideo.empty() && settings.dmdMediaWidth > 0 && settings.dmdMediaHeight > 0) {
        dmdVideoPlayer = setupVideoPlayer(backglassRenderer, table.dmdVideo, settings.dmdMediaWidth, settings.dmdMediaHeight);
        if (dmdVideoPlayer && libvlc_media_player_play(dmdVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play DMD video: " << table.dmdVideo);
        }
    } else {
        dmdVideoPlayer = nullptr;
    }
}

// Cleanup all active video players (moved from App::cleanup)
void AssetManager::cleanupVideoPlayers() {
    LOG_DEBUG("Cleaning up video players in AssetManager");
    
    // Clean up table video player
    if (tableVideoPlayer && tableVideoPlayer->player) {
        libvlc_media_player_stop(tableVideoPlayer->player);
        cleanupVideoContext(tableVideoPlayer);
        tableVideoPlayer = nullptr;
    }
    
    // Clean up backglass video player
    if (backglassVideoPlayer && backglassVideoPlayer->player) {
        libvlc_media_player_stop(backglassVideoPlayer->player);
        cleanupVideoContext(backglassVideoPlayer);
        backglassVideoPlayer = nullptr;
    }
    
    // Clean up DMD video player
    if (dmdVideoPlayer && dmdVideoPlayer->player) {
        libvlc_media_player_stop(dmdVideoPlayer->player);
        cleanupVideoContext(dmdVideoPlayer);
        dmdVideoPlayer = nullptr;
    }
    
    // Clear any queued old players
    clearOldVideoPlayers();
}

// Queue an old video player for cleanup
void AssetManager::addOldVideoPlayer(VideoContext* player) {
    if (player) {
        oldVideoPlayers_.push_back(player);
    }
}

// Clean up old video players in the queue
void AssetManager::clearOldVideoPlayers() {
    for (VideoContext* player : oldVideoPlayers_) {
        if (player && player->player) {
            libvlc_media_player_stop(player->player);
            cleanupVideoContext(player);
        }
    }
    oldVideoPlayers_.clear();
}

// Load a texture from a file, redirecting stderr to suppress SDL noise
SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    FILE* redirected;
#ifdef _WIN32
    redirected = freopen("nul", "w", stderr);
    if (!redirected) {
        LOG_DEBUG("Failed to redirect stderr to nul for texture load: " << path);
    }
#else
    redirected = freopen("/dev/null", "w", stderr);
    if (!redirected) {
        LOG_ERROR("Failed to redirect stderr to /dev/null for texture load: " << path);
    }
#endif
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    FILE* restored;
#ifdef _WIN32
    restored = freopen("CON", "w", stderr);
    if (!restored) {
        LOG_DEBUG("Failed to restore stderr from nul after loading texture: " << path);
    }
#else
    restored = freopen("/dev/tty", "w", stderr);
    if (!restored) {
        LOG_ERROR("Failed to restore stderr from /dev/null after loading texture: " << path);
    }
#endif
    if (!tex) {
        LOG_ERROR("Failed to load texture " << path << ": " << IMG_GetError());
    } else {
        LOG_DEBUG("Successfully loaded texture: " << path);
    }
    return tex;
}

// Render text to a texture, updating rect with size
SDL_Texture* AssetManager::renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        LOG_ERROR("TTF_RenderUTF8_Blended error: " << TTF_GetError());
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture) {
        LOG_ERROR("SDL_CreateTextureFromSurface error: " << SDL_GetError());
    } else {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textRect.w = surf->w;
        textRect.h = surf->h;
    }
    SDL_FreeSurface(surf);
    return texture;
}