#ifndef RENDERER_H
#define RENDERER_H

#include "render/irenderer.h" // Inherit from interface
#include <SDL.h>

class Renderer : public IRenderer { // Implement IRenderer
public:
    Renderer(SDL_Renderer* playfieldRenderer, 
                SDL_Renderer* backglassRenderer,
                    SDL_Renderer* dmdRenderer);
    void render(IAssetManager& assets) override; // Simplified signature

    // Public setter methods for renderers
    void setPlayfieldRenderer(SDL_Renderer* renderer) { playfieldRenderer_ = renderer; }
    void setBackglassRenderer(SDL_Renderer* renderer) { backglassRenderer_ = renderer; }
    void setDMDRenderer(SDL_Renderer* renderer) { dmdRenderer_ = renderer; }

private:
    SDL_Renderer* playfieldRenderer_;
    SDL_Renderer* backglassRenderer_;
    SDL_Renderer* dmdRenderer_;

    void renderPlayfieldWindow(IAssetManager& assets);
    void renderBackglassWindow(IAssetManager& assets);
    void renderDMDWindow(IAssetManager& assets);
};

#endif // RENDERER_H