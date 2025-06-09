#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <SDL2/SDL.h>
#include <chrono>
#include <string>

// Forward declarations for FFmpeg structs
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class FFmpegPlayer; // Forward declaration

/**
 * @file video_decoder.h
 * @brief Defines the VideoDecoder class for video decoding in ASAPCabinetFE.
 *
 * This header provides the VideoDecoder class, which handles video decoding using
 * FFmpeg and renders frames to an SDL texture. It integrates with FFmpegPlayer for
 * playback control and supports error handling and frame synchronization.
 */

/**
 * @class VideoDecoder
 * @brief Decodes and renders video frames using FFmpeg and SDL.
 *
 * This class manages the decoding of video streams from an AVFormatContext, converts
 * frames to RGB format using SwsContext, and updates an SDL texture for rendering.
 * It works in conjunction with FFmpegPlayer and handles playback timing and cleanup.
 */
class VideoDecoder {
public:
    /**
     * @brief Constructs a VideoDecoder instance.
     *
     * Initializes the decoder with a reference to the parent FFmpegPlayer.
     *
     * @param player Pointer to the FFmpegPlayer instance.
     */
    VideoDecoder(FFmpegPlayer* player);

    /**
     * @brief Destroys the VideoDecoder instance and cleans up resources.
     *
     * Ensures all allocated FFmpeg and SDL resources are properly freed.
     */
    ~VideoDecoder();

    /**
     * @brief Sets up the video decoder with format context and renderer.
     *
     * Configures the decoder with the video stream, codec, and rendering parameters.
     *
     * @param formatContext Pointer to the AVFormatContext containing the video stream.
     * @param renderer Pointer to the SDL_Renderer for texture creation.
     * @param width Desired width of the output texture.
     * @param height Desired height of the output texture.
     * @return True on successful setup, false otherwise.
     */
    bool setup(AVFormatContext* formatContext, SDL_Renderer* renderer, int width, int height);

    /**
     * @brief Starts video playback.
     *
     * Resets playback timing to synchronize with the video stream.
     */
    void play();

    /**
     * @brief Stops video playback.
     *
     * Flushes the decoder buffers to prepare for a new playback session.
     */
    void stop();

    /**
     * @brief Updates the video frame based on playback timing.
     *
     * Decodes and renders the next frame if the video clock aligns with elapsed time.
     */
    void update();

    /**
     * @brief Retrieves the current video texture.
     *
     * @return Pointer to the SDL_Texture, or nullptr if not initialized.
     */
    SDL_Texture* getTexture() const;

    /**
     * @brief Decodes the next video frame.
     *
     * Attempts to read and decode a video packet into a frame.
     *
     * @return True if a valid frame was decoded, false otherwise.
     */
    bool decodeVideoFrame();

    /**
     * @brief Updates the SDL texture with the current RGB frame data.
     *
     * Copies the decoded frame data to the texture for rendering.
     */
    void updateTexture();

    /**
     * @brief Flushes the decoder buffers.
     *
     * Clears any pending frames in the codec context.
     */
    void flush();

    /**
     * @brief Resets playback timing variables.
     *
     * Sets the video clock and timestamps to initial states.
     */
    void resetPlaybackTimes();

private:
    friend class FFmpegPlayer; // Allow FFmpegPlayer to access private members
    FFmpegPlayer* player_;              ///< Pointer to the parent FFmpegPlayer instance.
    AVCodecContext* videoCodecContext_; ///< FFmpeg codec context for video decoding.
    SDL_Renderer* renderer_;            ///< SDL renderer for texture creation.
    int width_;                         ///< Width of the output texture.
    int height_;                        ///< Height of the output texture.
    AVFrame* videoFrame_;               ///< FFmpeg frame for raw video data.
    AVFrame* rgbFrame_;                 ///< FFmpeg frame for converted RGB data.
    AVPacket* videoPacket_;             ///< FFmpeg packet for video data.
    SwsContext* swsContext_;            ///< FFmpeg scaling context for format conversion.
    int videoStreamIndex_;              ///< Index of the video stream in the format context.
    uint8_t* rgbBuffer_;                ///< Buffer for RGB frame data.
    SDL_Texture* texture_;              ///< SDL texture for rendering the video.
    double videoClock_;                 ///< Current video playback time in seconds.
    std::chrono::high_resolution_clock::time_point lastFrameTime_; ///< Timestamp of the last frame.
    std::chrono::high_resolution_clock::time_point playbackStartTime_; ///< Timestamp of playback start.
    bool needsReset_;                   ///< Flag indicating if playback needs to reset.

    /**
     * @brief Cleans up all allocated resources.
     *
     * Frees FFmpeg and SDL resources, resetting member pointers.
     */
    void cleanup();
};

#endif // VIDEO_DECODER_H