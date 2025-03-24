#include "transition_manager.h"
#include <algorithm>

// Constructor: Initializes transition state to idle with default values.
TransitionManager::TransitionManager() 
    : state(TransitionState::IDLE), startTime(0), currentAlpha(255), loadPending(false), maskFrame(false) {}

// Starts a transition by stopping video players, playing a sound, and initiating fade-out.
void TransitionManager::startTransition(VideoContext* tableVideoPlayer, 
                                       VideoContext* backglassVideoPlayer, 
                                       VideoContext* dmdVideoPlayer, 
                                       Mix_Chunk* tableChangeSound) {
    if (tableVideoPlayer) libvlc_media_player_stop(tableVideoPlayer->player);
    if (backglassVideoPlayer) libvlc_media_player_stop(backglassVideoPlayer->player);
    if (dmdVideoPlayer) libvlc_media_player_stop(dmdVideoPlayer->player);
    if (tableChangeSound) Mix_PlayChannel(-1, tableChangeSound, 0);
    state = TransitionState::FADING_OUT;
    startTime = SDL_GetTicks();
    currentAlpha = 255;
    loadPending = true;
    maskFrame = false;  // Reset mask for new transition.
}

// Updates the transition state, adjusting alpha for fade effects and managing asset visibility.
void TransitionManager::updateTransition(Uint32 currentTime, AssetManager& assets) {
    if (state == TransitionState::IDLE) {
        assets.clearOldVideoPlayers();
        return;
    }

    Uint32 elapsed = currentTime - startTime;
    int halfDuration = FADE_DURATION_MS / 2;

    // Handle fade-out: Decrease alpha until reaching target.
    if (state == TransitionState::FADING_OUT) {
        if (elapsed < (Uint32)halfDuration) {
            currentAlpha = 255 - (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
        } else {
            state = TransitionState::FADING_IN;
            startTime = currentTime;
            currentAlpha = FADE_TARGET_ALPHA;
            loadPending = true;
            maskFrame = true;  // Mask the next frame to avoid flicker.
        }
    }
    // Handle fade-in: Increase alpha back to full visibility.
    else if (state == TransitionState::FADING_IN) {
        if (elapsed < (Uint32)halfDuration) {
            currentAlpha = FADE_TARGET_ALPHA + (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
        } else {
            currentAlpha = 255;
            state = TransitionState::IDLE;
            loadPending = false;
            maskFrame = false;
        }
    }

    // Apply alpha to all textures and videos for smooth fading.
    if (assets.getTableTexture()) SDL_SetTextureAlphaMod(assets.getTableTexture(), currentAlpha);
    if (assets.getWheelTexture()) SDL_SetTextureAlphaMod(assets.getWheelTexture(), currentAlpha);
    if (assets.getBackglassTexture()) SDL_SetTextureAlphaMod(assets.getBackglassTexture(), currentAlpha);
    if (assets.getDmdTexture()) SDL_SetTextureAlphaMod(assets.getDmdTexture(), currentAlpha);
    if (assets.getTableNameTexture()) SDL_SetTextureAlphaMod(assets.getTableNameTexture(), currentAlpha);
    VideoContext* tableVid = assets.getTableVideoPlayer();
    VideoContext* backglassVid = assets.getBackglassVideoPlayer();
    VideoContext* dmdVid = assets.getDmdVideoPlayer();
    if (tableVid && tableVid->texture) SDL_SetTextureAlphaMod(tableVid->texture, currentAlpha);
    if (backglassVid && backglassVid->texture) SDL_SetTextureAlphaMod(backglassVid->texture, currentAlpha);
    if (dmdVid && dmdVid->texture) SDL_SetTextureAlphaMod(dmdVid->texture, currentAlpha);
}

// Loads new content at the midpoint of the transition (during fade-in).
void TransitionManager::loadNewContent(std::function<void()> loadCallback) {
    if (loadPending && state == TransitionState::FADING_IN && currentAlpha == FADE_TARGET_ALPHA) {
        loadCallback();
        loadPending = false;
    }
}

// Checks if a transition is currently active.
bool TransitionManager::isTransitionActive() const {
    return state != TransitionState::IDLE;
}

// Determines if the current frame should be masked (e.g., to avoid flicker).
bool TransitionManager::shouldMaskFrame() const {
    return maskFrame;
}