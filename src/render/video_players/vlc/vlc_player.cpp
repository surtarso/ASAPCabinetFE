/**
 * @file vlc_player.cpp
 * @brief Implementation of the VlcVideoPlayer class for video playback using VLC.
 */

 #include <vlc/libvlc_version.h>

// ----------------------------------------------------------
// Version detection
// ----------------------------------------------------------
#if LIBVLC_VERSION_MAJOR >= 4
    #define HAVE_LIBVLC_V4 1
#endif

#ifdef HAVE_LIBVLC_V4
    #include <vlc/libvlc.h>
    #include <vlc/libvlc_media.h>
    #include <vlc/libvlc_media_player.h>
    #include <vlc/libvlc_events.h>
#else
    #include <vlc/vlc.h>
#endif

// Includes
#include "vlc_player.h"
#include "log/logging.h"
#include <cstdlib> // For malloc, free
#include <string>

// ----------------------------------------------------------
// API compatibility macros
// ----------------------------------------------------------
#if defined(HAVE_LIBVLC_V4)
    #define VLC_STOP(player) libvlc_media_player_stop_async(player)
    #define VLC_NEW_MEDIA(instance, path) libvlc_media_new_path(path)
    #define VLC_STATE_ENDED(state)  ((state) == libvlc_Stopped || (state) == libvlc_Error)
#else
    #define VLC_STOP(player) libvlc_media_player_stop(player)
    #define VLC_NEW_MEDIA(instance, path) libvlc_media_new_path(instance, path)
    #define VLC_STATE_ENDED(state)  ((state) == libvlc_Ended || (state) == libvlc_Stopped || (state) == libvlc_Error)
#endif

VlcVideoPlayer::VlcVideoPlayer() : ctx_(nullptr) {}

VlcVideoPlayer::~VlcVideoPlayer() {
    cleanupContext();
}

void VlcVideoPlayer::cleanupContext() {
    if (!ctx_) return;
    if (ctx_->player) {
        VLC_STOP(ctx_->player);

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
        LOG_ERROR("Lock callback called with invalid context or mutex.");
        return nullptr;
    }
    if (SDL_LockMutex(ctx->mutex) != 0) {
        LOG_ERROR("Lock mutex failed: " + std::string(SDL_GetError()));
        return nullptr;
    }
    *pixels = ctx->pixels;
    // if (ctx->frameCount < 5) {
    //     LOG_DEBUG("VlcVideoPlayer::lock invoked (early). frameCount=" + std::to_string(ctx->frameCount));
    // }
    return nullptr;
}

void VlcVideoPlayer::unlock([[maybe_unused]] void* data, [[maybe_unused]] void* id, [[maybe_unused]] void* const* pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx || !ctx->mutex) {
        LOG_ERROR("Unlock callback called with invalid context or mutex.");
        return;
    }
    SDL_UnlockMutex(ctx->mutex);
    // if (ctx->frameCount < 5) {
    //     LOG_DEBUG("VlcVideoPlayer::unlock invoked (early). frameCount=" + std::to_string(ctx->frameCount));
    // }
}

void VlcVideoPlayer::display([[maybe_unused]] void* data, [[maybe_unused]] void* id) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx) {
        LOG_ERROR("Display callback called with invalid context.");
        return;
    }
    ctx->isPlaying = true;
    // Mark a frame as ready and increment diagnostic counter
    ctx->frameReady = true;
    ctx->frameCount++;
    // if (ctx->frameCount <= 5 || (ctx->frameCount % 60) == 0) {
    //     LOG_DEBUG("VlcVideoPlayer::display invoked. frameReady=true, frameCount=" + std::to_string(ctx->frameCount));
    // }
}

