#ifndef RENDERER_H
#define RENDERER_H

#include "render/irenderer.h" // Inherit from interface
#include <SDL.h>

class Renderer : public IRenderer { // Implement IRenderer
public:
    Renderer(SDL_Renderer* playfieldRenderer, 
                SDL_Renderer* backglassRenderer,
                    SDL_Renderer* dmdRenderer);
    void render(AssetManager& assets) override; // Simplified signature

private:
    SDL_Renderer* playfieldRenderer_;
    SDL_Renderer* backglassRenderer_;
    SDL_Renderer* dmdRenderer_;

    void renderPlayfieldWindow(AssetManager& assets);
    void renderBackglassWindow(AssetManager& assets);
    void renderDMDWindow(AssetManager& assets);
};

#endif // RENDERER_H