#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "tables/itable_loader.h"
#include "tables/table_data.h"
#include "render/ivideo_player.h"
#include "render/iasset_manager.h"
#include "sound/isound_manager.h"
#include "render/texture_cache.h"
#include "render/video_player_cache.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

class IConfigService;
class IWindowManager;

class AssetManager : public IAssetManager {
public:
    AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager);
    ~AssetManager() override = default;

    SDL_Texture* getPlayfieldTexture() override { return playfieldTexture; }
    SDL_Texture* getWheelTexture(SDL_Renderer* renderer) override {
        if (renderer == playfieldRenderer) return playfieldWheelTexture;
        if (renderer == backglassRenderer) return backglassWheelTexture;
        if (renderer == dmdRenderer) return dmdWheelTexture;
        if (renderer == topperRenderer) return topperWheelTexture;
        return nullptr;
    }
    SDL_Texture* getBackglassTexture() override { return backglassTexture; }
    SDL_Texture* getDmdTexture() override { return dmdTexture; }
    SDL_Texture* getTopperTexture() override { return topperTexture; }
    SDL_Texture* getTitleTexture(SDL_Renderer* renderer) override {
        if (renderer == playfieldRenderer) return playfieldTitleTexture;
        if (renderer == backglassRenderer) return backglassTitleTexture;
        if (renderer == dmdRenderer) return dmdTitleTexture;
        if (renderer == topperRenderer) return topperTitleTexture;
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
    void clearVideoCache();
    void clearTextureCache();
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
    struct WindowAssetInfo {
        SDL_Renderer* renderer;
        SDL_Texture*& texture;
        SDL_Texture*& wheelTexture;
        SDL_Texture*& titleTexture;
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

    SDL_Texture* playfieldTexture;
    SDL_Texture* playfieldWheelTexture;
    SDL_Texture* playfieldTitleTexture;
    SDL_Texture* backglassTexture;
    SDL_Texture* backglassWheelTexture;
    SDL_Texture* backglassTitleTexture;
    SDL_Texture* dmdTexture;
    SDL_Texture* dmdWheelTexture;
    SDL_Texture* dmdTitleTexture;
    SDL_Texture* topperTexture;
    SDL_Texture* topperWheelTexture;
    SDL_Texture* topperTitleTexture;
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

    std::unique_ptr<TextureCache> textureCache_;
    std::unique_ptr<VideoPlayerCache> videoPlayerCache_;
};

#endif // ASSET_MANAGER_H