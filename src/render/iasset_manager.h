#ifndef IASSET_MANAGER_H
#define IASSET_MANAGER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <vector>
#include "config/iconfig_service.h"
#include "render/table_loader.h"
#include "core/iwindow_manager.h"
#include "render/ivideo_player.h"

class IAssetManager {
public:
    virtual ~IAssetManager() = default;
    
    // Texture accessors
    virtual SDL_Texture* getPlayfieldTexture() = 0;
    virtual SDL_Texture* getWheelTexture() = 0;
    virtual SDL_Texture* getBackglassTexture() = 0;
    virtual SDL_Texture* getDmdTexture() = 0;
    virtual SDL_Texture* getTitleTexture() = 0;
    
    // Video player accessors
    virtual IVideoPlayer* getPlayfieldVideoPlayer() = 0;
    virtual IVideoPlayer* getBackglassVideoPlayer() = 0;
    virtual IVideoPlayer* getDmdVideoPlayer() = 0;
    
    // Settings and positioning
    virtual IConfigService* getSettingsManager() = 0;
    virtual SDL_Rect getTitleRect() = 0;
    virtual void setTitlePosition(int x, int y) = 0;
    
    // Font management
    virtual void setFont(TTF_Font* font) = 0;
    virtual void reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) = 0;
    
    // Asset management
    virtual void reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) = 0;
    virtual void setSettingsManager(IConfigService* cm) = 0;
    virtual void loadTableAssets(size_t index, const std::vector<TableData>& tables) = 0;
    virtual void clearOldVideoPlayers() = 0;
    virtual void cleanupVideoPlayers() = 0;
};

#endif // IASSET_MANAGER_H