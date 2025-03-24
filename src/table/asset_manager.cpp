#include "table/asset_manager.h"
#include "config/config_loader.h" // For FONT_COLOR
#include "utils/logging.h"
#include <SDL_image.h> // For IMG_LoadTexture
#include <iostream>
#include <stdio.h> // For stderr redirection

// Constructor: Initializes AssetManager with renderers and font, sets up smart pointers for textures.
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
      oldTableVideoPlayer(nullptr),
      oldBackglassVideoPlayer(nullptr),
      oldDmdVideoPlayer(nullptr),
      primaryRenderer(primary),
      secondaryRenderer(secondary),
      font(f) {}

// Loads assets (textures, text, videos) for a table at the given index.
void AssetManager::loadTableAssets(size_t index, const std::vector<Table>& tables) {
    const Table& table = tables[index];
    
    // Load textures for table, wheel, backglass, and DMD using member functions.
    tableTexture.reset(loadTexture(primaryRenderer, table.tableImage));
    wheelTexture.reset(loadTexture(primaryRenderer, table.wheelImage));
    backglassTexture.reset(loadTexture(secondaryRenderer, table.backglassImage));
    dmdTexture.reset(loadTexture(secondaryRenderer, table.dmdImage));

    // Render table name as a texture if font is available
    if (font) {
        tableNameTexture.reset(renderText(primaryRenderer, font, table.tableName, FONT_COLOR, tableNameRect));
        tableNameRect.x = (MAIN_WINDOW_WIDTH - tableNameRect.w) / 2;
        tableNameRect.y = 10;
    }

    // Lambda to stop and move current video player to old, preparing for new video.
    auto stopAndMove = [](VideoContext*& current, VideoContext*& old) {
        if (current) {
            if (current->player) {
                libvlc_media_player_stop(current->player);
                while (libvlc_media_player_is_playing(current->player)) {
                    SDL_Delay(10); // Wait for playback to fully stop.
                }
            }
            old = current;
            current = nullptr;
        }
    };

    // Stop and move existing video players to old pointers.
    stopAndMove(tableVideoPlayer, oldTableVideoPlayer);
    stopAndMove(backglassVideoPlayer, oldBackglassVideoPlayer);
    stopAndMove(dmdVideoPlayer, oldDmdVideoPlayer);

    LOG_DEBUG("Loading table video: " << table.tableVideo);
    LOG_DEBUG("Loading backglass video: " << table.backglassVideo);
    LOG_DEBUG("Loading DMD video: " << table.dmdVideo);

    // Set up new video players for table, backglass, and DMD if videos exist.
    tableVideoPlayer = table.tableVideo.empty() ? nullptr : setupVideoPlayer(primaryRenderer, table.tableVideo, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    backglassVideoPlayer = table.backglassVideo.empty() ? nullptr : setupVideoPlayer(secondaryRenderer, table.backglassVideo, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT);
    dmdVideoPlayer = table.dmdVideo.empty() ? nullptr : setupVideoPlayer(secondaryRenderer, table.dmdVideo, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT);

    if (tableVideoPlayer) {
        LOG_DEBUG("Table video setup: player=" << tableVideoPlayer->player 
                  << ", texture=" << tableVideoPlayer->texture 
                  << ", pixels=" << tableVideoPlayer->pixels 
                  << ", mutex=" << tableVideoPlayer->mutex); 
        if (libvlc_media_player_play(tableVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play table video: " << table.tableVideo);
        }
    }
    if (backglassVideoPlayer) {
        LOG_DEBUG("Backglass video setup: player=" << backglassVideoPlayer->player 
                  << ", texture=" << backglassVideoPlayer->texture 
                  << ", pixels=" << backglassVideoPlayer->pixels 
                  << ", mutex=" << backglassVideoPlayer->mutex); 
        if (libvlc_media_player_play(backglassVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play backglass video: " << table.backglassVideo); 
        }
    }
    if (dmdVideoPlayer) {
        LOG_DEBUG("DMD video setup: player=" << dmdVideoPlayer->player 
                  << ", texture=" << dmdVideoPlayer->texture 
                  << ", pixels=" << dmdVideoPlayer->pixels 
                  << ", mutex=" << tableVideoPlayer->mutex); 
        if (libvlc_media_player_play(dmdVideoPlayer->player) != 0) {
            LOG_DEBUG("Failed to play DMD video: " << table.dmdVideo); 
        }
    }
}

// Cleans up old video players to free resources.
void AssetManager::clearOldVideoPlayers() {
    if (oldTableVideoPlayer) {
        cleanupVideoContext(oldTableVideoPlayer);
        oldTableVideoPlayer = nullptr;
    }
    if (oldBackglassVideoPlayer) {
        cleanupVideoContext(oldBackglassVideoPlayer);
        oldBackglassVideoPlayer = nullptr;
    }
    if (oldDmdVideoPlayer) {
        cleanupVideoContext(oldDmdVideoPlayer);
        oldDmdVideoPlayer = nullptr;
    }
}

// Load a texture from a file path
SDL_Texture* AssetManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    // Log the path we’re trying to load
    LOG_DEBUG("Attempting to load texture: " << path);

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

    // Restore stderr
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
    } 
    else {
        LOG_DEBUG("Successfully loaded texture: " << path);
    }
    return tex;
}

// Render text to a texture
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
    // Ensure transparency is respected by enabling alpha blending
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    textRect.w = surf->w;
    textRect.h = surf->h;
    SDL_FreeSurface(surf);
    return texture;
}