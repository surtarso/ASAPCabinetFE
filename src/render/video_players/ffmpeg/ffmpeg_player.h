#ifndef FFMPEG_PLAYER_H
#define FFMPEG_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL2/SDL.h>
#include <string>

// Forward declarations
struct AVFormatContext;
class VideoDecoder;
class AudioDecoder;

class FFmpegPlayer : public IVideoPlayer {
public:
    FFmpegPlayer();
    ~FFmpegPlayer() override;

    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;
    void play() override;
    void stop() override;
    void update() override;
    SDL_Texture* getTexture() const override;
    bool isPlaying() const override;
    void setVolume(float volume) override;
    void setMute(bool mute) override;

    // For decoders to access
    void seekToBeginning(int streamIndex);
    AVFormatContext* getFormatContext() const { return formatContext_; }

private:
    SDL_Renderer* renderer_;
    std::string path_;
    int width_;
    int height_;
    bool isPlaying_;
    AVFormatContext* formatContext_;
    VideoDecoder* videoDecoder_;
    AudioDecoder* audioDecoder_;

    void cleanup();
};

#endif // FFMPEG_PLAYER_H