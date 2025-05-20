#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "render/itable_loader.h"
#include "render/table_data.h"
#include "render/ivideo_player.h"
#include "render/iasset_manager.h" // Include IAssetManager
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory> // For std::unique_ptr
#include <vector>

class IConfigService;
class IWindowManager; // Forward declare IWindowManager if not already included

class AssetManager : public IAssetManager {
public:
    AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, TTF_Font* f);

    // IAssetManager interface implementation (these are already correct)
    SDL_Texture* getPlayfieldTexture() override { return playfieldTexture.get(); }
    SDL_Texture* getWheelTexture() override { return wheelTexture.get(); }
    SDL_Texture* getBackglassTexture() override { return backglassTexture.get(); }
    SDL_Texture* getDmdTexture() override { return dmdTexture.get(); }
    SDL_Texture* getTitleTexture() override { return titleTexture.get(); }
    IVideoPlayer* getPlayfieldVideoPlayer() override { return playfieldVideoPlayer.get(); }
    IVideoPlayer* getBackglassVideoPlayer() override { return backglassVideoPlayer.get(); }
    IVideoPlayer* getDmdVideoPlayer() override { return dmdVideoPlayer.get(); }
    IConfigService* getSettingsManager() override { return configManager_; }
    SDL_Rect getTitleRect() override { return titleRect; }
    void setTitlePosition(int x, int y) override;
    void setFont(TTF_Font* font) override;
    void reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) override;
    void reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) override;
    void setSettingsManager(IConfigService* cm) override;
    void loadTableAssets(size_t index, const std::vector<TableData>& tables) override;
    void clearOldVideoPlayers() override;
    void cleanupVideoPlayers() override;

    // Non-interface methods
    // CORRECTED: Changed parameter type to std::unique_ptr
    void addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player);
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
    std::unique_ptr<IVideoPlayer> playfieldVideoPlayer;
    std::unique_ptr<IVideoPlayer> backglassVideoPlayer;
    std::unique_ptr<IVideoPlayer> dmdVideoPlayer;

private:
    SDL_Renderer* playfieldRenderer;
    SDL_Renderer* backglassRenderer;
    SDL_Renderer* dmdRenderer;
    std::string currentPlayfieldVideoPath_;
    std::string currentBackglassVideoPath_;
    std::string currentDmdVideoPath_;
    TTF_Font* font;
    IConfigService* configManager_;
    std::unique_ptr<ITableLoader> tableLoader_;
    // This was already correctly changed to std::unique_ptr
    std::vector<std::unique_ptr<IVideoPlayer>> oldVideoPlayers_;

    // Caching for image textures
    std::string currentPlayfieldImagePath_;
    std::string currentWheelImagePath_;
    std::string currentBackglassImagePath_;
    std::string currentDmdImagePath_;

    // Track video settings for reuse
    int currentPlayfieldMediaWidth_ = 0;
    int currentPlayfieldMediaHeight_ = 0;
    int currentBackglassMediaWidth_ = 0;
    int currentBackglassMediaHeight_ = 0;
    int currentDmdMediaWidth_ = 0;
    int currentDmdMediaHeight_ = 0;
};

#endif // ASSET_MANAGER_H