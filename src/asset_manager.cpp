#include "asset_manager.h"
#include <iostream>

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

void AssetManager::loadTableAssets(size_t index, const std::vector<Table>& tables) {
    const Table& table = tables[index];
    tableTexture.reset(loadTexture(primaryRenderer, table.tableImage));
    wheelTexture.reset(loadTexture(primaryRenderer, table.wheelImage));
    backglassTexture.reset(loadTexture(secondaryRenderer, table.backglassImage));
    dmdTexture.reset(loadTexture(secondaryRenderer, table.dmdImage));

    if (font) {
        SDL_Color white = {255, 255, 255, 255};
        tableNameTexture.reset(renderText(primaryRenderer, font, table.tableName, white, tableNameRect));
        tableNameRect.x = (MAIN_WINDOW_WIDTH - tableNameRect.w) / 2;
        tableNameRect.y = 10;
    }

    // Move current to old, stop playback
    auto stopAndMove = [](VideoContext*& current, VideoContext*& old) {
        if (current) {
            if (current->player) {
                libvlc_media_player_stop(current->player);
                while (libvlc_media_player_is_playing(current->player)) {
                    SDL_Delay(10);
                }
            }
            old = current;
            current = nullptr;
        }
    };
    stopAndMove(tableVideoPlayer, oldTableVideoPlayer);
    stopAndMove(backglassVideoPlayer, oldBackglassVideoPlayer);
    stopAndMove(dmdVideoPlayer, oldDmdVideoPlayer);

    // std::cout << "Loading table video: " << table.tableVideo << std::endl;
    // std::cout << "Loading backglass video: " << table.backglassVideo << std::endl;
    // std::cout << "Loading DMD video: " << table.dmdVideo << std::endl;

    tableVideoPlayer = table.tableVideo.empty() ? nullptr : setupVideoPlayer(primaryRenderer, table.tableVideo, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    backglassVideoPlayer = table.backglassVideo.empty() ? nullptr : setupVideoPlayer(secondaryRenderer, table.backglassVideo, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT);
    dmdVideoPlayer = table.dmdVideo.empty() ? nullptr : setupVideoPlayer(secondaryRenderer, table.dmdVideo, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT);

    if (tableVideoPlayer) {
        // std::cout << "Table video setup: player=" << tableVideoPlayer->player 
                //   << ", texture=" << tableVideoPlayer->texture 
                //   << ", pixels=" << tableVideoPlayer->pixels 
                //   << ", mutex=" << tableVideoPlayer->mutex << std::endl;
        if (libvlc_media_player_play(tableVideoPlayer->player) != 0) {
            std::cerr << "Failed to play table video: " << table.tableVideo << std::endl;
        }
    }
    if (backglassVideoPlayer) {
        // std::cout << "Backglass video setup: player=" << backglassVideoPlayer->player 
                //   << ", texture=" << backglassVideoPlayer->texture 
                //   << ", pixels=" << backglassVideoPlayer->pixels 
                //   << ", mutex=" << backglassVideoPlayer->mutex << std::endl;
        if (libvlc_media_player_play(backglassVideoPlayer->player) != 0) {
            std::cerr << "Failed to play backglass video: " << table.backglassVideo << std::endl;
        }
    }
    if (dmdVideoPlayer) {
        // std::cout << "DMD video setup: player=" << dmdVideoPlayer->player 
                //   << ", texture=" << dmdVideoPlayer->texture 
                //   << ", pixels=" << dmdVideoPlayer->pixels 
                //   << ", mutex=" << dmdVideoPlayer->mutex << std::endl;
        if (libvlc_media_player_play(dmdVideoPlayer->player) != 0) {
            std::cerr << "Failed to play DMD video: " << table.dmdVideo << std::endl;
        }
    }
}

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