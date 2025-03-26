#include "render/renderer.h"
#include "utils/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "config/config_manager.h"

Renderer::Renderer(SDL_Renderer *primaryRenderer, SDL_Renderer *secondaryRenderer)
    : primaryRenderer_(primaryRenderer), secondaryRenderer_(secondaryRenderer) {}

void Renderer::render(AssetManager &assets, TransitionManager &transitionManager,
                      bool showConfig, IniEditor &configEditor)
{
    renderPrimaryWindow(assets, transitionManager, showConfig, configEditor);
    renderSecondaryWindow(assets, transitionManager);
}

void Renderer::renderPrimaryWindow(AssetManager &assets, TransitionManager &transitionManager,
                                   bool showConfig, IniEditor &configEditor)
{
    LOG_DEBUG("Rendering primary window, showConfig: " << (showConfig ? 1 : 0));
    const Settings &settings = assets.getConfigManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(primaryRenderer_, &windowWidth, &windowHeight);

    SDL_Rect tableRect = {0, 0, windowWidth, windowHeight}; // Full window
    SDL_Rect wheelRect = {0, 0, settings.wheelImageSize, settings.wheelImageSize};
    SDL_Rect nameRect = assets.tableNameRect;

    if (transitionManager.isTransitionActive()) {
        // Draw old table
        if (assets.tableVideoPlayer && assets.tableVideoPlayer->texture) {
            updateVideoTexture(assets.tableVideoPlayer);
            SDL_RenderCopy(primaryRenderer_, assets.tableVideoPlayer->texture, nullptr, &tableRect);
        } else if (assets.tableTexture) {
            SDL_RenderCopy(primaryRenderer_, assets.tableTexture.get(), nullptr, &tableRect);
        }
        // Draw new table
        if (transitionManager.getNewTableVideo() && transitionManager.getNewTableVideo()->texture) {
            updateVideoTexture(transitionManager.getNewTableVideo());
            SDL_RenderCopy(primaryRenderer_, transitionManager.getNewTableVideo()->texture, nullptr, &tableRect);
        } else if (transitionManager.getNewTableTexture()) {
            SDL_RenderCopy(primaryRenderer_, transitionManager.getNewTableTexture(), nullptr, &tableRect);
        }
        // Draw old wheel
        if (assets.wheelTexture) {
            wheelRect.x = windowWidth - wheelRect.w - settings.wheelImageMargin; // Bottom-right
            wheelRect.y = windowHeight - wheelRect.h - settings.wheelImageMargin;
            SDL_RenderCopy(primaryRenderer_, assets.wheelTexture.get(), nullptr, &wheelRect);
        }
        // Draw new wheel
        if (transitionManager.getNewWheelTexture()) {
            wheelRect.x = windowWidth - wheelRect.w - settings.wheelImageMargin;
            wheelRect.y = windowHeight - wheelRect.h - settings.wheelImageMargin;
            SDL_RenderCopy(primaryRenderer_, transitionManager.getNewWheelTexture(), nullptr, &wheelRect);
        }
        // Draw old name
        if (assets.tableNameTexture) {
            nameRect.x = 10;
            nameRect.y = windowHeight - nameRect.h - 10;
            SDL_SetRenderDrawColor(primaryRenderer_, 0, 0, 0, 128);
            SDL_Rect bgRect = {nameRect.x - 5, nameRect.y - 5, nameRect.w + 10, nameRect.h + 10};
            SDL_RenderFillRect(primaryRenderer_, &bgRect);
            SDL_RenderCopy(primaryRenderer_, assets.tableNameTexture.get(), nullptr, &nameRect);
        }
        // Draw new name
        if (transitionManager.getNewTableNameTexture()) {
            SDL_Rect newNameRect = transitionManager.getNewTableNameRect();
            newNameRect.x = 10;
            newNameRect.y = windowHeight - newNameRect.h - 10;
            SDL_SetRenderDrawColor(primaryRenderer_, 0, 0, 0, 128);
            SDL_Rect bgRect = {newNameRect.x - 5, newNameRect.y - 5, newNameRect.w + 10, newNameRect.h + 10};
            SDL_RenderFillRect(primaryRenderer_, &bgRect);
            SDL_RenderCopy(primaryRenderer_, transitionManager.getNewTableNameTexture(), nullptr, &newNameRect);
        }
    } else {
        // Normal render
        if (assets.tableVideoPlayer && assets.tableVideoPlayer->texture) {
            updateVideoTexture(assets.tableVideoPlayer);
            SDL_RenderCopy(primaryRenderer_, assets.tableVideoPlayer->texture, nullptr, &tableRect);
        } else if (assets.tableTexture) {
            SDL_RenderCopy(primaryRenderer_, assets.tableTexture.get(), nullptr, &tableRect);
        }
        if (assets.wheelTexture) {
            wheelRect.x = windowWidth - wheelRect.w - settings.wheelImageMargin; // Bottom-right
            wheelRect.y = windowHeight - wheelRect.h - settings.wheelImageMargin;
            SDL_RenderCopy(primaryRenderer_, assets.wheelTexture.get(), nullptr, &wheelRect);
        }
        if (assets.tableNameTexture) {
            nameRect.x = 10;
            nameRect.y = windowHeight - nameRect.h - 10;
            SDL_SetRenderDrawColor(primaryRenderer_, 0, 0, 0, 128);
            SDL_Rect bgRect = {nameRect.x - 5, nameRect.y - 5, nameRect.w + 10, nameRect.h + 10};
            SDL_RenderFillRect(primaryRenderer_, &bgRect);
            SDL_RenderCopy(primaryRenderer_, assets.tableNameTexture.get(), nullptr, &nameRect);
        }
    }
}

