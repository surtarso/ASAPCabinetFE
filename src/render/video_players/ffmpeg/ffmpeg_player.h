#ifndef FFMPEG_PLAYER_H
#define FFMPEG_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL2/SDL.h>
#include <string>

// Forward declarations
struct AVFormatContext;
class VideoDecoder;
class AudioDecoder;

/**
 * @file ffmpeg_player.h
 * @brief Defines the FFmpegPlayer class for video and audio playback in ASAPCabinetFE.
 *
 * This header provides the FFmpegPlayer class, which implements the IVideoPlayer
 * interface using FFmpeg for decoding and playback of video and audio streams.
 * It manages VideoDecoder and AudioDecoder instances for rendering and sound.
 */

/**
 * @class FFmpegPlayer
 * @brief Implements video and audio playback using FFmpeg.
 *
 * This class extends IVideoPlayer to handle media file playback with FFmpeg,
 * managing format context, video, and audio decoding. It provides methods for
 * setup, playback control, and volume management, with cleanup handled via RAII.
 */
class FFmpegPlayer : public IVideoPlayer {
public:
    /**
     * @brief Constructs an FFmpegPlayer instance.
     *
     * Initializes with null pointers for decoders and format context.
     */
    FFmpegPlayer();

    /**
     * @brief Destroys the FFmpegPlayer instance and cleans up resources.
     *
     * Ensures all allocated resources, including decoders and format context, are freed.
     */
    ~FFmpegPlayer() override;

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
     * Halts video and audio playback and resets stream positions if applicable.
     */
    void stop() override;

    /**
     * @brief Updates the playback state.
     *
     * Calls update on video and audio decoders to process the next frame or audio sample.
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
     * @param volume Volume level (0.0 to 1.0).
     */
    void setVolume(float volume) override;

    /**
     * @brief Toggles audio mute state.
     *
     * @param mute True to mute, false to unmute.
     */
    void setMute(bool mute) override;

    /**
     * @brief Seeks to the beginning of the specified stream.
     *
     * Used by decoders to reset playback position.
     *
     * @param streamIndex Index of the stream to seek (-1 for all audio/video streams).
     */
    void seekToBeginning(int streamIndex);

    /**
     * @brief Gets the format context.
     *
     * Provides access to the AVFormatContext for decoder use.
     *
     * @return Pointer to the AVFormatContext.
     */
    AVFormatContext* getFormatContext() const { return formatContext_; }

private:
    SDL_Renderer* renderer_;        ///< SDL renderer for texture creation.
    std::string path_;              ///< Path to the media file.
    int width_;                     ///< Width of the video output.
    int height_;                    ///< Height of the video output.
    bool isPlaying_;                ///< Flag indicating playback state.
    AVFormatContext* formatContext_; ///< FFmpeg format context for the media file.
    VideoDecoder* videoDecoder_;    ///< Pointer to the video decoding instance.
    AudioDecoder* audioDecoder_;    ///< Pointer to the audio decoding instance.

    /**
     * @brief Cleans up all allocated resources.
     *
     * Frees format context and resets member variables.
     */
    void cleanup();
};

#endif // FFMPEG_PLAYER_H