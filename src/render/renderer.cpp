#include "render/renderer.h"
#include "render/video_player.h"
#include "utils/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h"

Renderer::Renderer(SDL_Renderer* playfieldRenderer, SDL_Renderer* backglassRenderer, SDL_Renderer* dmdRenderer)
    : playfieldRenderer_(playfieldRenderer),
      backglassRenderer_(backglassRenderer),
      dmdRenderer_(dmdRenderer) {}

void Renderer::setRenderers(IWindowManager* windowManager) {
    playfieldRenderer_ = windowManager->getPlayfieldRenderer();
    backglassRenderer_ = windowManager->getBackglassRenderer();
    dmdRenderer_ = windowManager->getDMDRenderer();
    LOG_DEBUG("Renderer: Updated renderers - playfield=" << playfieldRenderer_ 
              << ", backglass=" << backglassRenderer_ << ", dmd=" << dmdRenderer_);
}

void Renderer::render(IAssetManager& assets) {
    renderPlayfieldWindow(assets);
    renderBackglassWindow(assets);
    renderDMDWindow(assets);
}

void Renderer::renderPlayfieldWindow(IAssetManager& assets) {
    if (!playfieldRenderer_) {
        LOG_ERROR("Renderer: Playfield renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
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
            //LOG_DEBUG("Renderer: Rendered playfield video");
        } else {
            LOG_DEBUG("Renderer: Playfield video player has no texture");
        }
    } else if (auto* texture = assets.getPlayfieldTexture()) {
        SDL_RenderCopyEx(playfieldRenderer_,
                         texture,
                         nullptr,
                         &playfieldRect,
                         settings.playfieldRotation,
                         nullptr,
                         SDL_FLIP_NONE);
        //LOG_DEBUG("Renderer: Rendered playfield texture");
    } else {
        LOG_DEBUG("Renderer: No playfield video or texture available");
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
            //LOG_DEBUG("Renderer: Rendered wheel texture");
        } else {
            LOG_DEBUG("Renderer: No wheel texture available");
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
            //LOG_DEBUG("Renderer: Rendered title texture at x=" << titleRect.x << ", y=" << titleRect.y);
        } else {
            LOG_DEBUG("Renderer: No title texture available");
        }
    }
}

void Renderer::renderBackglassWindow(IAssetManager& assets) {
    if (!backglassRenderer_) {
        LOG_DEBUG("Renderer: Backglass renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
    if (!settings.showBackglass) {
        //LOG_DEBUG("Renderer: Backglass rendering skipped (showBackglass=false)");
        return;
    }
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(backglassRenderer_, &windowWidth, &windowHeight);

    SDL_Rect backglassRect = {settings.backglassMediaX, settings.backglassMediaY, settings.backglassMediaWidth, settings.backglassMediaHeight};
    if (auto* videoPlayer = assets.getBackglassVideoPlayer()) {
        if (videoPlayer->texture) {
            updateVideoTexture(videoPlayer);
            SDL_RenderCopyEx(backglassRenderer_,
                             videoPlayer->texture,
                             nullptr,
                             &backglassRect,
                             settings.backglassRotation,
                             nullptr,
                             SDL_FLIP_NONE);
            //LOG_DEBUG("Renderer: Rendered backglass video");
        } else {
            LOG_DEBUG("Renderer: Backglass video player has no texture");
        }
    } else if (auto* texture = assets.getBackglassTexture()) {
        SDL_RenderCopyEx(backglassRenderer_,
                         texture,
                         nullptr,
                         &backglassRect,
                         settings.backglassRotation,
                         nullptr,
                         SDL_FLIP_NONE);
        //LOG_DEBUG("Renderer: Rendered backglass texture");
    } else {
        LOG_DEBUG("Renderer: No backglass video or texture available");
    }
}

void Renderer::renderDMDWindow(IAssetManager& assets) {
    if (!dmdRenderer_) {
        LOG_DEBUG("Renderer: DMD renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
    if (!settings.showDMD) {
        //LOG_DEBUG("Renderer: DMD rendering skipped (showDMD=false)");
        return;
    }
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(dmdRenderer_, &windowWidth, &windowHeight);

    SDL_Rect dmdRect = {settings.dmdMediaX, settings.dmdMediaY, settings.dmdMediaWidth, settings.dmdMediaHeight};
    if (auto* videoPlayer = assets.getDmdVideoPlayer()) {
        if (videoPlayer->texture) {
            updateVideoTexture(videoPlayer);
            SDL_RenderCopyEx(dmdRenderer_,
                             videoPlayer->texture,
                             nullptr,
                             &dmdRect,
                             settings.dmdRotation,
                             nullptr,
                             SDL_FLIP_NONE);
            //LOG_DEBUG("Renderer: Rendered DMD video");
        } else {
            LOG_DEBUG("Renderer: DMD video player has no texture");
        }
    } else if (auto* texture = assets.getDmdTexture()) {
        SDL_RenderCopyEx(dmdRenderer_,
                         texture,
                         nullptr,
                         &dmdRect,
                         settings.dmdRotation,
                         nullptr,
                         SDL_FLIP_NONE);
        //LOG_DEBUG("Renderer: Rendered DMD texture");
    } else {
        LOG_DEBUG("Renderer: No DMD video or texture available");
    }
}