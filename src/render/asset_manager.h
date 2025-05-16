#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "render/table_loader.h"
#include "render/video_player.h"
#include "render/iasset_manager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

class IConfigService;

class AssetManager : public IAssetManager {
public:
    AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, TTF_Font* f);
    
    // IAssetManager interface implementation
    SDL_Texture* getPlayfieldTexture() override { return playfieldTexture.get(); }
    SDL_Texture* getWheelTexture() override { return wheelTexture.get(); }
    SDL_Texture* getBackglassTexture() override { return backglassTexture.get(); }
    SDL_Texture* getDmdTexture() override { return dmdTexture.get(); }
    SDL_Texture* getTitleTexture() override { return titleTexture.get(); }
    VideoContext* getPlayfieldVideoPlayer() override { return playfieldVideoPlayer; }
    VideoContext* getBackglassVideoPlayer() override { return backglassVideoPlayer; }
    VideoContext* getDmdVideoPlayer() override { return dmdVideoPlayer; }
    IConfigService* getSettingsManager() override { return configManager_; }
    SDL_Rect getTitleRect() override { return titleRect; }
    void setTitlePosition(int x, int y) override;
    void setFont(TTF_Font* font) override;
    void reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) override;
    void reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableLoader>& tables, size_t index) override;
    void setSettingsManager(IConfigService* cm) override;
    void loadTableAssets(size_t index, const std::vector<TableLoader>& tables) override;
    void clearOldVideoPlayers() override;
    void cleanupVideoPlayers() override;

    // Non-interface methods
    void addOldVideoPlayer(VideoContext* player);
    void clearVideoCache();
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);
    SDL_Renderer* getPlayfieldRenderer() { return playfieldRenderer; }
    SDL_Renderer* getBackglassRenderer() { return backglassRenderer; }
    SDL_Renderer* getDMDRenderer() { return dmdRenderer; }
    void setPlayfieldRenderer(SDL_Renderer* renderer) { playfieldRenderer = renderer; }
    void setBackglassRenderer(SDL_Renderer* renderer) { backglassRenderer = renderer; }
    void setDMDRenderer(SDL_Renderer* renderer) { dmdRenderer = renderer; }

    // Smart pointers for textures
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> playfieldTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> wheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> titleTexture;
    
    SDL_Rect titleRect;
    VideoContext* playfieldVideoPlayer;
    VideoContext* backglassVideoPlayer;
    VideoContext* dmdVideoPlayer;

private:
    SDL_Renderer* playfieldRenderer;
    SDL_Renderer* backglassRenderer;
    SDL_Renderer* dmdRenderer;
    std::string currentPlayfieldVideoPath_;
    std::string currentBackglassVideoPath_;
    std::string currentDmdVideoPath_;
    TTF_Font* font;
    IConfigService* configManager_;
    std::vector<VideoContext*> oldVideoPlayers_;
};

#endif // ASSET_MANAGER_H