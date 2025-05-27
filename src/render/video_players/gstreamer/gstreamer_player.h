#ifndef GSTREAMER_PLAYER_H
#define GSTREAMER_PLAYER_H

#include "render/ivideo_player.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <SDL2/SDL.h>
#include <string>
#include <vector>

class GStreamerVideoPlayer : public IVideoPlayer {
public:
    GStreamerVideoPlayer();
    ~GStreamerVideoPlayer() override;
    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;
    void play() override;
    void stop() override;
    void update() override;
    SDL_Texture* getTexture() const override;
    bool isPlaying() const override;
    void setVolume(float volume) override;
    void setMute(bool mute) override;

private:
    struct VideoContext {
        SDL_Renderer* renderer = nullptr;
        GstElement* pipeline = nullptr;
        SDL_Texture* texture = nullptr;
        void* pixels = nullptr;
        int pitch = 0;
        int width = 0;
        int height = 0;
        SDL_mutex* mutex = nullptr;
        std::vector<uint8_t> frame_buffer;
        bool isPlaying = false;
        bool frame_ready = false;
        bool first_frame = true;
        std::string current_path;
        guint bus_watch_id = 0;
        GstElement* volume_element = nullptr; // Added for volume control
    };

    VideoContext* ctx_;
    static GstFlowReturn onNewSample(GstAppSink* appsink, void* data);
    void cleanupContext();
    std::string getCurrentPath() const;
    static void onPadAdded(GstElement* decodebin, GstPad* pad, gpointer data);
    void linkVideoPad(GstPad* pad);
    void linkAudioPad(GstPad* pad);
};

#endif // GSTREAMER_PLAYER_H