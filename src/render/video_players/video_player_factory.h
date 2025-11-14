/**
 * @file video_player_factory.h
 * @brief Defines the VideoPlayerFactory class for creating video player instances in ASAPCabinetFE.
 *
 * This header provides the VideoPlayerFactory class, which is responsible for creating
 * video player instances (implementing IVideoPlayer) based on the configured video backend.
 * It supports VLC and FFmpeg backends for rendering video content in the application.
 */

#ifndef VIDEO_PLAYER_FACTORY_H
#define VIDEO_PLAYER_FACTORY_H

#include "render/ivideo_player.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string>

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class VideoPlayerFactory
 * @brief Factory class for creating video player instances.
 *
 * This class provides a static method to create video player instances (VlcVideoPlayer or FFmpegPlayer)
 * based on the video backend specified in the configuration settings. It ensures the appropriate
 * player is initialized with the provided renderer, path, and dimensions.
 */
class VideoPlayerFactory {
public:
    /**
     * @brief Creates a video player instance based on the configured backend.
     *
     * Constructs a video player (either VlcVideoPlayer or FFmpegPlayer) depending on the
     * video backend specified in the configuration. Returns nullptr if initialization fails
     * or if invalid parameters are provided.
     *
     * @param renderer The SDL renderer used for video rendering.
     * @param path The file path to the video to be played.
     * @param width The width of the video display area in pixels.
     * @param height The height of the video display area in pixels.
     * @param configService Pointer to the configuration service for accessing settings.
     * @return A unique_ptr to an IVideoPlayer instance, or nullptr on failure.
     */
    static std::unique_ptr<IVideoPlayer> createVideoPlayer(SDL_Renderer* renderer,
                                                          const std::string& path,
                                                          int width, int height,
                                                          IConfigService* configService);

    static std::unique_ptr<IVideoPlayer> createDefaultMediaPlayer(
    SDL_Renderer* renderer,
    int width,
    int height);
};

#endif // VIDEO_PLAYER_FACTORY_H
