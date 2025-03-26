#include "render/transition_manager.h"
#include "table/asset_manager.h"
#include "config/config_manager.h"
#include "utils/logging.h"

TransitionManager::TransitionManager()
    : transitionActive_(false), startTime_(0), duration_(0),
      oldTableVideo_(nullptr), oldBackglassVideo_(nullptr), oldDmdVideo_(nullptr), tableSound_(nullptr),
      primaryRenderer_(nullptr), secondaryRenderer_(nullptr),
      newTableTexture_(nullptr, SDL_DestroyTexture), newWheelTexture_(nullptr, SDL_DestroyTexture),
      newBackglassTexture_(nullptr, SDL_DestroyTexture), newDmdTexture_(nullptr, SDL_DestroyTexture),
      newTableNameTexture_(nullptr, SDL_DestroyTexture), newTableVideo_(nullptr),
      newBackglassVideo_(nullptr), newDmdVideo_(nullptr) {}

      void TransitionManager::startTransition(VideoContext* tableVideo, VideoContext* backglassVideo,
        VideoContext* dmdVideo, Mix_Chunk* tableSound,
        SDL_Renderer* primaryRenderer, SDL_Renderer* secondaryRenderer,
        AssetManager& assets, size_t newIndex, const std::vector<Table>& tables) {
        oldTableVideo_ = tableVideo;
        oldBackglassVideo_ = backglassVideo;
        oldDmdVideo_ = dmdVideo;
        tableSound_ = tableSound;
        primaryRenderer_ = primaryRenderer;
        secondaryRenderer_ = secondaryRenderer;

        const Table& newTable = tables[newIndex];
        const Settings& settings = assets.getConfigManager()->getSettings();
        newTableTexture_.reset(assets.loadTexture(primaryRenderer_, newTable.tableImage));
        newWheelTexture_.reset(assets.loadTexture(primaryRenderer_, newTable.wheelImage));
        newBackglassTexture_.reset(assets.loadTexture(secondaryRenderer_, newTable.backglassImage));
        newDmdTexture_.reset(assets.loadTexture(secondaryRenderer_, newTable.dmdImage));
        if (assets.getFont()) {
        newTableNameTexture_.reset(assets.renderText(primaryRenderer_, assets.getFont(), 
                            newTable.tableName, settings.fontColor, newTableNameRect_));
        newTableNameRect_.x = (settings.mainWindowWidth - newTableNameRect_.w) / 2;
        newTableNameRect_.y = 10;
        }
        if (!newTable.tableVideo.empty() && settings.mainWindowWidth > 0 && settings.mainWindowHeight > 0) {
        newTableVideo_ = setupVideoPlayer(primaryRenderer_, newTable.tableVideo, settings.mainWindowWidth, settings.mainWindowHeight);
        if (newTableVideo_ && newTableVideo_->player) libvlc_media_player_play(newTableVideo_->player);
        } else {
        newTableVideo_ = nullptr;
        }
        if (!newTable.backglassVideo.empty() && settings.backglassMediaWidth > 0 && settings.backglassMediaHeight > 0) {
        newBackglassVideo_ = setupVideoPlayer(secondaryRenderer_, newTable.backglassVideo, settings.backglassMediaWidth, settings.backglassMediaHeight);
        if (newBackglassVideo_ && newBackglassVideo_->player) libvlc_media_player_play(newBackglassVideo_->player);
        } else {
        newBackglassVideo_ = nullptr;
        }
        if (!newTable.dmdVideo.empty() && settings.dmdMediaWidth > 0 && settings.dmdMediaHeight > 0) {
        newDmdVideo_ = setupVideoPlayer(secondaryRenderer_, newTable.dmdVideo, settings.dmdMediaWidth, settings.dmdMediaHeight);
        if (newDmdVideo_ && newDmdVideo_->player) libvlc_media_player_play(newDmdVideo_->player);
        } else {
        newDmdVideo_ = nullptr;
        }

        transitionActive_ = true;
        startTime_ = SDL_GetTicks();
        duration_ = settings.fadeDurationMs;
        if (tableSound_) Mix_PlayChannel(-1, tableSound_, 0);
        LOG_DEBUG("Transition started, duration: " << duration_);
}

