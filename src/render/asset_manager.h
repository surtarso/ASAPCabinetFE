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
#include <unordered_map>
#include <deque>
#include <list> // For LRU cache management
#include <mutex> // For potential multi-threading

class IConfigService;
class IWindowManager;

class AssetManager : public IAssetManager {
public:
    AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, SDL_Renderer* topper, TTF_Font* f, ISoundManager* soundManager);
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
    void addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player);
    void clearVideoCache();
    void clearTextureCache(); // New function to clear texture cache
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
    struct WindowAssetInfo {
        SDL_Renderer* renderer;
        SDL_Texture*& texture; // Now raw pointer, AssetManager member will point to cached texture
        SDL_Texture*& wheelTexture; // Now raw pointer
        SDL_Texture*& titleTexture; // Now raw pointer
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

    // Texture cache: path -> {renderer, unique_ptr<texture>}
    struct TextureCacheEntry {
        SDL_Renderer* renderer;
        std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> texture;
        TextureCacheEntry(SDL_Renderer* r, SDL_Texture* t) : renderer(r), texture(t, SDL_DestroyTexture) {}
    };
    std::unordered_map<std::string, TextureCacheEntry> textureCache_;
    std::list<std::string> lru_texture_keys_; // List to maintain LRU order of textureCache_ keys

    // Video player cache: path_width_height -> {renderer, width, height, player}
    struct VideoPlayerCacheEntry {
        SDL_Renderer* renderer;
        int width;
        int height;
        std::unique_ptr<IVideoPlayer> player;
        VideoPlayerCacheEntry(SDL_Renderer* r, int w, int h, std::unique_ptr<IVideoPlayer> p)
            : renderer(r), width(w), height(h), player(std::move(p)) {}
    };
    std::unordered_map<std::string, VideoPlayerCacheEntry> videoPlayerCache_;
    std::list<std::string> lru_video_keys_; // List to maintain LRU order of videoPlayerCache_ keys

    SDL_Texture* playfieldTexture; // Now raw pointer
    SDL_Texture* playfieldWheelTexture; // Now raw pointer
    SDL_Texture* playfieldTitleTexture; // Now raw pointer
    SDL_Texture* backglassTexture; // Now raw pointer
    SDL_Texture* backglassWheelTexture; // Now raw pointer
    SDL_Texture* backglassTitleTexture; // Now raw pointer
    SDL_Texture* dmdTexture; // Now raw pointer
    SDL_Texture* dmdWheelTexture; // Now raw pointer
    SDL_Texture* dmdTitleTexture; // Now raw pointer
    SDL_Texture* topperTexture; // Now raw pointer
    SDL_Texture* topperWheelTexture; // Now raw pointer
    SDL_Texture* topperTitleTexture; // Now raw pointer
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
    std::deque<std::unique_ptr<IVideoPlayer>> oldVideoPlayers_;
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