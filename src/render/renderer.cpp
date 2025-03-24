#include "render/renderer.h"
#include "render/video_player.h"
#include "config/config_loader.h" // For constants like MAIN_WINDOW_WIDTH, FONT_BG_COLOR, etc.
#include "imgui.h"               // For ImGui::Render()
#include "imgui_impl_sdlrenderer2.h" // For ImGui_ImplSDLRenderer2_RenderDrawData

Renderer::Renderer(SDL_Renderer* primary, SDL_Renderer* secondary)
    : primaryRenderer(primary), secondaryRenderer(secondary) {}

void Renderer::render(AssetManager& assets, TransitionManager& transitionManager, bool showConfig, IniEditor& configEditor) {
    // Update video textures for all video players
    VideoContext* tableVideo = assets.getTableVideoPlayer();
    VideoContext* backglassVideo = assets.getBackglassVideoPlayer();
    VideoContext* dmdVideo = assets.getDmdVideoPlayer();

    if (!transitionManager.isTransitionActive()) {
        updateVideoTexture(tableVideo, primaryRenderer);
        updateVideoTexture(backglassVideo, secondaryRenderer);
        updateVideoTexture(dmdVideo, secondaryRenderer);
    }

    // Render primary window (playfield, wheel, table name)
    renderPrimaryWindow(assets, transitionManager, showConfig, configEditor);

    // Render secondary window (backglass, DMD)
    renderSecondaryWindow(assets, transitionManager);
}

void Renderer::renderPrimaryWindow(AssetManager& assets, TransitionManager& transitionManager, bool showConfig, IniEditor& configEditor) {
    SDL_SetRenderDrawColor(primaryRenderer, 32, 32, 32, 255);
    SDL_RenderClear(primaryRenderer);

    SDL_Rect tableRect = {0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT};
    if (transitionManager.shouldMaskFrame()) {
        SDL_SetRenderDrawColor(primaryRenderer, 0, 0, 0, 255);
        SDL_RenderClear(primaryRenderer);
    } else if (transitionManager.isTransitionActive()) {
        if (transitionManager.getState() == TransitionState::FADING_OUT && transitionManager.getCapturedTableFrame()) {
            SDL_RenderCopy(primaryRenderer, transitionManager.getCapturedTableFrame(), nullptr, &tableRect);
        } else if (transitionManager.getState() == TransitionState::FADING_IN) {
            VideoContext* tableVideo = assets.getTableVideoPlayer();
            if (tableVideo && tableVideo->texture) {
                SDL_RenderCopy(primaryRenderer, tableVideo->texture, nullptr, &tableRect);
            } else if (assets.getTableTexture()) {
                SDL_RenderCopy(primaryRenderer, assets.getTableTexture(), nullptr, &tableRect);
            }
        }
    } else {
        VideoContext* tableVideo = assets.getTableVideoPlayer();
        if (tableVideo && tableVideo->texture) {
            SDL_RenderCopy(primaryRenderer, tableVideo->texture, nullptr, &tableRect);
        } else if (assets.getTableTexture()) {
            SDL_RenderCopy(primaryRenderer, assets.getTableTexture(), nullptr, &tableRect);
        }
    }

    if (assets.getWheelTexture()) {
        SDL_Rect wheelRect = {MAIN_WINDOW_WIDTH - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN,
                              MAIN_WINDOW_HEIGHT - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN,
                              WHEEL_IMAGE_SIZE, WHEEL_IMAGE_SIZE};
        SDL_RenderCopy(primaryRenderer, assets.getWheelTexture(), nullptr, &wheelRect);
    }

    if (assets.getTableNameTexture()) {
        SDL_Rect nameRect = assets.getTableNameRect();
        nameRect.x = 10;
        nameRect.y = MAIN_WINDOW_HEIGHT - nameRect.h - 10;
        SDL_Rect bgRect = {nameRect.x - 5, nameRect.y - 5, nameRect.w + 10, nameRect.h + 10};
        SDL_SetRenderDrawBlendMode(primaryRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(primaryRenderer, FONT_BG_COLOR.r, FONT_BG_COLOR.g, FONT_BG_COLOR.b, FONT_BG_COLOR.a);
        SDL_RenderFillRect(primaryRenderer, &bgRect);
        SDL_RenderCopy(primaryRenderer, assets.getTableNameTexture(), nullptr, &nameRect);
    }

    // Render config GUI if open
    if (showConfig) configEditor.drawGUI();

    // Finalize ImGui rendering for primary window
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), primaryRenderer);
    SDL_RenderPresent(primaryRenderer);
}

