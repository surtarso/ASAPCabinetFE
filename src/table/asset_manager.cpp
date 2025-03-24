#include "table/asset_manager.h"
#include "logging.h"
#include <iostream>

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
    
    // Load textures for table, wheel, backglass, and DMD.
    tableTexture.reset(loadTexture(primaryRenderer, table.tableImage));
    wheelTexture.reset(loadTexture(primaryRenderer, table.wheelImage));
    backglassTexture.reset(loadTexture(secondaryRenderer, table.backglassImage));
    dmdTexture.reset(loadTexture(secondaryRenderer, table.dmdImage));

    // Render table name as a texture if font is available
    if (font) {
        SDL_Color white = {255, 255, 255, 255};
        tableNameTexture.reset(renderText(primaryRenderer, font, table.tableName, white, tableNameRect));
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
                  << ", mutex=" << dmdVideoPlayer->mutex); 
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