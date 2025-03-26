#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include "table/asset_manager.h"
#include "config/config_gui.h"

class Renderer {
public:
    Renderer(SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer);
    void render(AssetManager& assets, bool showConfig, IniEditor& configEditor);

private:
    SDL_Renderer* primaryRenderer_;
    SDL_Renderer* secondaryRenderer_;

    void renderPrimaryWindow(AssetManager& assets, bool showConfig, IniEditor& configEditor);
    void renderSecondaryWindow(AssetManager& assets);
};

#endif // RENDERER_H