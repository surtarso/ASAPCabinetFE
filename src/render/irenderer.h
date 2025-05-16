#ifndef IRENDERER_H
#define IRENDERER_H

#include "render/iasset_manager.h"
#include "core/iwindow_manager.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void render(IAssetManager& assets) = 0;
    virtual void setRenderers(IWindowManager* windowManager) = 0;
};

#endif // IRENDERER_H