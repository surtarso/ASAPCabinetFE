#include "vlc_player.h"
#include "log/logging.h"
#include <cstdio> // For fopen, fclose, stderr redirection

VlcVideoPlayer::VlcVideoPlayer() : ctx_(nullptr) {}

VlcVideoPlayer::~VlcVideoPlayer() {
    cleanupContext();
}

void VlcVideoPlayer::cleanupContext() {
    if (!ctx_) return;
    if (ctx_->player) {
        libvlc_media_player_stop(ctx_->player);
        libvlc_media_player_release(ctx_->player);
        ctx_->player = nullptr;
    }
    if (ctx_->instance) {
        libvlc_release(ctx_->instance);
        ctx_->instance = nullptr;
    }
    if (ctx_->texture) {
        SDL_DestroyTexture(ctx_->texture);
        ctx_->texture = nullptr;
    }
    if (ctx_->pixels) {
        free(ctx_->pixels);
        ctx_->pixels = nullptr;
    }
    if (ctx_->mutex) {
        SDL_DestroyMutex(ctx_->mutex);
        ctx_->mutex = nullptr;
    }
    delete ctx_;
    ctx_ = nullptr;
}

void* VlcVideoPlayer::lock(void* data, void** pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx || !ctx->mutex) {
        LOG_ERROR("VlcVideoPlayer: Lock callback called with invalid context or mutex.");
        return nullptr;
    }
    if (SDL_LockMutex(ctx->mutex) != 0) {
        LOG_ERROR("VlcVideoPlayer: Lock mutex failed: " << SDL_GetError());
        return nullptr;
    }
    *pixels = ctx->pixels;
    return nullptr;
}

void VlcVideoPlayer::unlock(void* data, void* id, void* const* pixels) {
    (void)id;
    (void)pixels; // Suppress unused parameter warning
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx || !ctx->mutex) {
        LOG_ERROR("VlcVideoPlayer: Unlock callback called with invalid context or mutex.");
        return;
    }
    SDL_UnlockMutex(ctx->mutex);
}

void VlcVideoPlayer::display(void* data, void* id) {
    (void)id;
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx) {
        LOG_ERROR("VlcVideoPlayer: Display callback called with invalid context.");
        return;
    }
    ctx->isPlaying = true;
    ctx->frameReady = true; // Signal that a new frame is ready
}

bool VlcVideoPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    if (!ctx_) {
        // Initial setup: Allocate all context resources
        ctx_ = new VideoContext();
        ctx_->renderer = renderer;
        ctx_->instance = nullptr;
        ctx_->player = nullptr;
        ctx_->texture = nullptr;
        ctx_->pixels = nullptr;
        ctx_->mutex = nullptr;

        ctx_->mutex = SDL_CreateMutex();
        if (!ctx_->mutex) {
            LOG_ERROR("VlcVideoPlayer: Failed to create mutex: " << SDL_GetError());
            cleanupContext();
            return false;
        }

        const char* args[] = {"--quiet", "--no-xlib"};
        ctx_->instance = libvlc_new(sizeof(args) / sizeof(args[0]), args);
        if (!ctx_->instance) {
            LOG_ERROR("VlcVideoPlayer: Failed to create VLC instance");
            cleanupContext();
            return false;
        }

        ctx_->player = libvlc_media_player_new(ctx_->instance);
        if (!ctx_->player) {
            LOG_ERROR("VlcVideoPlayer: Failed to create VLC media player");
            cleanupContext();
            return false;
        }
        libvlc_video_set_callbacks(ctx_->player, lock, unlock, display, ctx_);

        // Initialize dimensions
        ctx_->width = 0;
        ctx_->height = 0;
        ctx_->pitch = 0;

    } else {
        // Player is being reused: Stop current media
        if (ctx_->player) {
            libvlc_media_player_stop(ctx_->player);
        }
    }

    // Always set new media and recreate texture/pixels if dimensions change
    if (ctx_->player) {
        // Create new media for the given path
        libvlc_media_t* media = libvlc_media_new_path(ctx_->instance, path.c_str());
        if (!media) {
            LOG_ERROR("VlcVideoPlayer: Failed to create VLC media for path: " << path);
            // If we're reusing, don't clean up everything, just return false
            return false;
        }
        libvlc_media_add_option(media, ":input-repeat=-1"); // -1 for infinite loop

        // Set the new media to the existing player
        libvlc_media_player_set_media(ctx_->player, media);
        libvlc_media_release(media); // Media can be released after setting it to the player

        // Recreate texture and pixel buffer only if dimensions change
        if (width != ctx_->width || height != ctx_->height) {
            if (ctx_->texture) {
                SDL_DestroyTexture(ctx_->texture);
                ctx_->texture = nullptr;
            }
            if (ctx_->pixels) {
                free(ctx_->pixels);
                ctx_->pixels = nullptr;
            }

            ctx_->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING, width, height);
            if (!ctx_->texture) {
                LOG_ERROR("VlcVideoPlayer: Failed to create video texture: " << SDL_GetError());
                // If we're reusing, don't clean up everything, just return false
                return false;
            }

            ctx_->pitch = width * 4; // ARGB8888 has 4 bytes per pixel
            ctx_->pixels = malloc(ctx_->pitch * height);
            if (!ctx_->pixels) {
                LOG_ERROR("VlcVideoPlayer: Failed to allocate pixel buffer");
                // If we're reusing, don't clean up everything, just return false
                return false;
            }
            
            // Update stored dimensions
            ctx_->width = width;
            ctx_->height = height;

            // Re-set video format for the player with new dimensions
            libvlc_video_set_format(ctx_->player, "RV32", ctx_->width, ctx_->height, ctx_->pitch);
        }
    } else {
        LOG_ERROR("VlcVideoPlayer: Player not initialized in setup path.");
        return false;
    }

    ctx_->isPlaying = false;
    ctx_->frameReady = false;

    LOG_DEBUG("VlcVideoPlayer: Successfully set up/changed media for path=" << path << ", width=" << width << ", height=" << height << " with backend=vlc");

    return true;
}

