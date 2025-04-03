#include "render/video_player.h"
#include "utils/logging.h"
#include <iostream>

void* lock(void* data, void** pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (SDL_LockMutex(ctx->mutex) != 0) {
        LOG_DEBUG("Lock mutex failed: " << SDL_GetError());
        return nullptr;
    }
    *pixels = ctx->pixels;
    return nullptr;
}

void unlock(void* data, void* id, void* const* pixels) {
    (void)id;      // Silence warning
    (void)pixels;  // Silence warning
    VideoContext* ctx = static_cast<VideoContext*>(data);
    SDL_UnlockMutex(ctx->mutex);
}

void display(void* data, void* id) {
    (void)id;    // Silence warning
    VideoContext* ctx = static_cast<VideoContext*>(data);
    ctx->isPlaying = true; // Mark as playing once frames are being delivered
    //LOG_DEBUG("Display callback triggered for video context: " << ctx);
}

VideoContext* setupVideoPlayer(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    VideoContext* ctx = new VideoContext();
    ctx->renderer = renderer;
    ctx->width = width;
    ctx->height = height;
    ctx->isPlaying = false; // Initially not playing
    ctx->mutex = SDL_CreateMutex();
    if (!ctx->mutex) {
        LOG_ERROR("Failed to create mutex: " << SDL_GetError());
        delete ctx;
        return nullptr;
    }

    const char* args[] = {"--quiet", "--loop"};
    ctx->instance = libvlc_new(2, args);
    if (!ctx->instance) {
        LOG_ERROR("Failed to create VLC instance");
        SDL_DestroyMutex(ctx->mutex);
        delete ctx;
        return nullptr;
    }

    libvlc_media_t* media = libvlc_media_new_path(ctx->instance, path.c_str());
    if (!media) {
        LOG_ERROR("Failed to create VLC media for path: " << path);
        libvlc_release(ctx->instance);
        SDL_DestroyMutex(ctx->mutex);
        delete ctx;
        return nullptr;
    }

    libvlc_media_add_option(media, "input-repeat=65535"); // Loop indefinitely

    ctx->player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);
    if (!ctx->player) {
        LOG_ERROR("Failed to create VLC media player");
        libvlc_release(ctx->instance);
        SDL_DestroyMutex(ctx->mutex);
        delete ctx;
        return nullptr;
    }

    ctx->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!ctx->texture) {
        LOG_ERROR("Failed to create video texture: " << SDL_GetError());
        libvlc_media_player_release(ctx->player);
        libvlc_release(ctx->instance);
        SDL_DestroyMutex(ctx->mutex);
        delete ctx;
        return nullptr;
    }

    ctx->pitch = width * 4; // 4 bytes per pixel for ARGB8888
    ctx->pixels = malloc(ctx->pitch * height);
    if (!ctx->pixels) {
        LOG_ERROR("Failed to allocate pixel buffer");
        SDL_DestroyTexture(ctx->texture);
        libvlc_media_player_release(ctx->player);
        libvlc_release(ctx->instance);
        SDL_DestroyMutex(ctx->mutex);
        delete ctx;
        return nullptr;
    }

    libvlc_video_set_callbacks(ctx->player, lock, unlock, display, ctx);
    libvlc_video_set_format(ctx->player, "RV32", width, height, ctx->pitch);
    if (libvlc_media_player_play(ctx->player) != 0) {
        LOG_ERROR("Failed to start VLC playback");
        SDL_DestroyTexture(ctx->texture);
        libvlc_media_player_release(ctx->player);
        libvlc_release(ctx->instance);
        SDL_DestroyMutex(ctx->mutex);
        free(ctx->pixels);
        delete ctx;
        return nullptr;
    }

    LOG_DEBUG("Video player setup: player=" << ctx->player << ", texture=" << ctx->texture
              << ", pixels=" << ctx->pixels << ", mutex=" << ctx->mutex);
    return ctx;
}

void cleanupVideoContext(VideoContext* ctx) {
    if (!ctx) return;
    if (ctx->player) {
        libvlc_media_player_stop(ctx->player);
        libvlc_media_player_release(ctx->player);
    }
    if (ctx->instance) libvlc_release(ctx->instance);
    if (ctx->texture) SDL_DestroyTexture(ctx->texture);
    if (ctx->pixels) free(ctx->pixels);
    if (ctx->mutex) SDL_DestroyMutex(ctx->mutex);
    delete ctx;
}

void updateVideoTexture(VideoContext* video) {
    if (!video || !video->texture || !video->pixels || !video->mutex || !video->player) {
        LOG_ERROR("Invalid video context in updateVideoTexture");
        return;
    }
    if (!video->isPlaying) {
        //LOG_DEBUG("Video not yet playing, skipping update");
        return;
    }
    if (SDL_LockMutex(video->mutex) == 0) {
        if (SDL_UpdateTexture(video->texture, nullptr, video->pixels, video->pitch) != 0) {
            LOG_ERROR("SDL_UpdateTexture failed: " << SDL_GetError());
        }
        SDL_UnlockMutex(video->mutex);
    } else {
        LOG_ERROR("SDL_LockMutex failed: " << SDL_GetError());
    }
}