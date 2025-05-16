#ifndef IASSET_MANAGER_H
#define IASSET_MANAGER_H

#include <SDL.h>
#include <memory>
#include "config/iconfig_service.h"

struct VideoContext;

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
    virtual VideoContext* getPlayfieldVideoPlayer() = 0;
    virtual VideoContext* getBackglassVideoPlayer() = 0;
    virtual VideoContext* getDmdVideoPlayer() = 0;
    
    // Settings and positioning
    virtual IConfigService* getSettingsManager() = 0;
    virtual SDL_Rect getTitleRect() = 0;
    virtual void setTitlePosition(int x, int y) = 0;
};

#endif // IASSET_MANAGER_H