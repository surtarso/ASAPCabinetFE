#ifndef FFMPEG_PLAYER_H
#define FFMPEG_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL.h>
#include <string>
#include <chrono> // For timing video frames

// Forward declarations for FFmpeg structs
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct AVAudioFifo; // For audio buffering
struct SwrContext;  // For audio resampling

/**
 * @brief Concrete implementation of IVideoPlayer using FFmpeg.
 *
 * This class provides video playback capabilities by integrating with the FFmpeg libraries.
 * It handles video decoding, rendering to an SDL_Texture, and basic playback controls.
 * It also includes basic audio handling for the video stream, allowing volume and mute control.
 */
class FFmpegPlayer : public IVideoPlayer {
public:
    /**
     * @brief Constructs an FFmpegPlayer instance.
     */
    FFmpegPlayer();

    /**
     * @brief Destroys the FFmpegPlayer instance and cleans up FFmpeg and SDL resources.
     */
    ~FFmpegPlayer() override;

    /**
     * @brief Sets up the FFmpeg video player with the given renderer, path, and dimensions.
     * @param renderer Pointer to the SDL_Renderer.
     * @param path Path to the video file.
     * @param width Desired width of the video texture.
     * @param height Desired height of the video texture.
     * @return True if setup is successful, false otherwise.
     */
    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;

    /**
     * @brief Starts video playback.
     */
    void play() override;

    /**
     * @brief Stops video playback.
     */
    void stop() override;

    /**
     * @brief Updates the video frame and texture.
     */
    void update() override;

    /**
     * @brief Retrieves the current video frame as an SDL_Texture.
     * @return Pointer to the SDL_Texture.
     */
    SDL_Texture* getTexture() const override;

    /**
     * @brief Checks if the video is currently playing.
     * @return True if playing, false otherwise.
     */
    bool isPlaying() const override;

    /**
     * @brief Sets the volume for the video's audio track.
     * @param volume The desired volume, a float between 0.0 (silent) and 1.0 (full volume).
     */
    void setVolume(float volume) override;

    /**
     * @brief Sets the mute state for the video's audio track.
     * @param mute True to mute, false to unmute.
     */
    void setMute(bool mute) override;

private:
    SDL_Renderer* renderer_;
    std::string path_;
    int width_;
    int height_;
    bool isPlaying_;
    SDL_Texture* texture_;

    // FFmpeg video context
    AVFormatContext* formatContext_;
    AVCodecContext* videoCodecContext_;
    AVFrame* videoFrame_;
    AVFrame* rgbFrame_;
    AVPacket* videoPacket_;
    SwsContext* swsContext_;
    int videoStreamIndex_;
    uint8_t* rgbBuffer_;

    // Video synchronization
    double videoClock_; // Current video timestamp in seconds
    std::chrono::high_resolution_clock::time_point lastFrameTime_; // Time when the last frame was displayed
    std::chrono::high_resolution_clock::time_point playbackStartTime_; // Time when playback started for current loop

    // FFmpeg audio context
    AVCodecContext* audioCodecContext_;
    AVFrame* audioFrame_;
    AVPacket* audioPacket_;
    SwrContext* swrContext_;
    AVAudioFifo* audioFifo_; // Buffer for decoded audio
    int audioStreamIndex_;
    SDL_AudioDeviceID audioDevice_;
    SDL_AudioSpec audioSpec_;
    bool needsVideoDecoderReset_; 
    float currentVolume_;
    bool isMuted_;
    
    /**
     * @brief Cleans up all FFmpeg and SDL resources.
     */
    void cleanup();

    /**
     * @brief Decodes a single video frame.
     * @return True if a frame was successfully decoded and converted, false otherwise.
     */
    bool decodeVideoFrame();

    /**
     * @brief Decodes audio packets and pushes samples to the FIFO.
     * @return True if audio was successfully decoded, false otherwise.
     */
    bool decodeAudioFrame();

    /**
     * @brief Updates the SDL_Texture with the current RGB video frame.
     */
    void updateTexture();

    /**
     * @brief Static callback for SDL audio.
     * This function is called by SDL when it needs more audio data.
     * @param userdata Pointer to the FFmpegPlayer instance.
     * @param stream Pointer to the audio buffer.
     * @param len Length of the audio buffer in bytes.
     */
    static void SDLAudioCallback(void* userdata, Uint8* stream, int len);
};

#endif // FFMPEG_PLAYER_H