#ifndef VIDEO_H
#define VIDEO_H

#include <SDL.h>
#include <vlc/vlc.h>
#include <string>

struct VideoContext {
    libvlc_instance_t* instance;
    libvlc_media_player_t* player;
    SDL_Renderer* renderer; // Store renderer for main-thread updates
    SDL_Texture* texture;
    void* pixels;           // Buffer for video frames
    int pitch;              // Pitch of the frame buffer
    int width;
    int height;
    SDL_mutex* mutex;       // Mutex for thread-safe buffer access
};

VideoContext* setupVideoPlayer(SDL_Renderer* renderer, const std::string& path, int width, int height);
void cleanupVideoContext(VideoContext* ctx);

#endif