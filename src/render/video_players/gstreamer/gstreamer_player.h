#ifndef GSTREAMER_PLAYER_H
#define GSTREAMER_PLAYER_H

#include "render/ivideo_player.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <SDL2/SDL.h>
#include <string>
#include <vector>

/**
 * @file gstreamer_player.h
 * @brief Defines the GStreamerVideoPlayer class for video playback using GStreamer.
 *
 * This header provides the GStreamerVideoPlayer class, which implements the IVideoPlayer
 * interface using GStreamer for decoding and playback of video and audio streams.
 * It manages a pipeline with dynamic pad linking and SDL texture rendering.
 */

/**
 * @struct VideoContext
 * @brief Structure holding the GStreamer video playback context.
 *
 * Contains all state and resources required for video playback, including the
 * GStreamer pipeline, SDL renderer, texture, and synchronization mutex.
 */
struct VideoContext {
    SDL_Renderer* renderer = nullptr;        ///< SDL renderer for texture creation.
    GstElement* pipeline = nullptr;          ///< GStreamer pipeline for media playback.
    SDL_Texture* texture = nullptr;          ///< SDL texture for rendering video frames.
    void* pixels = nullptr;                  ///< Pixel buffer for texture updates.
    int pitch = 0;                           ///< Pitch (bytes per row) of the texture.
    int width = 0;                           ///< Width of the video output.
    int height = 0;                          ///< Height of the video output.
    SDL_mutex* mutex = nullptr;              ///< Mutex for thread-safe frame updates.
    std::vector<uint8_t> frame_buffer;       ///< Buffer to store decoded video frames.
    bool isPlaying = false;                  ///< Flag indicating playback state.
    bool frame_ready = false;                ///< Flag indicating a new frame is available.
    bool first_frame = true;                 ///< Flag for the first frame detection.
    std::string current_path;                ///< Path to the current media file.
    guint bus_watch_id = 0;                  ///< ID of the GStreamer bus watch.
    GstElement* volume_element = nullptr;    ///< GStreamer volume element for audio control.
    GstElement* audiosink_element = nullptr;  ///< GStreamer audio sink element.
    GstElement* videosink_element = nullptr;  ///< GStreamer video sink (appsink) element.
};

/**
 * @class GStreamerVideoPlayer
 * @brief Implements video and audio playback using GStreamer.
 *
 * This class extends IVideoPlayer to handle media file playback with GStreamer,
 * managing a dynamic pipeline with decodebin, video conversion, and audio handling.
 * It uses SDL for rendering and supports volume and mute control.
 */
class GStreamerVideoPlayer : public IVideoPlayer {
public:
    /**
     * @brief Constructs a GStreamerVideoPlayer instance.
     *
     * Initializes the player with global GStreamer instance management.
     */
    GStreamerVideoPlayer();

    /**
     * @brief Destroys the GStreamerVideoPlayer instance and cleans up resources.
     *
     * Ensures all GStreamer and SDL resources are properly freed.
     */
    ~GStreamerVideoPlayer() override;

    /**
     * @brief Sets up the player with renderer and media file.
     *
     * Configures the player with the specified renderer, file path, and resolution.
     *
     * @param renderer Pointer to the SDL_Renderer for texture rendering.
     * @param path Path to the media file.
     * @param width Desired width of the video output.
     * @param height Desired height of the video output.
     * @return True on successful setup, false otherwise.
     */
    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;

    /**
     * @brief Starts playback of the media file.
     *
     * Initiates video and audio playback if not already playing.
     */
    void play() override;

    /**
     * @brief Stops playback of the media file.
     *
     * Halts video and audio playback and resets the pipeline state.
     */
    void stop() override;

    /**
     * @brief Updates the playback state.
     *
     * Updates the SDL texture with the latest decoded frame if available.
     */
    void update() override;

    /**
     * @brief Retrieves the current video texture.
     *
     * @return Pointer to the SDL_Texture, or nullptr if not initialized.
     */
    SDL_Texture* getTexture() const override;

    /**
     * @brief Checks if the player is currently playing.
     *
     * @return True if playing, false otherwise.
     */
    bool isPlaying() const override;

    /**
     * @brief Sets the audio volume.
     *
     * @param volume Volume level (0-100).
     */
    void setVolume(float volume) override;

    /**
     * @brief Toggles audio mute state.
     *
     * @param mute True to mute, false to unmute.
     */
    void setMute(bool mute) override;

private:
    VideoContext* ctx_; ///< Pointer to the video playback context.

    /**
     * @brief Handles new video frame samples from the appsink.
     *
     * Static callback invoked by GStreamer when a new sample is available.
     *
     * @param appsink The GStreamer appsink element.
     * @param data Pointer to the GStreamerVideoPlayer instance.
     * @return GstFlowReturn indicating the result of the operation.
     */
    static GstFlowReturn onNewSample(GstAppSink* appsink, void* data);

    /**
     * @brief Cleans up the video playback context.
     *
     * Frees all GStreamer and SDL resources associated with the context.
     */
    void cleanupContext();

    /**
     * @brief Gets the current media file path.
     *
     * @return The current media file path.
     */
    std::string getCurrentPath() const;

    /**
     * @brief Handles new pads added by the decodebin.
     *
     * Static callback invoked by GStreamer when a new pad is added.
     *
     * @param decodebin The GStreamer decodebin element.
     * @param pad The new pad added.
     * @param data Pointer to the GStreamerVideoPlayer instance.
     */
    static void onPadAdded(GstElement* decodebin, GstPad* pad, gpointer data);

    /**
     * @brief Links a video pad to the pipeline.
     *
     * Sets up the video processing chain (videoconvert, videoscale, appsink).
     *
     * @param pad The video pad to link.
     */
    void linkVideoPad(GstPad* pad);

    /**
     * @brief Links an audio pad to the pipeline.
     *
     * Sets up the audio processing chain (audioconvert, volume, audiosink).
     *
     * @param pad The audio pad to link.
     */
    void linkAudioPad(GstPad* pad);
};

#endif // GSTREAMER_PLAYER_H