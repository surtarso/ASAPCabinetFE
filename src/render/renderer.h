#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include "table/asset_manager.h"
#include "render/transition_manager.h"
#include "config/config_gui.h"

class Renderer {
public:
    Renderer(SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer);
    void render(AssetManager& assets, TransitionManager& transitionManager,
                bool showConfig, IniEditor& configEditor);

private:
    SDL_Renderer* primaryRenderer_;
    SDL_Renderer* secondaryRenderer_;

    void renderPrimaryWindow(AssetManager& assets, TransitionManager& transitionManager,
                             bool showConfig, IniEditor& configEditor);
    void renderSecondaryWindow(AssetManager& assets, TransitionManager& transitionManager);
};

#endif // RENDERER_H