#include "render/renderer.h"
#include "utils/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h"

Renderer::Renderer(SDL_Renderer *primaryRenderer, SDL_Renderer *secondaryRenderer)
    : primaryRenderer_(primaryRenderer), secondaryRenderer_(secondaryRenderer) {}

void Renderer::render(AssetManager &assets) {
    renderPrimaryWindow(assets);
    renderSecondaryWindow(assets);
}

void Renderer::renderPrimaryWindow(AssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(primaryRenderer_, &windowWidth, &windowHeight);

    SDL_Rect tableRect = {0, 0, windowWidth, windowHeight}; // Full window
    SDL_Rect wheelRect = {0, 0, settings.wheelImageSize, settings.wheelImageSize};
    SDL_Rect nameRect = assets.tableNameRect;

    if (assets.tableVideoPlayer && assets.tableVideoPlayer->texture) {
        updateVideoTexture(assets.tableVideoPlayer);
        SDL_RenderCopy(primaryRenderer_, assets.tableVideoPlayer->texture, nullptr, &tableRect);
    } else if (assets.tableTexture) {
        SDL_RenderCopy(primaryRenderer_, assets.tableTexture.get(), nullptr, &tableRect);
    }
    
    // Only render wheel if enabled
    if (settings.showWheel && assets.wheelTexture) {
        wheelRect.x = windowWidth - wheelRect.w - settings.wheelImageMargin;
        wheelRect.y = windowHeight - wheelRect.h - settings.wheelImageMargin;
        SDL_RenderCopy(primaryRenderer_, assets.wheelTexture.get(), nullptr, &wheelRect);
    }
    // Only render title if enabled
    if (settings.showTitle && assets.tableNameTexture) {
        nameRect.x = 10;
        nameRect.y = windowHeight - nameRect.h - 10;
        SDL_SetRenderDrawColor(primaryRenderer_,
                               settings.fontBgColor.r,
                               settings.fontBgColor.g,
                               settings.fontBgColor.b,
                               settings.fontBgColor.a);
        SDL_Rect bgRect = {nameRect.x - 5, nameRect.y - 5, nameRect.w + 10, nameRect.h + 10};
        SDL_RenderFillRect(primaryRenderer_, &bgRect);
        SDL_RenderCopy(primaryRenderer_, assets.tableNameTexture.get(), nullptr, &nameRect);
    }
}

void Renderer::renderSecondaryWindow(AssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(secondaryRenderer_, &windowWidth, &windowHeight);

    SDL_Rect backglassRect = {0, 0, windowWidth, settings.backglassMediaHeight};
    SDL_Rect dmdRect = {0, backglassRect.h, windowWidth, settings.dmdMediaHeight};

    if (assets.backglassVideoPlayer && assets.backglassVideoPlayer->texture) {
        updateVideoTexture(assets.backglassVideoPlayer);
        SDL_RenderCopy(secondaryRenderer_, assets.backglassVideoPlayer->texture, nullptr, &backglassRect);
    } else if (assets.backglassTexture) {
        SDL_RenderCopy(secondaryRenderer_, assets.backglassTexture.get(), nullptr, &backglassRect);
    }
    if (assets.dmdVideoPlayer && assets.dmdVideoPlayer->texture) {
        updateVideoTexture(assets.dmdVideoPlayer);
        SDL_RenderCopy(secondaryRenderer_, assets.dmdVideoPlayer->texture, nullptr, &dmdRect);
    } else if (assets.dmdTexture) {
        SDL_RenderCopy(secondaryRenderer_, assets.dmdTexture.get(), nullptr, &dmdRect);
    }
}
