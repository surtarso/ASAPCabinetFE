#include "render/transition_manager.h"
#include <algorithm>

// Constructor: Initialize with no captured frames
TransitionManager::TransitionManager() 
    : state(TransitionState::IDLE), startTime(0), currentAlpha(255), 
      loadPending(false), maskFrame(false), 
      capturedTableFrame(nullptr), capturedBackglassFrame(nullptr), capturedDmdFrame(nullptr),
      primaryRenderer(nullptr), secondaryRenderer(nullptr) {}

// Destructor: Clean up captured textures
TransitionManager::~TransitionManager() {
    if (capturedTableFrame) SDL_DestroyTexture(capturedTableFrame);
    if (capturedBackglassFrame) SDL_DestroyTexture(capturedBackglassFrame);
    if (capturedDmdFrame) SDL_DestroyTexture(capturedDmdFrame);
}

// Capture the current frame from a video player
SDL_Texture* TransitionManager::captureFrame(VideoContext* videoPlayer, SDL_Renderer* renderer) {
    if (!videoPlayer || !videoPlayer->texture || !renderer) return nullptr;
    SDL_Texture* frame = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                           SDL_TEXTUREACCESS_TARGET, 
                                           videoPlayer->width, videoPlayer->height);
    if (frame) {
        SDL_SetTextureBlendMode(frame, SDL_BLENDMODE_BLEND); // Ensure transparency support
        SDL_SetRenderTarget(renderer, frame);
        SDL_RenderCopy(renderer, videoPlayer->texture, nullptr, nullptr);
        SDL_SetRenderTarget(renderer, nullptr);
    }
    return frame;
}

// Start the transition by capturing frames and initiating fade-out
void TransitionManager::startTransition(VideoContext* tableVideoPlayer, 
                                       VideoContext* backglassVideoPlayer, 
                                       VideoContext* dmdVideoPlayer, 
                                       Mix_Chunk* tableChangeSound,
                                       SDL_Renderer* primaryRenderer,
                                       SDL_Renderer* secondaryRenderer) {
    this->primaryRenderer = primaryRenderer;
    this->secondaryRenderer = secondaryRenderer;

    // Capture last frames before stopping players
    capturedTableFrame = captureFrame(tableVideoPlayer, primaryRenderer);
    capturedBackglassFrame = captureFrame(backglassVideoPlayer, secondaryRenderer);
    capturedDmdFrame = captureFrame(dmdVideoPlayer, secondaryRenderer);

    // Stop video players
    if (tableVideoPlayer) libvlc_media_player_stop(tableVideoPlayer->player);
    if (backglassVideoPlayer) libvlc_media_player_stop(backglassVideoPlayer->player);
    if (dmdVideoPlayer) libvlc_media_player_stop(dmdVideoPlayer->player);

    if (tableChangeSound) Mix_PlayChannel(-1, tableChangeSound, 0);
    state = TransitionState::FADING_OUT;
    startTime = SDL_GetTicks();
    currentAlpha = 255;
    loadPending = true;
    maskFrame = false;
}

// Update the transition, managing alpha and content loading
void TransitionManager::updateTransition(Uint32 currentTime, AssetManager& assets) {
    if (state == TransitionState::IDLE) {
        assets.clearOldVideoPlayers();
        // Clean up captured frames
        if (capturedTableFrame) SDL_DestroyTexture(capturedTableFrame);
        if (capturedBackglassFrame) SDL_DestroyTexture(capturedBackglassFrame);
        if (capturedDmdFrame) SDL_DestroyTexture(capturedDmdFrame);
        capturedTableFrame = nullptr;
        capturedBackglassFrame = nullptr;
        capturedDmdFrame = nullptr;
        return;
    }

    Uint32 elapsed = currentTime - startTime;
    int halfDuration = FADE_DURATION_MS / 2;

    if (state == TransitionState::FADING_OUT) {
        if (elapsed < (Uint32)halfDuration) {
            currentAlpha = 255 - (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
            // Apply alpha to captured frames
            if (capturedTableFrame) SDL_SetTextureAlphaMod(capturedTableFrame, currentAlpha);
            if (capturedBackglassFrame) SDL_SetTextureAlphaMod(capturedBackglassFrame, currentAlpha);
            if (capturedDmdFrame) SDL_SetTextureAlphaMod(capturedDmdFrame, currentAlpha);
        } else {
            state = TransitionState::FADING_IN;
            startTime = currentTime;
            currentAlpha = FADE_TARGET_ALPHA;
            loadPending = true;
            maskFrame = true;
        }
    } else if (state == TransitionState::FADING_IN) {
        if (elapsed < (Uint32)halfDuration) {
            currentAlpha = FADE_TARGET_ALPHA + (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
            // Apply alpha to new content
            if (assets.getTableTexture()) SDL_SetTextureAlphaMod(assets.getTableTexture(), currentAlpha);
            if (assets.getWheelTexture()) SDL_SetTextureAlphaMod(assets.getWheelTexture(), currentAlpha);
            if (assets.getBackglassTexture()) SDL_SetTextureAlphaMod(assets.getBackglassTexture(), currentAlpha);
            if (assets.getDmdTexture()) SDL_SetTextureAlphaMod(assets.getDmdTexture(), currentAlpha);
            if (assets.getTableNameTexture()) SDL_SetTextureAlphaMod(assets.getTableNameTexture(), currentAlpha);
            VideoContext* tableVid = assets.getTableVideoPlayer();
            VideoContext* backglassVid = assets.getBackglassVideoPlayer();
            VideoContext* dmdVid = assets.getDmdVideoPlayer();
            if (tableVid && tableVid->texture) {
                SDL_SetTextureAlphaMod(tableVid->texture, currentAlpha);
                SDL_SetTextureBlendMode(tableVid->texture, SDL_BLENDMODE_BLEND);
            }
            if (backglassVid && backglassVid->texture) {
                SDL_SetTextureAlphaMod(backglassVid->texture, currentAlpha);
                SDL_SetTextureBlendMode(backglassVid->texture, SDL_BLENDMODE_BLEND);
            }
            if (dmdVid && dmdVid->texture) {
                SDL_SetTextureAlphaMod(dmdVid->texture, currentAlpha);
                SDL_SetTextureBlendMode(dmdVid->texture, SDL_BLENDMODE_BLEND);
            }
        } else {
            currentAlpha = 255;
            state = TransitionState::IDLE;
            loadPending = false;
            maskFrame = false;
        }
    }
}

// Load new content at the midpoint
void TransitionManager::loadNewContent(std::function<void()> loadCallback) {
    if (loadPending && state == TransitionState::FADING_IN && currentAlpha == FADE_TARGET_ALPHA) {
        loadCallback();
        loadPending = false;
    }
}

// Check if transition is active
bool TransitionManager::isTransitionActive() const {
    return state != TransitionState::IDLE;
}

// Check if frame should be masked
bool TransitionManager::shouldMaskFrame() const {
    return maskFrame;
}