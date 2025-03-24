#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <vlc/vlc.h>
#include <memory>
#include <vector>
#include "table/table_manager.h"
#include "render/video_player.h"

class AssetManager {
public:
    AssetManager(SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer, TTF_Font* font);
    void loadTableAssets(size_t index, const std::vector<Table>& tables);
    SDL_Texture* getTableTexture() const { return tableTexture.get(); }
    SDL_Texture* getWheelTexture() const { return wheelTexture.get(); }
    SDL_Texture* getBackglassTexture() const { return backglassTexture.get(); }
    SDL_Texture* getDmdTexture() const { return dmdTexture.get(); }
    SDL_Texture* getTableNameTexture() const { return tableNameTexture.get(); }
    SDL_Rect getTableNameRect() const { return tableNameRect; }
    VideoContext* getTableVideoPlayer() const { return tableVideoPlayer; }
    VideoContext* getBackglassVideoPlayer() const { return backglassVideoPlayer; }
    VideoContext* getDmdVideoPlayer() const { return dmdVideoPlayer; }
    VideoContext* getOldTableVideoPlayer() const { return oldTableVideoPlayer; }
    VideoContext* getOldBackglassVideoPlayer() const { return oldBackglassVideoPlayer; }
    VideoContext* getOldDmdVideoPlayer() const { return oldDmdVideoPlayer; }
    void clearOldVideoPlayers(); // New method to clean up after transition

private:
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> tableTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> wheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> tableNameTexture;
    SDL_Rect tableNameRect;
    VideoContext* tableVideoPlayer;
    VideoContext* backglassVideoPlayer;
    VideoContext* dmdVideoPlayer;
    VideoContext* oldTableVideoPlayer;    // Hold old contexts
    VideoContext* oldBackglassVideoPlayer;
    VideoContext* oldDmdVideoPlayer;
    SDL_Renderer* primaryRenderer;
    SDL_Renderer* secondaryRenderer;
    TTF_Font* font;

    // Utility functions for loading textures (moved from renderer.h)
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);
};

#endif // ASSET_MANAGER_H