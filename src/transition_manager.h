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
    void startTransition(VideoContext* tableVideoPlayer, 
                        VideoContext* backglassVideoPlayer, 
                        VideoContext* dmdVideoPlayer, 
                        Mix_Chunk* tableChangeSound);
    void updateTransition(Uint32 currentTime, AssetManager& assets);
    void loadNewContent(std::function<void()> loadCallback);
    bool isTransitionActive() const;
    bool shouldMaskFrame() const;

private:
    TransitionState state;
    Uint32 startTime;
    Uint8 currentAlpha;
    bool loadPending;
    bool maskFrame;
};

#endif // TRANSITION_MANAGER_H