void Renderer::renderSecondaryWindow(AssetManager &assets, TransitionManager &transitionManager)
{
    const Settings &settings = assets.getConfigManager()->getSettings();
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(secondaryRenderer_, &windowWidth, &windowHeight);

    SDL_Rect backglassRect = {0, 0, windowWidth, settings.backglassMediaHeight};
    SDL_Rect dmdRect = {0, backglassRect.h, windowWidth, settings.dmdMediaHeight};

    if (transitionManager.isTransitionActive()) {
        // Draw old backglass
        if (assets.backglassVideoPlayer && assets.backglassVideoPlayer->texture) {
            updateVideoTexture(assets.backglassVideoPlayer);
            SDL_RenderCopy(secondaryRenderer_, assets.backglassVideoPlayer->texture, nullptr, &backglassRect);
        } else if (assets.backglassTexture) {
            SDL_RenderCopy(secondaryRenderer_, assets.backglassTexture.get(), nullptr, &backglassRect);
        }
        // Draw new backglass
        if (transitionManager.getNewBackglassVideo() && transitionManager.getNewBackglassVideo()->texture) {
            updateVideoTexture(transitionManager.getNewBackglassVideo());
            SDL_RenderCopy(secondaryRenderer_, transitionManager.getNewBackglassVideo()->texture, nullptr, &backglassRect);
        } else if (transitionManager.getNewBackglassTexture()) {
            SDL_RenderCopy(secondaryRenderer_, transitionManager.getNewBackglassTexture(), nullptr, &backglassRect);
        }
        // Draw old DMD
        if (assets.dmdVideoPlayer && assets.dmdVideoPlayer->texture) {
            updateVideoTexture(assets.dmdVideoPlayer);
            SDL_RenderCopy(secondaryRenderer_, assets.dmdVideoPlayer->texture, nullptr, &dmdRect);
        } else if (assets.dmdTexture) {
            SDL_RenderCopy(secondaryRenderer_, assets.dmdTexture.get(), nullptr, &dmdRect);
        }
        // Draw new DMD
        if (transitionManager.getNewDmdVideo() && transitionManager.getNewDmdVideo()->texture) {
            updateVideoTexture(transitionManager.getNewDmdVideo());
            SDL_RenderCopy(secondaryRenderer_, transitionManager.getNewDmdVideo()->texture, nullptr, &dmdRect);
        } else if (transitionManager.getNewDmdTexture()) {
            SDL_RenderCopy(secondaryRenderer_, transitionManager.getNewDmdTexture(), nullptr, &dmdRect);
        }
    } else {
        // Normal render
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
}