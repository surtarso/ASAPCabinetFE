#include "render/renderer.h"
#include "log/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h"

Renderer::Renderer(SDL_Renderer* playfieldRenderer, SDL_Renderer* backglassRenderer, 
                   SDL_Renderer* dmdRenderer, SDL_Renderer* topperRenderer)
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
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, playfieldRenderer_, "playfield", true,
                 settings.playfieldMediaX, settings.playfieldMediaY,
                 settings.playfieldMediaWidth, settings.playfieldMediaHeight,
                 settings.playfieldRotation);
    renderWindow(assets, backglassRenderer_, "backglass", settings.showBackglass,
                 settings.backglassMediaX, settings.backglassMediaY,
                 settings.backglassMediaWidth, settings.backglassMediaHeight,
                 settings.backglassRotation);
    renderWindow(assets, dmdRenderer_, "dmd", settings.showDMD,
                 settings.dmdMediaX, settings.dmdMediaY,
                 settings.dmdMediaWidth, settings.dmdMediaHeight,
                 settings.dmdRotation);
    renderWindow(assets, topperRenderer_, "topper", settings.showTopper,
                 settings.topperMediaX, settings.topperMediaY,
                 settings.topperMediaWidth, settings.topperMediaHeight,
                 settings.topperRotation);
}

void Renderer::renderWindow(IAssetManager& assets, SDL_Renderer* renderer, const std::string& windowName,
                            bool isVisible, int mediaX, int mediaY, int mediaWidth, int mediaHeight,
                            double rotation) {
    if (!renderer) {
        //LOG_ERROR(windowName << " Renderer: Renderer is null");
        return;
    }
    if (!isVisible) {
        // LOG_DEBUG(windowName << " Renderer: Window is not visible");
        return;
    }
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(renderer, &windowWidth, &windowHeight);

    SDL_Rect mediaRect = {mediaX, mediaY, mediaWidth, mediaHeight};
    SDL_Rect wheelRect = {assets.getSettingsManager()->getSettings().wheelMediaX,
                          assets.getSettingsManager()->getSettings().wheelMediaY,
                          assets.getSettingsManager()->getSettings().wheelMediaWidth,
                          assets.getSettingsManager()->getSettings().wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect();

    // Get video player or texture based on window name
    IVideoPlayer* videoPlayer = nullptr;
    SDL_Texture* texture = nullptr;
    if (windowName == "playfield") {
        videoPlayer = assets.getPlayfieldVideoPlayer();
        texture = assets.getPlayfieldTexture();
    } else if (windowName == "backglass") {
        videoPlayer = assets.getBackglassVideoPlayer();
        texture = assets.getBackglassTexture();
    } else if (windowName == "dmd") {
        videoPlayer = assets.getDmdVideoPlayer();
        texture = assets.getDmdTexture();
    } else if (windowName == "topper") {
        videoPlayer = assets.getTopperVideoPlayer();
        texture = assets.getTopperTexture();
    }

    if (videoPlayer && videoPlayer->getTexture()) {
        videoPlayer->update();
        SDL_RenderCopyEx(renderer, videoPlayer->getTexture(), nullptr, &mediaRect,
                         rotation, nullptr, SDL_FLIP_NONE);
    } else if (texture) {
        SDL_RenderCopyEx(renderer, texture, nullptr, &mediaRect,
                         rotation, nullptr, SDL_FLIP_NONE);
    } else {
        LOG_DEBUG(windowName << " Renderer: No video or texture available");
    }

    const Settings& settings = assets.getSettingsManager()->getSettings();
    // Show wheel
    if (settings.showWheel && settings.wheelWindow == windowName) {
        if (auto* wheelTexture = assets.getWheelTexture(renderer)) {
            SDL_RenderCopyEx(renderer, wheelTexture, nullptr, &wheelRect,
                             rotation, nullptr, SDL_FLIP_NONE);
        } else {
            LOG_DEBUG(windowName << " Renderer: No wheel texture available");
        }
    }

    // Show title
    if (settings.showTitle && settings.titleWindow == windowName) {
        if (auto* titleTexture = assets.getTitleTexture(renderer)) {
            SDL_SetRenderDrawColor(renderer,
                                   settings.fontBgColor.r,
                                   settings.fontBgColor.g,
                                   settings.fontBgColor.b,
                                   settings.fontBgColor.a);
            SDL_Rect titleBgRect = {titleRect.x - 5, titleRect.y - 5, titleRect.w + 10, titleRect.h + 10};
            SDL_RenderFillRect(renderer, &titleBgRect);
            SDL_RenderCopyEx(renderer, titleTexture, nullptr, &titleRect,
                             rotation, nullptr, SDL_FLIP_NONE);
        } else {
            LOG_DEBUG(windowName << " Renderer: No title texture available");
        }
    }
}

void Renderer::renderPlayfieldWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, playfieldRenderer_, "playfield", true,
                 settings.playfieldMediaX, settings.playfieldMediaY,
                 settings.playfieldMediaWidth, settings.playfieldMediaHeight,
                 settings.playfieldRotation);
}

void Renderer::renderBackglassWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, backglassRenderer_, "backglass", settings.showBackglass,
                 settings.backglassMediaX, settings.backglassMediaY,
                 settings.backglassMediaWidth, settings.backglassMediaHeight,
                 settings.backglassRotation);
}

void Renderer::renderDMDWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, dmdRenderer_, "dmd", settings.showDMD,
                 settings.dmdMediaX, settings.dmdMediaY,
                 settings.dmdMediaWidth, settings.dmdMediaHeight,
                 settings.dmdRotation);
}

void Renderer::renderTopperWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, topperRenderer_, "topper", settings.showTopper,
                 settings.topperMediaX, settings.topperMediaY,
                 settings.topperMediaWidth, settings.topperMediaHeight,
                 settings.topperRotation);
}