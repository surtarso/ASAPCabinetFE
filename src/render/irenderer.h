#ifndef IRENDERER_H
#define IRENDERER_H

#include "render/asset_manager.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void render(AssetManager& assets) = 0;
};

#endif // IRENDERER_H