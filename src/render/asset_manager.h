#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "render/table_loader.h"
#include "render/video_player.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

class IConfigService;  // Forward declare to avoid including full header

class AssetManager {
public:
    // Constructor: Takes renderers and font, initializes smart pointers
    AssetManager(SDL_Renderer* primary, SDL_Renderer* secondary, TTF_Font* f);
    
    // Loads all assets (textures, videos) for a given table index
    void loadTableAssets(size_t index, const std::vector<TableLoader>& tables);
    
    // Cleans up old video players stored in the queue
    void clearOldVideoPlayers();
    
    // Adds a video player to the cleanup queue
    void addOldVideoPlayer(VideoContext* player);
    
    // Loads a texture from a file path, returns raw pointer (managed by smart pointers elsewhere)
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
    
    // Renders text to a texture, updates rect with dimensions
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);

    // Smart pointers for textures (auto-destroyed via SDL_DestroyTexture)
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> tableTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> wheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> tableNameTexture;
    
    // Rect for positioning table name text
    SDL_Rect tableNameRect;
    
    // Raw pointers to video players (managed manually via setup/cleanup)
    VideoContext* tableVideoPlayer;
    VideoContext* backglassVideoPlayer;
    VideoContext* dmdVideoPlayer;

    // Getters for renderers and video players
    SDL_Renderer* getPrimaryRenderer() { return primaryRenderer; }
    SDL_Renderer* getSecondaryRenderer() { return secondaryRenderer; }
    VideoContext* getTableVideoPlayer() { return tableVideoPlayer; }
    VideoContext* getBackglassVideoPlayer() { return backglassVideoPlayer; }
    VideoContext* getDmdVideoPlayer() { return dmdVideoPlayer; }

    // Config and font management
    void setSettingsManager(IConfigService* cm) { configManager_ = cm; }
    IConfigService* getSettingsManager() { return configManager_; }
    TTF_Font* getFont() { return font; }
    void setFont(TTF_Font* f) { font = f; }

    // Cleanup method for all video players (moved from App)
    void cleanupVideoPlayers();

private:
    SDL_Renderer* primaryRenderer;      // Main window renderer
    SDL_Renderer* secondaryRenderer;    // Backglass/DMD renderer
    TTF_Font* font;                     // Font for text rendering
    IConfigService* configManager_;     // Pointer to config service for settings
    std::vector<VideoContext*> oldVideoPlayers_;  // Queue of old video players to clean
};

#endif // ASSET_MANAGER_H