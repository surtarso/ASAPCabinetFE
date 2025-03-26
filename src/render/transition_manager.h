#ifndef TRANSITION_MANAGER_H
#define TRANSITION_MANAGER_H

#include <SDL.h>
#include <SDL_mixer.h>
#include <functional>
#include "render/video_player.h"
#include "table/table_manager.h"
#include <memory>

class AssetManager;

class TransitionManager {
public:
    TransitionManager();
    void startTransition(VideoContext* tableVideo, VideoContext* backglassVideo,
                         VideoContext* dmdVideo, Mix_Chunk* tableSound,
                         SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer,
                         AssetManager& assets, size_t newIndex, const std::vector<Table>& tables);
    void updateTransition(Uint32 currentTime, AssetManager& assets);
    void loadNewContent(std::function<void()> loadFunction);
    bool isTransitionActive() const;
    Uint32 duration() const { return duration_; }

    SDL_Texture* getNewTableTexture() const { return newTableTexture_.get(); }
    SDL_Texture* getNewWheelTexture() const { return newWheelTexture_.get(); }
    SDL_Texture* getNewBackglassTexture() const { return newBackglassTexture_.get(); }
    SDL_Texture* getNewDmdTexture() const { return newDmdTexture_.get(); }
    SDL_Texture* getNewTableNameTexture() const { return newTableNameTexture_.get(); }
    SDL_Rect getNewTableNameRect() const { return newTableNameRect_; }
    VideoContext* getNewTableVideo() const { return newTableVideo_; }
    VideoContext* getNewBackglassVideo() const { return newBackglassVideo_; }
    VideoContext* getNewDmdVideo() const { return newDmdVideo_; }

private:
    bool transitionActive_;
    Uint32 startTime_;
    Uint32 duration_;
    VideoContext* oldTableVideo_;
    VideoContext* oldBackglassVideo_;
    VideoContext* oldDmdVideo_;
    Mix_Chunk* tableSound_;
    SDL_Renderer* primaryRenderer_;
    SDL_Renderer* secondaryRenderer_;
    std::function<void()> loadFunction_;

    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> newTableTexture_;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> newWheelTexture_;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> newBackglassTexture_;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> newDmdTexture_;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> newTableNameTexture_;
    SDL_Rect newTableNameRect_;
    VideoContext* newTableVideo_;
    VideoContext* newBackglassVideo_;
    VideoContext* newDmdVideo_;
};

#endif // TRANSITION_MANAGER_H