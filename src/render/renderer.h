#ifndef RENDERER_H
#define RENDERER_H

#include "render/irenderer.h"
#include <SDL.h>

class Renderer : public IRenderer {
public:
    Renderer(SDL_Renderer* playfieldRenderer, 
            SDL_Renderer* backglassRenderer,
            SDL_Renderer* dmdRenderer);
    void render(IAssetManager& assets) override;
    void setRenderers(IWindowManager* windowManager) override;

private:
    SDL_Renderer* playfieldRenderer_;
    SDL_Renderer* backglassRenderer_;
    SDL_Renderer* dmdRenderer_;

    void renderPlayfieldWindow(IAssetManager& assets);
    void renderBackglassWindow(IAssetManager& assets);
    void renderDMDWindow(IAssetManager& assets);
};

#endif // RENDERER_H