#include "vlc_player.h"
#include "log/logging.h"

VlcVideoPlayer::VlcVideoPlayer() : ctx_(nullptr) {}

VlcVideoPlayer::~VlcVideoPlayer() {
    cleanupContext();
}

void VlcVideoPlayer::cleanupContext() {
    if (!ctx_) return;
    if (ctx_->player) {
        libvlc_media_player_stop(ctx_->player);
        libvlc_media_player_release(ctx_->player);
    }
    if (ctx_->instance) libvlc_release(ctx_->instance);
    if (ctx_->texture) SDL_DestroyTexture(ctx_->texture);
    if (ctx_->pixels) free(ctx_->pixels);
    if (ctx_->mutex) SDL_DestroyMutex(ctx_->mutex);
    delete ctx_;
    ctx_ = nullptr;
}

void* VlcVideoPlayer::lock(void* data, void** pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (SDL_LockMutex(ctx->mutex) != 0) {
        LOG_ERROR("VlcVideoPlayer: Lock mutex failed: " << SDL_GetError());
        return nullptr;
    }
    *pixels = ctx->pixels;
    return nullptr;
}

void VlcVideoPlayer::unlock(void* data, void* id, void* const* pixels) {
    (void)id;
    (void)pixels;
    VideoContext* ctx = static_cast<VideoContext*>(data);
    SDL_UnlockMutex(ctx->mutex);
}

void VlcVideoPlayer::display(void* data, void* id) {
    (void)id;
    VideoContext* ctx = static_cast<VideoContext*>(data);
    ctx->isPlaying = true;
}

bool VlcVideoPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    ctx_ = new VideoContext();
    ctx_->renderer = renderer;
    ctx_->width = width;
    ctx_->height = height;
    ctx_->isPlaying = false;
    ctx_->mutex = SDL_CreateMutex();
    if (!ctx_->mutex) {
        LOG_ERROR("VlcVideoPlayer: Failed to create mutex: " << SDL_GetError());
        cleanupContext();
        return false;
    }

    const char* args[] = {"--quiet", "--loop"};
    ctx_->instance = libvlc_new(2, args);
    if (!ctx_->instance) {
        LOG_ERROR("VlcVideoPlayer: Failed to create VLC instance");
        cleanupContext();
        return false;
    }

    libvlc_media_t* media = libvlc_media_new_path(ctx_->instance, path.c_str());
    if (!media) {
        LOG_ERROR("VlcVideoPlayer: Failed to create VLC media for path: " << path);
        cleanupContext();
        return false;
    }

    libvlc_media_add_option(media, "input-repeat=65535");
    ctx_->player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);
    if (!ctx_->player) {
        LOG_ERROR("VlcVideoPlayer: Failed to create VLC media player");
        cleanupContext();
        return false;
    }

    ctx_->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!ctx_->texture) {
        LOG_ERROR("VlcVideoPlayer: Failed to create video texture: " << SDL_GetError());
        cleanupContext();
        return false;
    }

    ctx_->pitch = width * 4;
    ctx_->pixels = malloc(ctx_->pitch * height);
    if (!ctx_->pixels) {
        LOG_ERROR("VlcVideoPlayer: Failed to allocate pixel buffer");
        cleanupContext();
        return false;
    }

    libvlc_video_set_callbacks(ctx_->player, lock, unlock, display, ctx_);
    libvlc_video_set_format(ctx_->player, "RV32", width, height, ctx_->pitch);
    return true;
}

void VlcVideoPlayer::play() {
    if (ctx_ && ctx_->player && !ctx_->isPlaying) {
        if (libvlc_media_player_play(ctx_->player) != 0) {
            LOG_ERROR("VlcVideoPlayer: Failed to start VLC playback");
        }
    }
}

void VlcVideoPlayer::stop() {
    if (ctx_ && ctx_->player) {
        libvlc_media_player_stop(ctx_->player);
        ctx_->isPlaying = false; // Ensure isPlaying state is false after stopping
    }
}

void VlcVideoPlayer::update() {
    if (!ctx_ || !ctx_->texture || !ctx_->pixels || !ctx_->mutex || !ctx_->player) {
        LOG_ERROR("VlcVideoPlayer: Invalid video context in update");
        return;
    }
    if (!ctx_->isPlaying) {
        return;
    }

    // Check if the player is still playing, VLC might stop it internally
    libvlc_state_t state = libvlc_media_player_get_state(ctx_->player);
    if (state == libvlc_Ended || state == libvlc_Stopped || state == libvlc_Error) {
        ctx_->isPlaying = false;
        return;
    }

    if (SDL_LockMutex(ctx_->mutex) == 0) {
        if (SDL_UpdateTexture(ctx_->texture, nullptr, ctx_->pixels, ctx_->pitch) != 0) {
            LOG_ERROR("VlcVideoPlayer: SDL_UpdateTexture failed: " << SDL_GetError());
        }
        SDL_UnlockMutex(ctx_->mutex);
    } else {
        LOG_ERROR("VlcVideoPlayer: SDL_LockMutex failed: " << SDL_GetError());
    }
}

SDL_Texture* VlcVideoPlayer::getTexture() const {
    return ctx_ ? ctx_->texture : nullptr;
}

bool VlcVideoPlayer::isPlaying() const {
    return ctx_ ? ctx_->isPlaying : false;
}

void VlcVideoPlayer::setVolume(float volume) {
    if (!ctx_ || !ctx_->player) {
        LOG_ERROR("VlcVideoPlayer: Cannot set volume, player not initialized");
        return;
    }
    // Normalize volume from 0-100 (Settings) to 0.0-1.0
    float normalizedVolume = volume / 100.0f;
    // Clamp volume to 0.0-1.0 range
    float clampedVolume = normalizedVolume < 0.0f ? 0.0f : (normalizedVolume > 1.0f ? 1.0f : normalizedVolume);
    // VLC volume is 0-100, so scale the 0.0-1.0 float
    int vlcVolume = static_cast<int>(clampedVolume * 100.0f);
    LOG_DEBUG("VlcVideoPlayer: Setting volume to " << clampedVolume << " (VLC: " << vlcVolume << ")");
    libvlc_audio_set_volume(ctx_->player, vlcVolume);
    // Verify VLC state
    // int currentVolume = libvlc_audio_get_volume(ctx_->player);
    // LOG_DEBUG("VlcVideoPlayer: VLC reported volume: " << currentVolume);
}

void VlcVideoPlayer::setMute(bool mute) {
    if (!ctx_ || !ctx_->player) {
        LOG_ERROR("VlcVideoPlayer: Cannot set mute state, player not initialized");
        return;
    }
    LOG_DEBUG("VlcVideoPlayer: Setting mute to " << (mute ? "true" : "false"));
    libvlc_audio_set_mute(ctx_->player, mute ? 1 : 0);
    // Verify VLC state
    // int muteState = libvlc_audio_get_mute(ctx_->player);
    // LOG_DEBUG("VlcVideoPlayer: VLC reported mute state: " << (muteState ? "true" : "false"));
}
