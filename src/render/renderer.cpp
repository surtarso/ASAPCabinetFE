#include "render/renderer.h"
#include "utils/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h"

Renderer::Renderer(SDL_Renderer *playfieldRenderer, SDL_Renderer *backglassRenderer, SDL_Renderer *dmdRenderer)
    : playfieldRenderer_(playfieldRenderer),
    backglassRenderer_(backglassRenderer),
    dmdRenderer_(dmdRenderer) {}

void Renderer::render(AssetManager &assets) {
    renderPlayfieldWindow(assets);
    renderBackglassWindow(assets);
    renderDMDWindow(assets);
}

// Render the Playfield window
void Renderer::renderPlayfieldWindow(AssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(playfieldRenderer_, &windowWidth, &windowHeight);

    SDL_Rect playfieldRect = {settings.playfieldMediaX, settings.playfieldMediaY, settings.playfieldMediaWidth, settings.playfieldMediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX, settings.wheelMediaY, settings.wheelMediaWidth, settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.titleRect;

    // Render Playfield texture/video
    if (assets.playfieldVideoPlayer && assets.playfieldVideoPlayer->texture) {
        updateVideoTexture(assets.playfieldVideoPlayer);
        SDL_RenderCopyEx(playfieldRenderer_,
            assets.playfieldVideoPlayer->texture,
            nullptr,
            &playfieldRect,
            settings.playfieldRotation,
            nullptr,
            SDL_FLIP_NONE);

    } else if (assets.playfieldTexture) {
        SDL_RenderCopyEx(playfieldRenderer_,
            assets.playfieldTexture.get(),
            nullptr,
            &playfieldRect,
            settings.playfieldRotation,
            nullptr,
            SDL_FLIP_NONE);
    }
    
    // Render wheel if enabled
    if (settings.showWheel && assets.wheelTexture) {
        SDL_RenderCopyEx(
            playfieldRenderer_,
            assets.wheelTexture.get(),
            nullptr,
            &wheelRect,
            settings.playfieldRotation,
            nullptr,
            SDL_FLIP_NONE);
    }
    
    // Render title if enabled
    if (settings.showTitle && assets.titleTexture) {
        titleRect.x = 10;
        titleRect.y = windowHeight - titleRect.h - 10;
        SDL_SetRenderDrawColor(playfieldRenderer_,
                               settings.fontBgColor.r,
                               settings.fontBgColor.g,
                               settings.fontBgColor.b,
                               settings.fontBgColor.a);
        SDL_Rect titleBgRect = {titleRect.x - 5, titleRect.y - 5, titleRect.w + 10, titleRect.h + 10};
        SDL_RenderFillRect(playfieldRenderer_, &titleBgRect);
        SDL_RenderCopyEx(
            playfieldRenderer_,
            assets.titleTexture.get(),
            nullptr,
            &titleRect,
            settings.playfieldRotation,
            nullptr,
            SDL_FLIP_NONE);
    }
}

// Render the Backglass window
void Renderer::renderBackglassWindow(AssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(backglassRenderer_, &windowWidth, &windowHeight);

    SDL_Rect backglassRect = {settings.backglassMediaX, settings.backglassMediaY, settings.backglassMediaWidth, settings.backglassMediaHeight};

    if (assets.backglassVideoPlayer && assets.backglassVideoPlayer->texture) {
        updateVideoTexture(assets.backglassVideoPlayer);
        SDL_RenderCopyEx(
            backglassRenderer_,
            assets.backglassVideoPlayer->texture,
            nullptr,
            &backglassRect,
            settings.backglassRotation,
            nullptr,
            SDL_FLIP_NONE);

    } else if (assets.backglassTexture) {
        SDL_RenderCopyEx(
            backglassRenderer_,
            assets.backglassTexture.get(),
            nullptr,
            &backglassRect,
            settings.backglassRotation,
            nullptr,
            SDL_FLIP_NONE);
    }
}

// Render the DMD window
void Renderer::renderDMDWindow(AssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(dmdRenderer_, &windowWidth, &windowHeight);

    SDL_Rect dmdRect = {settings.dmdMediaX, settings.dmdMediaY, settings.dmdMediaWidth, settings.dmdMediaHeight};

    if (assets.dmdVideoPlayer && assets.dmdVideoPlayer->texture) {
        updateVideoTexture(assets.dmdVideoPlayer);
        SDL_RenderCopyEx(
            dmdRenderer_,
            assets.dmdVideoPlayer->texture,
            nullptr,
            &dmdRect,
            settings.dmdRotation,
            nullptr,
            SDL_FLIP_NONE);

    } else if (assets.dmdTexture) {
        SDL_RenderCopyEx(
            dmdRenderer_,
            assets.dmdTexture.get(),
            nullptr,
            &dmdRect,
            settings.dmdRotation,
            nullptr,
            SDL_FLIP_NONE);
    }
}