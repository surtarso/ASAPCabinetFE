#ifndef VLC_PLAYER_H
#define VLC_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL2/SDL.h>
#include <vlc/vlc.h>
#include <string>

class VlcVideoPlayer : public IVideoPlayer {
public:
    VlcVideoPlayer();
    ~VlcVideoPlayer() override;
    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;
    void play() override;
    void stop() override;
    void update() override;
    SDL_Texture* getTexture() const override;
    bool isPlaying() const override;

private:
    struct VideoContext {
        libvlc_instance_t* instance;
        libvlc_media_player_t* player;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        void* pixels;
        int pitch;
        int width;
        int height;
        SDL_mutex* mutex;
        bool isPlaying;
    };

    VideoContext* ctx_;

    static void* lock(void* data, void** pixels);
    static void unlock(void* data, void* id, void* const* pixels);
    static void display(void* data, void* id);
    void cleanupContext();
};

#endif // VLC_PLAYER_H