void VlcVideoPlayer::play() {
    if (ctx_ && ctx_->player && !ctx_->isPlaying) {
        if (libvlc_media_player_play(ctx_->player) != 0) {
            LOG_ERROR("VlcVideoPlayer: Failed to start VLC playback");
        } else {
            ctx_->isPlaying = true;
            // No need to set frameReady here, it will be set by the display callback
        }
    } else if (ctx_ && ctx_->player && ctx_->isPlaying) {
        LOG_DEBUG("VlcVideoPlayer: Player is already playing.");
    } else {
        LOG_ERROR("VlcVideoPlayer: Cannot play, context or player not initialized.");
    }
}

void VlcVideoPlayer::stop() {
    if (ctx_ && ctx_->player) {
        libvlc_media_player_stop(ctx_->player);
        ctx_->isPlaying = false;
        ctx_->frameReady = false; // Reset frameReady on stop
        LOG_DEBUG("VlcVideoPlayer: Player stopped.");
    } else {
        LOG_DEBUG("VlcVideoPlayer: No active player to stop.");
    }
}

void VlcVideoPlayer::update() {
    if (!ctx_ || !ctx_->texture || !ctx_->pixels || !ctx_->mutex || !ctx_->player) {
        // This can happen if setup failed or cleanup happened
        LOG_ERROR("VlcVideoPlayer: Invalid video context in update. Skipping frame update.");
        return;
    }

    // Check if the player is still playing, VLC might stop it internally
    libvlc_state_t state = libvlc_media_player_get_state(ctx_->player);
    if (state == libvlc_Ended || state == libvlc_Stopped || state == libvlc_Error) {
        if (ctx_->isPlaying) { // Only log if we thought it was playing
            LOG_DEBUG("VlcVideoPlayer: VLC player state changed to " << state << ". Setting isPlaying to false.");
        }
        ctx_->isPlaying = false;
        ctx_->frameReady = false; // No new frames if stopped
        return;
    }
    
    // Only update texture if a new frame is ready
    if (ctx_->frameReady) {
        if (SDL_LockMutex(ctx_->mutex) == 0) {
            if (SDL_UpdateTexture(ctx_->texture, nullptr, ctx_->pixels, ctx_->pitch) != 0) {
                LOG_ERROR("VlcVideoPlayer: SDL_UpdateTexture failed: " << SDL_GetError());
            } else {
                // Texture updated, clear the flag
                ctx_->frameReady = false;
            }
            SDL_UnlockMutex(ctx_->mutex);
        } else {
            LOG_ERROR("VlcVideoPlayer: SDL_LockMutex failed during update: " << SDL_GetError());
        }
    }
}

SDL_Texture* VlcVideoPlayer::getTexture() const {
    return ctx_ ? ctx_->texture : nullptr;
}

bool VlcVideoPlayer::isPlaying() const {
    // Also check VLC's internal state to be more robust
    if (ctx_ && ctx_->player) {
        libvlc_state_t state = libvlc_media_player_get_state(ctx_->player);
        return (state == libvlc_Playing);
    }
    return false;
}

void VlcVideoPlayer::setVolume(float volume) {
    if (!ctx_ || !ctx_->player) {
        LOG_ERROR("VlcVideoPlayer: Cannot set volume, player not initialized");
        return;
    }
    // VLC volume is 0-100, so scale the float from 0-100 (from settings) directly
    int vlcVolume = static_cast<int>(volume);
    // Clamp to VLC's expected range (0-100 for libvlc_audio_set_volume)
    vlcVolume = std::max(0, std::min(100, vlcVolume));

    LOG_DEBUG("VlcVideoPlayer: Setting volume to " << volume << " (VLC: " << vlcVolume << ")");
    libvlc_audio_set_volume(ctx_->player, vlcVolume);
}

void VlcVideoPlayer::setMute(bool mute) {
    if (!ctx_ || !ctx_->player) {
        LOG_ERROR("VlcVideoPlayer: Cannot set mute state, player not initialized");
        return;
    }
    LOG_DEBUG("VlcVideoPlayer: Setting mute to " << (mute ? "true" : "false"));
    libvlc_audio_set_mute(ctx_->player, mute ? 1 : 0);
}