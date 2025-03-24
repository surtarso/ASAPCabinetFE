#ifndef TRANSITION_MANAGER_H
#define TRANSITION_MANAGER_H

#include <SDL.h>
#include <SDL_mixer.h>
#include "render/video_player.h"
#include "table/asset_manager.h"
#include "config/config_loader.h"
#include <functional>

enum class TransitionState { IDLE, FADING_OUT, FADING_IN };

class TransitionManager {
public:
    TransitionManager();
    ~TransitionManager(); // Added destructor to clean up textures
    void startTransition(VideoContext* tableVideoPlayer, 
                        VideoContext* backglassVideoPlayer, 
                        VideoContext* dmdVideoPlayer, 
                        Mix_Chunk* tableChangeSound,
                        SDL_Renderer* primaryRenderer,
                        SDL_Renderer* secondaryRenderer); // Added renderers for frame capture
    void updateTransition(Uint32 currentTime, AssetManager& assets);
    void loadNewContent(std::function<void()> loadCallback);
    bool isTransitionActive() const;
    bool shouldMaskFrame() const;

    // Accessors for rendering
    TransitionState getState() const { return state; }
    SDL_Texture* getCapturedTableFrame() const { return capturedTableFrame; }
    SDL_Texture* getCapturedBackglassFrame() const { return capturedBackglassFrame; }
    SDL_Texture* getCapturedDmdFrame() const { return capturedDmdFrame; }

private:
    TransitionState state;
    Uint32 startTime;
    Uint8 currentAlpha;
    bool loadPending;
    bool maskFrame;

    // Captured frames for smooth transitions
    SDL_Texture* capturedTableFrame;
    SDL_Texture* capturedBackglassFrame;
    SDL_Texture* capturedDmdFrame;

    // Renderers for frame capture
    SDL_Renderer* primaryRenderer;
    SDL_Renderer* secondaryRenderer;

    // Helper to capture a frame from a video player
    SDL_Texture* captureFrame(VideoContext* videoPlayer, SDL_Renderer* renderer);
};

#endif // TRANSITION_MANAGER_H