void TransitionManager::updateTransition(Uint32 currentTime, AssetManager& assets) {
    if (!transitionActive_) return;

    Uint32 elapsed = currentTime - startTime_;
    LOG_DEBUG("Transition update, elapsed: " << elapsed << " / " << duration_);
    if (elapsed >= duration_) {
        transitionActive_ = false;
        assets.tableTexture.swap(newTableTexture_);
        assets.wheelTexture.swap(newWheelTexture_);
        assets.backglassTexture.swap(newBackglassTexture_);
        assets.dmdTexture.swap(newDmdTexture_);
        assets.tableNameTexture.swap(newTableNameTexture_);
        assets.tableNameRect = newTableNameRect_;

        // Swap new video players in, stash old ones
        if (assets.tableVideoPlayer) {
            VideoContext* old = assets.tableVideoPlayer;
            assets.tableVideoPlayer = newTableVideo_ ? newTableVideo_ : nullptr;
            newTableVideo_ = nullptr;
            assets.addOldVideoPlayer(old);
        }
        if (assets.backglassVideoPlayer) {
            VideoContext* old = assets.backglassVideoPlayer;
            assets.backglassVideoPlayer = newBackglassVideo_ ? newBackglassVideo_ : nullptr;
            newBackglassVideo_ = nullptr;
            assets.addOldVideoPlayer(old);
        }
        if (assets.dmdVideoPlayer) {
            VideoContext* old = assets.dmdVideoPlayer;
            assets.dmdVideoPlayer = newDmdVideo_ ? newDmdVideo_ : nullptr;
            newDmdVideo_ = nullptr;
            assets.addOldVideoPlayer(old);
        }

        LOG_DEBUG("Transition complete, videos swapped");

        if (loadFunction_) {
            loadFunction_();
            loadFunction_ = nullptr;
        }
        return;
    }

    float t = static_cast<float>(elapsed) / duration_;
    Uint8 oldAlpha = static_cast<Uint8>(255 * (1 - t));
    Uint8 newAlpha = static_cast<Uint8>(255 * t);

    if (assets.tableTexture) SDL_SetTextureAlphaMod(assets.tableTexture.get(), oldAlpha);
    if (assets.wheelTexture) SDL_SetTextureAlphaMod(assets.wheelTexture.get(), oldAlpha);
    if (assets.backglassTexture) SDL_SetTextureAlphaMod(assets.backglassTexture.get(), oldAlpha);
    if (assets.dmdTexture) SDL_SetTextureAlphaMod(assets.dmdTexture.get(), oldAlpha);
    if (assets.tableNameTexture) SDL_SetTextureAlphaMod(assets.tableNameTexture.get(), oldAlpha);
    if (oldTableVideo_ && oldTableVideo_->texture) SDL_SetTextureAlphaMod(oldTableVideo_->texture, oldAlpha);
    if (oldBackglassVideo_ && oldBackglassVideo_->texture) SDL_SetTextureAlphaMod(oldBackglassVideo_->texture, oldAlpha);
    if (oldDmdVideo_ && oldDmdVideo_->texture) SDL_SetTextureAlphaMod(oldDmdVideo_->texture, oldAlpha);

    if (newTableTexture_) SDL_SetTextureAlphaMod(newTableTexture_.get(), newAlpha);
    if (newWheelTexture_) SDL_SetTextureAlphaMod(newWheelTexture_.get(), newAlpha);
    if (newBackglassTexture_) SDL_SetTextureAlphaMod(newBackglassTexture_.get(), newAlpha);
    if (newDmdTexture_) SDL_SetTextureAlphaMod(newDmdTexture_.get(), newAlpha);
    if (newTableNameTexture_) SDL_SetTextureAlphaMod(newTableNameTexture_.get(), newAlpha);
    if (newTableVideo_ && newTableVideo_->texture) SDL_SetTextureAlphaMod(newTableVideo_->texture, newAlpha);
    else if (newTableTexture_) SDL_SetTextureAlphaMod(newTableTexture_.get(), newAlpha);
    if (newBackglassVideo_ && newBackglassVideo_->texture) SDL_SetTextureAlphaMod(newBackglassVideo_->texture, newAlpha);
    else if (newBackglassTexture_) SDL_SetTextureAlphaMod(newBackglassTexture_.get(), newAlpha);
    if (newDmdVideo_ && newDmdVideo_->texture) SDL_SetTextureAlphaMod(newDmdVideo_->texture, newAlpha);
    else if (newDmdTexture_) SDL_SetTextureAlphaMod(newDmdTexture_.get(), newAlpha);
}

void TransitionManager::loadNewContent(std::function<void()> loadFunction) {
    if (transitionActive_ && !loadFunction_) loadFunction_ = loadFunction;
}

bool TransitionManager::isTransitionActive() const {
    return transitionActive_;
}