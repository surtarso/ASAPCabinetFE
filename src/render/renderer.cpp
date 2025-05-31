#include "render/renderer.h"
#include "render/ivideo_player.h"
#include "utils/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h"

Renderer::Renderer(SDL_Renderer* playfieldRenderer, SDL_Renderer* backglassRenderer, SDL_Renderer* dmdRenderer, SDL_Renderer* topperRenderer)
    : playfieldRenderer_(playfieldRenderer),
      backglassRenderer_(backglassRenderer),
      dmdRenderer_(dmdRenderer),
      topperRenderer_(topperRenderer) {}

void Renderer::setRenderers(IWindowManager* windowManager) {
    playfieldRenderer_ = windowManager->getPlayfieldRenderer();
    backglassRenderer_ = windowManager->getBackglassRenderer();
    dmdRenderer_ = windowManager->getDMDRenderer();
    topperRenderer_ = windowManager->getTopperRenderer();
    LOG_DEBUG("Renderer: Updated renderers - playfield=" << playfieldRenderer_ 
              << ", backglass=" << backglassRenderer_ << ", dmd=" << dmdRenderer_
                << ", topper=" << topperRenderer_);
}

void Renderer::render(IAssetManager& assets) {
    renderPlayfieldWindow(assets);
    renderBackglassWindow(assets);
    renderDMDWindow(assets);
    renderTopperWindow(assets);
}