bool VlcVideoPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    if (!ctx_) {
        ctx_ = new VideoContext();
        ctx_->renderer = renderer;
        ctx_->instance = nullptr;
        ctx_->player = nullptr;
        ctx_->texture = nullptr;
        ctx_->pixels = nullptr;
        ctx_->mutex = nullptr;

        ctx_->mutex = SDL_CreateMutex();
        if (!ctx_->mutex) {
            LOG_ERROR("Failed to create mutex: " + std::string(SDL_GetError()));
            cleanupContext();
            return false;
        }

    // Removed --no-xlib because it can interfere with some platform backends (Wayland/Arch)
    const char* args[] = {"--quiet", "--loop"}; // "--no-xlib", "--vout=opengl"
    ctx_->instance = libvlc_new(2, args);
        if (!ctx_->instance) {
            LOG_ERROR("Failed to create VLC instance");
            cleanupContext();
            return false;
        }

        ctx_->player = libvlc_media_player_new(ctx_->instance);
        if (!ctx_->player) {
            LOG_ERROR("Failed to create VLC media player");
            cleanupContext();
            return false;
        }
        libvlc_video_set_callbacks(ctx_->player, lock, unlock, display, ctx_);

        ctx_->width = 0;
        ctx_->height = 0;
        ctx_->pitch = 0;
    } else {
        if (ctx_->player) {
            VLC_STOP(ctx_->player);
        }
    }

    if (ctx_->player) {
        libvlc_media_t* media = VLC_NEW_MEDIA(ctx_->instance, path.c_str());

        if (!media) {
            LOG_ERROR("Failed to create VLC media for path: " + path);
            return false;
        }
        libvlc_media_add_option(media, ":input-repeat=65555");

        libvlc_media_player_set_media(ctx_->player, media);
        libvlc_media_release(media);

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
                LOG_ERROR("Failed to create video texture: " + std::string(SDL_GetError()));
                return false;
            }

            ctx_->pitch = width * 4;
            ctx_->pixels = malloc(ctx_->pitch * height);
            if (!ctx_->pixels) {
                LOG_ERROR("Failed to allocate pixel buffer");
                return false;
            }

            // Diagnostic counter for incoming frames from VLC callbacks
            ctx_->frameCount = 0;

            ctx_->width = width;
            ctx_->height = height;

            // libvlc_video_set_format(ctx_->player, "RV32", ctx_->width, ctx_->height, ctx_->pitch);
            #ifdef HAVE_LIBVLC_V4
                // VLC 4 uses BGRA internally for most outputs
                libvlc_video_set_format(ctx_->player, "BGRA",
                                        ctx_->width, ctx_->height, ctx_->pitch);
            #else
                libvlc_video_set_format(ctx_->player, "RV32",
                                        ctx_->width, ctx_->height, ctx_->pitch);
            #endif

        }
    } else {
        LOG_ERROR("Player not initialized in setup path.");
        return false;
    }

    ctx_->isPlaying = false;
    ctx_->frameReady = false;

    LOG_DEBUG("Successfully set up/changed media for path=" + path + ", width=" + std::to_string(width) + ", height=" + std::to_string(height) + " with backend=vlc");

    return true;
}

void VlcVideoPlayer::play() {
    if (ctx_ && ctx_->player && !ctx_->isPlaying) {
        if (libvlc_media_player_play(ctx_->player) != 0) {
            LOG_ERROR("Failed to start VLC playback");
        } else {
            ctx_->isPlaying = true;
        }
    } else if (ctx_ && ctx_->player && ctx_->isPlaying) {
        LOG_DEBUG("Player is already playing.");
    } else {
        LOG_ERROR("Cannot play, context or player not initialized.");
    }
}

void VlcVideoPlayer::stop() {
    if (ctx_ && ctx_->player) {
        VLC_STOP(ctx_->player);
        ctx_->isPlaying = false;
        ctx_->frameReady = false;
        LOG_DEBUG("Player stopped.");
    } else {
        LOG_DEBUG("No active player to stop.");
    }
}

void VlcVideoPlayer::update() {
    if (!ctx_ || !ctx_->texture || !ctx_->pixels || !ctx_->mutex || !ctx_->player) {
        LOG_ERROR("Invalid video context in update. Skipping frame update.");
        return;
    }

    libvlc_state_t state = libvlc_media_player_get_state(ctx_->player);
    if (VLC_STATE_ENDED(state)) {
        ctx_->isPlaying = false;
        ctx_->frameReady = false;
        return;
    }

    if (ctx_->frameReady) {
        if (SDL_LockMutex(ctx_->mutex) == 0) {
            if (SDL_UpdateTexture(ctx_->texture, nullptr, ctx_->pixels, ctx_->pitch) != 0) {
                LOG_ERROR("SDL_UpdateTexture failed: " + std::string(SDL_GetError()));
            } else {
                ctx_->frameReady = false;
                // if (ctx_->frameCount <= 5 || (ctx_->frameCount % 60) == 0) {
                //     LOG_DEBUG("VlcVideoPlayer: SDL_UpdateTexture succeeded. frameCount=" + std::to_string(ctx_->frameCount));
                // }
            }
            SDL_UnlockMutex(ctx_->mutex);
        } else {
            LOG_ERROR("SDL_LockMutex failed during update: " + std::string(SDL_GetError()));
        }
    }
}

SDL_Texture* VlcVideoPlayer::getTexture() const {
    return ctx_ ? ctx_->texture : nullptr;
}

int VlcVideoPlayer::getFrameCount() const {
    return ctx_ ? ctx_->frameCount : 0;
}

bool VlcVideoPlayer::isPlaying() const {
    if (ctx_ && ctx_->player) {
        libvlc_state_t state = libvlc_media_player_get_state(ctx_->player);
        return (state == libvlc_Playing);
    }
    return false;
}

void VlcVideoPlayer::setVolume(float volume) {
    if (!ctx_ || !ctx_->player) {
        LOG_ERROR("Cannot set volume, player not initialized");
        return;
    }
    int vlcVolume = static_cast<int>(volume);
    vlcVolume = std::max(0, std::min(100, vlcVolume));
    LOG_DEBUG("Setting volume to " + std::to_string(volume) + " (VLC: " + std::to_string(vlcVolume) + ")");
    libvlc_audio_set_volume(ctx_->player, vlcVolume);
}

void VlcVideoPlayer::setMute(bool mute) {
    if (!ctx_ || !ctx_->player) {
        LOG_ERROR("Cannot set mute state, player not initialized");
        return;
    }
    LOG_DEBUG("Setting mute to " + std::string(mute ? "true" : "false"));
    libvlc_audio_set_mute(ctx_->player, mute ? 1 : 0);
}
