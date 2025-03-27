#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include "render/asset_manager.h"
#include "config/ui/setup_editor.h"

class Renderer {
public:
    Renderer(SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer);
    void render(AssetManager& assets, bool showConfig, RuntimeEditor& configEditor);

private:
    SDL_Renderer* primaryRenderer_;
    SDL_Renderer* secondaryRenderer_;

    void renderPrimaryWindow(AssetManager& assets);
    void renderSecondaryWindow(AssetManager& assets);
};

#endif // RENDERER_H