void Renderer::renderPlayfieldWindow(IAssetManager& assets) {
    if (!playfieldRenderer_) {
        LOG_ERROR("Playfield Renderer: Playfield renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(playfieldRenderer_, &windowWidth, &windowHeight);

    SDL_Rect playfieldRect = {settings.playfieldMediaX, settings.playfieldMediaY, settings.playfieldMediaWidth, settings.playfieldMediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX, settings.wheelMediaY, settings.wheelMediaWidth, settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect();

    if (auto* videoPlayer = assets.getPlayfieldVideoPlayer()) {
        if (videoPlayer->getTexture()) {
            videoPlayer->update();
            SDL_RenderCopyEx(playfieldRenderer_,
                             videoPlayer->getTexture(),
                             nullptr,
                             &playfieldRect,
                             settings.playfieldRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Playfield Renderer: Playfield video player has no texture");
        }
    } else if (auto* texture = assets.getPlayfieldTexture()) {
        SDL_RenderCopyEx(playfieldRenderer_,
                         texture,
                         nullptr,
                         &playfieldRect,
                         settings.playfieldRotation,
                         nullptr,
                         SDL_FLIP_NONE);
    } else {
        LOG_DEBUG("Playfield Renderer: No playfield video or texture available");
    }
    
    // Show wheel on playfield window
    if (settings.showWheel && settings.wheelWindow == "playfield") {
        if (auto* texture = assets.getWheelTexture(playfieldRenderer_)) {
            SDL_RenderCopyEx(playfieldRenderer_,
                             texture,
                             nullptr,
                             &wheelRect,
                             settings.playfieldRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Playfield Renderer: No wheel texture available");
        }
    }

    // Show title on playfield window
    if (settings.showTitle && settings.titleWindow == "playfield") {
        if (auto* texture = assets.getTitleTexture(playfieldRenderer_)) {
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
        } else {
            LOG_DEBUG("Playfield Renderer: No title texture available");
        }
    }
}

void Renderer::renderBackglassWindow(IAssetManager& assets) {
    if (!backglassRenderer_) {
        // LOG_ERROR("Backglass Renderer: Backglass renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
    // LOG_DEBUG("Backglass Renderer: showWheel=" << settings.showWheel
    //           << ", wheelWindow=" << settings.wheelWindow
    //           << ", showTitle=" << settings.showTitle
    //           << ", titleWindow=" << settings.titleWindow);
    if (!settings.showBackglass) {
        // LOG_DEBUG("Backglass Renderer: showBackglass is false");
        return;
    }
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(backglassRenderer_, &windowWidth, &windowHeight);

    SDL_Rect backglassRect = {settings.backglassMediaX, settings.backglassMediaY, settings.backglassMediaWidth, settings.backglassMediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX, settings.wheelMediaY, settings.wheelMediaWidth, settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect();
    if (auto* videoPlayer = assets.getBackglassVideoPlayer()) {
        if (videoPlayer->getTexture()) {
            videoPlayer->update();
            SDL_RenderCopyEx(backglassRenderer_,
                             videoPlayer->getTexture(),
                             nullptr,
                             &backglassRect,
                             settings.backglassRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Backglass Renderer: Backglass video player has no texture");
        }
    } else if (auto* texture = assets.getBackglassTexture()) {
        SDL_RenderCopyEx(backglassRenderer_,
                         texture,
                         nullptr,
                         &backglassRect,
                         settings.backglassRotation, // Fixed: Use backglassRotation
                         nullptr,
                         SDL_FLIP_NONE);
    } else {
        LOG_DEBUG("Backglass Renderer: No backglass video or texture available");
    }

    // Show wheel on backglass window
    if (settings.showWheel && settings.wheelWindow == "backglass") {
        if (auto* texture = assets.getWheelTexture(backglassRenderer_)) {
            SDL_RenderCopyEx(backglassRenderer_,
                             texture,
                             nullptr,
                             &wheelRect,
                             settings.backglassRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Backglass Renderer: No wheel texture available");
        }
    }

    // Show title on backglass window
    if (settings.showTitle && settings.titleWindow == "backglass") {
        if (auto* texture = assets.getTitleTexture(backglassRenderer_)) {
            SDL_SetRenderDrawColor(backglassRenderer_,
                                   settings.fontBgColor.r,
                                   settings.fontBgColor.g,
                                   settings.fontBgColor.b,
                                   settings.fontBgColor.a);
            SDL_Rect titleBgRect = {titleRect.x - 5, titleRect.y - 5, titleRect.w + 10, titleRect.h + 10};
            SDL_RenderFillRect(backglassRenderer_, &titleBgRect);
            SDL_RenderCopyEx(backglassRenderer_,
                             texture,
                             nullptr,
                             &titleRect,
                             settings.backglassRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Backglass Renderer: No title texture available");
        }
    }
}

void Renderer::renderDMDWindow(IAssetManager& assets) {
    if (!dmdRenderer_) {
        // LOG_ERROR("DMD Renderer: DMD renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
    // LOG_DEBUG("DMD Renderer: showWheel=" << settings.showWheel
    //           << ", wheelWindow=" << settings.wheelWindow
    //           << ", showTitle=" << settings.showTitle
    //           << ", titleWindow=" << settings.titleWindow);
    if (!settings.showDMD) {
        //LOG_DEBUG("DMD Renderer: showDMD is false");
        return;
    }
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(dmdRenderer_, &windowWidth, &windowHeight);

    SDL_Rect dmdRect = {settings.dmdMediaX, settings.dmdMediaY, settings.dmdMediaWidth, settings.dmdMediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX, settings.wheelMediaY, settings.wheelMediaWidth, settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect();
    if (auto* videoPlayer = assets.getDmdVideoPlayer()) {
        if (videoPlayer->getTexture()) {
            videoPlayer->update();
            SDL_RenderCopyEx(dmdRenderer_,
                             videoPlayer->getTexture(),
                             nullptr,
                             &dmdRect,
                             settings.dmdRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("DMD Renderer: DMD video player has no texture");
        }
    } else if (auto* texture = assets.getDmdTexture()) {
        SDL_RenderCopyEx(dmdRenderer_,
                         texture,
                         nullptr,
                         &dmdRect,
                         settings.dmdRotation,
                         nullptr,
                         SDL_FLIP_NONE);
    } else {
        LOG_DEBUG("DMD Renderer: No DMD video or texture available");
    }

    // Show wheel on dmd window
    if (settings.showWheel && settings.wheelWindow == "dmd") {
        if (auto* texture = assets.getWheelTexture(dmdRenderer_)) {
            SDL_RenderCopyEx(dmdRenderer_,
                             texture,
                             nullptr,
                             &wheelRect,
                             settings.dmdRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("DMD Renderer: No wheel texture available");
        }
    }

    // Show title on dmd window
    if (settings.showTitle && settings.titleWindow == "dmd") {
        if (auto* texture = assets.getTitleTexture(dmdRenderer_)) {
            SDL_SetRenderDrawColor(dmdRenderer_,
                                   settings.fontBgColor.r,
                                   settings.fontBgColor.g,
                                   settings.fontBgColor.b,
                                   settings.fontBgColor.a);
            SDL_Rect titleBgRect = {titleRect.x - 5, titleRect.y - 5, titleRect.w + 10, titleRect.h + 10};
            SDL_RenderFillRect(dmdRenderer_, &titleBgRect);
            SDL_RenderCopyEx(dmdRenderer_,
                             texture,
                             nullptr,
                             &titleRect,
                             settings.dmdRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("DMD Renderer: No title texture available");
        }
    }
}

void Renderer::renderTopperWindow(IAssetManager& assets) {
    if (!topperRenderer_) {
        //LOG_ERROR("Topper Renderer: Topper renderer is null");
        return;
    }
    const Settings& settings = assets.getSettingsManager()->getSettings();
    // LOG_DEBUG("Topper Renderer: showWheel=" << settings.showWheel
    //           << ", wheelWindow=" << settings.wheelWindow
    //           << ", showTitle=" << settings.showTitle
    //           << ", titleWindow=" << settings.titleWindow);
    if (!settings.showTopper) {
        //LOG_DEBUG("Topper Renderer: showTopper is false");
        return;
    }
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(topperRenderer_, &windowWidth, &windowHeight);

    SDL_Rect topperRect = {settings.topperMediaX, settings.topperMediaY, settings.topperMediaWidth, settings.topperMediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX, settings.wheelMediaY, settings.wheelMediaWidth, settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect();
    if (auto* videoPlayer = assets.getTopperVideoPlayer()) {
        if (videoPlayer->getTexture()) {
            videoPlayer->update();
            SDL_RenderCopyEx(topperRenderer_,
                             videoPlayer->getTexture(),
                             nullptr,
                             &topperRect,
                             settings.topperRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Topper Renderer: Topper video player has no texture");
        }
    } else if (auto* texture = assets.getTopperTexture()) {
        SDL_RenderCopyEx(topperRenderer_,
                         texture,
                         nullptr,
                         &topperRect,
                         settings.topperRotation,
                         nullptr,
                         SDL_FLIP_NONE);
    } else {
        LOG_DEBUG("Topper Renderer: No Topper video or texture available");
    }

    // Show wheel on topper window
    if (settings.showWheel && settings.wheelWindow == "topper") {
        if (auto* texture = assets.getWheelTexture(topperRenderer_)) {
            SDL_RenderCopyEx(topperRenderer_,
                             texture,
                             nullptr,
                             &wheelRect,
                             settings.topperRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Topper Renderer: No wheel texture available");
        }
    }

    // Show title on topper window
    if (settings.showTitle && settings.titleWindow == "topper") {
        if (auto* texture = assets.getTitleTexture(topperRenderer_)) {
            SDL_SetRenderDrawColor(topperRenderer_,
                                   settings.fontBgColor.r,
                                   settings.fontBgColor.g,
                                   settings.fontBgColor.b,
                                   settings.fontBgColor.a);
            SDL_Rect titleBgRect = {titleRect.x - 5, titleRect.y - 5, titleRect.w + 10, titleRect.h + 10};
            SDL_RenderFillRect(topperRenderer_, &titleBgRect);
            SDL_RenderCopyEx(topperRenderer_,
                             texture,
                             nullptr,
                             &titleRect,
                             settings.topperRotation,
                             nullptr,
                             SDL_FLIP_NONE);
        } else {
            LOG_DEBUG("Topper Renderer: No title texture available");
        }
    }
}