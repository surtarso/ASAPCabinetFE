#include "renderer.h"
#include "log/logging.h"
#include "imgui.h" 
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_service.h" // Included for Settings struct definition

Renderer::Renderer(SDL_Renderer* playfieldRenderer, 
                   SDL_Renderer* backglassRenderer, 
                   SDL_Renderer* dmdRenderer, 
                   SDL_Renderer* topperRenderer)
    : playfieldRenderer_(playfieldRenderer),
      backglassRenderer_(backglassRenderer),
      dmdRenderer_(dmdRenderer),
      topperRenderer_(topperRenderer) {}

void Renderer::setRenderers(IWindowManager* windowManager) {
    playfieldRenderer_ = windowManager->getPlayfieldRenderer();
    backglassRenderer_ = windowManager->getBackglassRenderer();
    dmdRenderer_ = windowManager->getDMDRenderer();
    topperRenderer_ = windowManager->getTopperRenderer();
    LOG_DEBUG("Updated renderers - playfield=" + std::to_string(reinterpret_cast<uintptr_t>(playfieldRenderer_)) +
              ", backglass=" + std::to_string(reinterpret_cast<uintptr_t>(backglassRenderer_)) +
              ", dmd=" + std::to_string(reinterpret_cast<uintptr_t>(dmdRenderer_)) +
              ", topper=" + std::to_string(reinterpret_cast<uintptr_t>(topperRenderer_)));
}

void Renderer::render(IAssetManager& assets) {
    // Retrieve settings once for the entire frame's rendering
    const Settings& settings = assets.getSettingsManager()->getSettings();

    renderWindow(assets, playfieldRenderer_, "playfield", true,
                 settings.playfieldMediaX, settings.playfieldMediaY,
                 settings.playfieldMediaWidth, settings.playfieldMediaHeight,
                 settings.playfieldRotation, settings); // Pass settings
    renderWindow(assets, backglassRenderer_, "backglass", settings.showBackglass,
                 settings.backglassMediaX, settings.backglassMediaY,
                 settings.backglassMediaWidth, settings.backglassMediaHeight,
                 settings.backglassRotation, settings); // Pass settings
    renderWindow(assets, dmdRenderer_, "dmd", settings.showDMD,
                 settings.dmdMediaX, settings.dmdMediaY,
                 settings.dmdMediaWidth, settings.dmdMediaHeight,
                 settings.dmdRotation, settings); // Pass settings
    renderWindow(assets, topperRenderer_, "topper", settings.showTopper,
                 settings.topperMediaX, settings.topperMediaY,
                 settings.topperMediaWidth, settings.topperMediaHeight,
                 settings.topperRotation, settings); // Pass settings
}

void Renderer::renderWindow(IAssetManager& assets, SDL_Renderer* renderer, const std::string& windowName,
                            bool isVisible, int mediaX, int mediaY, int mediaWidth, int mediaHeight,
                            double rotation, const Settings& settings) { 
    if (!renderer || !isVisible) {
        return;
    }

    SDL_Rect mediaRect = {mediaX, mediaY, mediaWidth, mediaHeight};
    SDL_Rect wheelRect = {settings.wheelMediaX,
                          settings.wheelMediaY,
                          settings.wheelMediaWidth,
                          settings.wheelMediaHeight};
    SDL_Rect titleRect = assets.getTitleRect(); // TitleRect is managed by AssetManager

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
        videoPlayer->update(); // Update video frame
        if (rotation == 0.0) {
            SDL_RenderCopy(renderer, videoPlayer->getTexture(), nullptr, &mediaRect);
        } else {
            SDL_RenderCopyEx(renderer, videoPlayer->getTexture(), nullptr, &mediaRect,
                             rotation, nullptr, SDL_FLIP_NONE);
        }
    } else if (texture) {
        if (rotation == 0.0) {
            SDL_RenderCopy(renderer, texture, nullptr, &mediaRect);
        } else {
            SDL_RenderCopyEx(renderer, texture, nullptr, &mediaRect,
                             rotation, nullptr, SDL_FLIP_NONE);
        }
    }

    // Show wheel
    if (settings.showWheel && settings.wheelWindow == windowName) {
        if (auto* wheelTexture = assets.getWheelTexture(renderer)) {
            if (rotation == 0.0) {
                SDL_RenderCopy(renderer, wheelTexture, nullptr, &wheelRect);
            } else {
                SDL_RenderCopyEx(renderer, wheelTexture, nullptr, &wheelRect,
                                 rotation, nullptr, SDL_FLIP_NONE);
            }
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
            if (rotation == 0.0) {
                SDL_RenderCopy(renderer, titleTexture, nullptr, &titleRect);
            } else {
                SDL_RenderCopyEx(renderer, titleTexture, nullptr, &titleRect,
                                 rotation, nullptr, SDL_FLIP_NONE);
            }
        }
    }
}

// Individual render methods now pass the settings object to the unified renderWindow
void Renderer::renderPlayfieldWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, playfieldRenderer_, "playfield", true,
                 settings.playfieldMediaX, settings.playfieldMediaY,
                 settings.playfieldMediaWidth, settings.playfieldMediaHeight,
                 settings.playfieldRotation, settings);
}

void Renderer::renderBackglassWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, backglassRenderer_, "backglass", settings.showBackglass,
                 settings.backglassMediaX, settings.backglassMediaY,
                 settings.backglassMediaWidth, settings.backglassMediaHeight,
                 settings.backglassRotation, settings);
}

void Renderer::renderDMDWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, dmdRenderer_, "dmd", settings.showDMD,
                 settings.dmdMediaX, settings.dmdMediaY,
                 settings.dmdMediaWidth, settings.dmdMediaHeight,
                 settings.dmdRotation, settings);
}

void Renderer::renderTopperWindow(IAssetManager& assets) {
    const Settings& settings = assets.getSettingsManager()->getSettings();
    renderWindow(assets, topperRenderer_, "topper", settings.showTopper,
                 settings.topperMediaX, settings.topperMediaY,
                 settings.topperMediaWidth, settings.topperMediaHeight,
                 settings.topperRotation, settings);
}