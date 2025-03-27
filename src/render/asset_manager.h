#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "render/table_loader.h"
#include "render/video_player.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

class SettingsManager;

class AssetManager {
public:
    AssetManager(SDL_Renderer* primary, SDL_Renderer* secondary, TTF_Font* f);
    void loadTableAssets(size_t index, const std::vector<TableLoader>& tables);
    void clearOldVideoPlayers();
    void addOldVideoPlayer(VideoContext* player);
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);

    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> tableTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> wheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> tableNameTexture;
    SDL_Rect tableNameRect;
    VideoContext* tableVideoPlayer;
    VideoContext* backglassVideoPlayer;
    VideoContext* dmdVideoPlayer;

    SDL_Renderer* getPrimaryRenderer() { return primaryRenderer; }
    SDL_Renderer* getSecondaryRenderer() { return secondaryRenderer; }
    VideoContext* getTableVideoPlayer() { return tableVideoPlayer; }
    VideoContext* getBackglassVideoPlayer() { return backglassVideoPlayer; }
    VideoContext* getDmdVideoPlayer() { return dmdVideoPlayer; }

    void setSettingsManager(SettingsManager* cm) { configManager_ = cm; }
    SettingsManager* getSettingsManager() { return configManager_; }
    TTF_Font* getFont() { return font; }

private:
    SDL_Renderer* primaryRenderer;
    SDL_Renderer* secondaryRenderer;
    TTF_Font* font;
    SettingsManager* configManager_;
    std::vector<VideoContext*> oldVideoPlayers_;
};

#endif // ASSET_MANAGER_H