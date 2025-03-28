#ifndef RENDERER_H
#define RENDERER_H

#include "render/irenderer.h" // Inherit from interface
#include <SDL.h>

class Renderer : public IRenderer { // Implement IRenderer
public:
    Renderer(SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer);
    void render(AssetManager& assets) override; // Simplified signature

private:
    SDL_Renderer* primaryRenderer_;
    SDL_Renderer* secondaryRenderer_;

    void renderPrimaryWindow(AssetManager& assets);
    void renderSecondaryWindow(AssetManager& assets);
};

#endif // RENDERER_H