void Renderer::renderSecondaryWindow(AssetManager& assets, TransitionManager& transitionManager) {
    SDL_SetRenderDrawColor(secondaryRenderer, 0, 0, 0, 255);
    SDL_RenderClear(secondaryRenderer);

    SDL_Rect backglassRect = {0, 0, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT};
    if (transitionManager.shouldMaskFrame()) {
        SDL_SetRenderDrawColor(secondaryRenderer, 0, 0, 0, 255);
        SDL_RenderClear(secondaryRenderer);
    } else if (transitionManager.isTransitionActive()) {
        if (transitionManager.getState() == TransitionState::FADING_OUT && transitionManager.getCapturedBackglassFrame()) {
            SDL_RenderCopy(secondaryRenderer, transitionManager.getCapturedBackglassFrame(), nullptr, &backglassRect);
        } else if (transitionManager.getState() == TransitionState::FADING_IN) {
            VideoContext* backglassVideo = assets.getBackglassVideoPlayer();
            if (backglassVideo && backglassVideo->texture) {
                SDL_RenderCopy(secondaryRenderer, backglassVideo->texture, nullptr, &backglassRect);
            } else if (assets.getBackglassTexture()) {
                SDL_RenderCopy(secondaryRenderer, assets.getBackglassTexture(), nullptr, &backglassRect);
            }
        }
    } else {
        VideoContext* backglassVideo = assets.getBackglassVideoPlayer();
        if (backglassVideo && backglassVideo->texture) {
            SDL_RenderCopy(secondaryRenderer, backglassVideo->texture, nullptr, &backglassRect);
        } else if (assets.getBackglassTexture()) {
            SDL_RenderCopy(secondaryRenderer, assets.getBackglassTexture(), nullptr, &backglassRect);
        }
    }

    SDL_Rect dmdRect = {0, BACKGLASS_MEDIA_HEIGHT, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT};
    if (transitionManager.shouldMaskFrame()) {
        SDL_SetRenderDrawColor(secondaryRenderer, 0, 0, 0, 255);
        SDL_RenderClear(secondaryRenderer);
    } else if (transitionManager.isTransitionActive()) {
        if (transitionManager.getState() == TransitionState::FADING_OUT && transitionManager.getCapturedDmdFrame()) {
            SDL_RenderCopy(secondaryRenderer, transitionManager.getCapturedDmdFrame(), nullptr, &dmdRect);
        } else if (transitionManager.getState() == TransitionState::FADING_IN) {
            VideoContext* dmdVideo = assets.getDmdVideoPlayer();
            if (dmdVideo && dmdVideo->texture) {
                SDL_RenderCopy(secondaryRenderer, dmdVideo->texture, nullptr, &dmdRect);
            } else if (assets.getDmdTexture()) {
                SDL_RenderCopy(secondaryRenderer, assets.getDmdTexture(), nullptr, &dmdRect);
            }
        }
    } else {
        VideoContext* dmdVideo = assets.getDmdVideoPlayer();
        if (dmdVideo && dmdVideo->texture) {
            SDL_RenderCopy(secondaryRenderer, dmdVideo->texture, nullptr, &dmdRect);
        } else if (assets.getDmdTexture()) {
            SDL_RenderCopy(secondaryRenderer, assets.getDmdTexture(), nullptr, &dmdRect);
        }
    }

    SDL_RenderPresent(secondaryRenderer);
}