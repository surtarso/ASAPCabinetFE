#include "render/renderer.h"
#include "render/video_player.h"
#include "utils/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h"

Renderer::Renderer(SDL_Renderer *playfieldRenderer, SDL_Renderer *backglassRenderer, SDL_Renderer *dmdRenderer)
    : playfieldRenderer_(playfieldRenderer),
      backglassRenderer_(backglassRenderer),
      dmdRenderer_(dmdRenderer) {}

void Renderer::render(IAssetManager &assets) {
    renderPlayfieldWindow(assets);
    renderBackglassWindow(assets);
    renderDMDWindow(assets);
}

void Renderer::renderPlayfieldWindow(IAssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(playfieldRenderer_, &windowWidth, &windowHeight);

    SDL_Rect playfieldRect = {settings.playfieldMediaX, settings.playfieldMediaY, settings.playfieldMediaWidth, settings.playfieldMediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX, settings.wheelMediaY, settings.wheelMediaWidth, settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect();

    // Render Playfield texture/video
    if (auto* videoPlayer = assets.getPlayfieldVideoPlayer()) {
        if (videoPlayer->texture) {
            updateVideoTexture(videoPlayer);
            SDL_RenderCopyEx(playfieldRenderer_,
                             videoPlayer->texture,
                             nullptr,
                             &playfieldRect,
                             settings.playfieldRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    } else if (auto* texture = assets.getPlayfieldTexture()) {
        SDL_RenderCopyEx(playfieldRenderer_,
                         texture,
                         nullptr,
                         &playfieldRect,
                         settings.playfieldRotation,
                         nullptr,
                         SDL_FLIP_NONE);
    }
    
    // Render wheel if enabled
    if (settings.showWheel) {
        if (auto* texture = assets.getWheelTexture()) {
            SDL_RenderCopyEx(playfieldRenderer_,
                             texture,
                             nullptr,
                             &wheelRect,
                             settings.playfieldRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    }

    // Render title if enabled
    if (settings.showTitle) {
        if (auto* texture = assets.getTitleTexture()) {
            SDL_SetRenderDrawColor(playfieldRenderer_,
                                   settings.fontBgColor.r,
                                   settings.fontBgColor.g,
                                   settings.fontBgColor.b,
                                   settings.fontBgColor.a);
            SDL_Rect titleBgRect = {titleRect.x - 5, titleRect.y - 5, titleRect.w + 10, titleRect.h + 10};
            SDL_RenderFillRect(playfieldRenderer_, &titleBgRect);
            SDL_RenderCopyEx(playfieldRenderer_,
                             texture,
                             nullptr,
                             &titleRect,
                             settings.playfieldRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    }
}

void Renderer::renderBackglassWindow(IAssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(backglassRenderer_, &windowWidth, &windowHeight);

    SDL_Rect backglassRect = {settings.backglassMediaX, settings.backglassMediaY, settings.backglassMediaWidth, settings.backglassMediaHeight};
    if (auto* videoPlayer = assets.getBackglassVideoPlayer()) {
        if (videoPlayer->texture && settings.showBackglass) {
            updateVideoTexture(videoPlayer);
            SDL_RenderCopyEx(backglassRenderer_,
                             videoPlayer->texture,
                             nullptr,
                             &backglassRect,
                             settings.backglassRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    } else if (auto* texture = assets.getBackglassTexture()) {
        if (settings.showBackglass) {
            SDL_RenderCopyEx(backglassRenderer_,
                             texture,
                             nullptr,
                             &backglassRect,
                             settings.backglassRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    }
}

void Renderer::renderDMDWindow(IAssetManager &assets) {
    const Settings &settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(dmdRenderer_, &windowWidth, &windowHeight);

    SDL_Rect dmdRect = {settings.dmdMediaX, settings.dmdMediaY, settings.dmdMediaWidth, settings.dmdMediaHeight};
    if (auto* videoPlayer = assets.getDmdVideoPlayer()) {
        if (videoPlayer->texture && settings.showDMD) {
            updateVideoTexture(videoPlayer);
            SDL_RenderCopyEx(dmdRenderer_,
                             videoPlayer->texture,
                             nullptr,
                             &dmdRect,
                             settings.dmdRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    } else if (auto* texture = assets.getDmdTexture()) {
        if (settings.showDMD) {
            SDL_RenderCopyEx(dmdRenderer_,
                             texture,
                             nullptr,
                             &dmdRect,
                             settings.dmdRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        }
    }
}