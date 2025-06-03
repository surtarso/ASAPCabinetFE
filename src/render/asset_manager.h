/**
 * @file asset_manager.h
 * @brief Defines the AssetManager class for managing VPX table assets in ASAPCabinetFE.
 *
 * This header provides the AssetManager class, which implements the IAssetManager interface
 * to manage textures, video players, audio, and table data for Visual Pinball X (VPX) tables.
 * It handles asset loading, caching, and rendering resources using SDL and TTF.
 */
#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "tables/itable_loader.h"
#include "tables/table_data.h"
#include "render/ivideo_player.h"
#include "render/iasset_manager.h"
#include "sound/isound_manager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class IWindowManager
 * @brief Interface for window management (forward declaration).
 */
class IWindowManager;

/**
 * @class AssetManager
 * @brief Manages textures, video players, audio, and table assets for VPX rendering.
 *
 * This class implements the IAssetManager interface to load, cache, and provide access
 * to textures, video players, music, and table data for playfield, backglass, DMD, and topper displays.
 * It interfaces with IConfigService for settings, ITableLoader for table data, ISoundManager
 * for audio, and SDL/TTF for rendering resources.
 */
class AssetManager : public IAssetManager {
public:
    AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager);
    SDL_Texture* getPlayfieldTexture() override { return playfieldTexture.get(); }
    SDL_Texture* getWheelTexture(SDL_Renderer* renderer) override {
        if (renderer == playfieldRenderer) return playfieldWheelTexture.get();
        if (renderer == backglassRenderer) return backglassWheelTexture.get();
        if (renderer == dmdRenderer) return dmdWheelTexture.get();
        if (renderer == topperRenderer) return topperWheelTexture.get();
        return nullptr;
    }
    SDL_Texture* getBackglassTexture() override { return backglassTexture.get(); }
    SDL_Texture* getDmdTexture() override { return dmdTexture.get(); }
    SDL_Texture* getTopperTexture() override { return topperTexture.get(); }
    SDL_Texture* getTitleTexture(SDL_Renderer* renderer) override {
        if (renderer == playfieldRenderer) return playfieldTitleTexture.get();
        if (renderer == backglassRenderer) return backglassTitleTexture.get();
        if (renderer == dmdRenderer) return dmdTitleTexture.get();
        if (renderer == topperRenderer) return topperTitleTexture.get();
        return nullptr;
    }
    IVideoPlayer* getPlayfieldVideoPlayer() override { return playfieldVideoPlayer.get(); }
    IVideoPlayer* getBackglassVideoPlayer() override { return backglassVideoPlayer.get(); }
    IVideoPlayer* getDmdVideoPlayer() override { return dmdVideoPlayer.get(); }
    IVideoPlayer* getTopperVideoPlayer() override { return topperVideoPlayer.get(); }
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
    void setSoundManager(ISoundManager* soundManager) override;
    void playTableMusic(size_t index, const std::vector<TableData>& tables) override;
    void applyVideoAudioSettings() override;
    void addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player);
    void clearVideoCache();
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);

    SDL_Renderer* getPlayfieldRenderer() { return playfieldRenderer; }
    SDL_Renderer* getBackglassRenderer() { return backglassRenderer; }
    SDL_Renderer* getDMDRenderer() { return dmdRenderer; }
    SDL_Renderer* getTopperRenderer() { return topperRenderer; }
    void setPlayfieldRenderer(SDL_Renderer* renderer) { playfieldRenderer = renderer; }
    void setBackglassRenderer(SDL_Renderer* renderer) { backglassRenderer = renderer; }
    void setDMDRenderer(SDL_Renderer* renderer) { dmdRenderer = renderer; }
    void setTopperRenderer(SDL_Renderer* renderer) { topperRenderer = renderer; }

private:
    /**
     * @brief Struct to group window-specific asset data.
     */
    struct WindowAssetInfo {
        SDL_Renderer* renderer;
        std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)>& texture;
        std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)>& wheelTexture;
        std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)>& titleTexture;
        std::unique_ptr<IVideoPlayer>& videoPlayer;
        std::string& imagePath;
        std::string& wheelImagePath;
        std::string& videoPath;
        int& mediaWidth;
        int& mediaHeight;
        bool show;
        const char* name;
        const std::string& tableImage;
        const std::string& tableVideo;
    };

    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> playfieldTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> playfieldWheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> playfieldTitleTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassWheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTitleTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdWheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTitleTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> topperTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> topperWheelTexture;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> topperTitleTexture;
    SDL_Rect titleRect;
    std::unique_ptr<IVideoPlayer> playfieldVideoPlayer;
    std::unique_ptr<IVideoPlayer> backglassVideoPlayer;
    std::unique_ptr<IVideoPlayer> dmdVideoPlayer;
    std::unique_ptr<IVideoPlayer> topperVideoPlayer;
    SDL_Renderer* playfieldRenderer;
    SDL_Renderer* backglassRenderer;
    SDL_Renderer* dmdRenderer;
    SDL_Renderer* topperRenderer;
    ISoundManager* soundManager_;
    std::string currentPlayfieldVideoPath_;
    std::string currentBackglassVideoPath_;
    std::string currentDmdVideoPath_;
    std::string currentTopperVideoPath_;
    TTF_Font* font;
    IConfigService* configManager_;
    std::unique_ptr<ITableLoader> tableLoader_;
    std::vector<std::unique_ptr<IVideoPlayer>> oldVideoPlayers_;
    std::string currentPlayfieldImagePath_;
    std::string currentPlayfieldWheelImagePath_;
    std::string currentBackglassImagePath_;
    std::string currentBackglassWheelImagePath_;
    std::string currentDmdImagePath_;
    std::string currentDmdWheelImagePath_;
    std::string currentTopperImagePath_;
    std::string currentTopperWheelImagePath_;
    int currentPlayfieldMediaWidth_ = 0;
    int currentPlayfieldMediaHeight_ = 0;
    int currentBackglassMediaWidth_ = 0;
    int currentBackglassMediaHeight_ = 0;
    int currentDmdMediaWidth_ = 0;
    int currentDmdMediaHeight_ = 0;
    int currentTopperMediaWidth_ = 0;
    int currentTopperMediaHeight_ = 0;
};

#endif // ASSET_MANAGER_H