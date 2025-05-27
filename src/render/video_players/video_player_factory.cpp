/**
 * @file video_player_factory.cpp
 * @brief Implementation of the VideoPlayerFactory class for creating video player instances.
 *
 * This file contains the implementation of the VideoPlayerFactory's createVideoPlayer method,
 * which constructs VlcVideoPlayer or FFmpegPlayer instances based on the configured video backend.
 * It handles parameter validation, backend selection, and fallback logic.
 */

#include "video_player_factory.h"
#include "dummy_player.h"
#include "render/video_players/vlc/vlc_player.h"
#include "render/video_players/ffmpeg/ffmpeg_player.h"
#include "render/video_players/gstreamer/gstreamer_player.h"
#include "config/iconfig_service.h"
#include "config/settings.h"
#include "utils/logging.h"

/**
 * @brief Creates a video player instance based on the configured backend.
 *
 * Constructs a video player (either VlcVideoPlayer or FFmpegPlayer) depending on the
 * video backend specified in the configuration settings. If the backend is unsupported
 * or initialization fails, it falls back to VLC. Returns nullptr if all attempts fail
 * or if invalid parameters are provided.
 *
 * @param renderer The SDL renderer used for video rendering.
 * @param path The file path to the video to be played.
 * @param width The width of the video display area in pixels.
 * @param height The height of the video display area in pixels.
 * @param configService Pointer to the configuration service for accessing settings.
 * @return A unique_ptr to an IVideoPlayer instance, or nullptr on failure.
 */
std::unique_ptr<IVideoPlayer> VideoPlayerFactory::createVideoPlayer(
    SDL_Renderer* renderer,
    const std::string& path,
    int width,
    int height,
    IConfigService* configService) {
    // Validate input parameters
    if (!renderer || path.empty() || width <= 0 || height <= 0) {
        LOG_ERROR("VideoPlayerFactory: Invalid parameters - renderer=" << renderer
                  << ", path=" << path << ", width=" << width << ", height=" << height);
        return nullptr;
    }

    // Determine video backend from configuration
    std::string videoBackend = "vlc"; // Default
    if (configService) {
        const Settings& settings = configService->getSettings();
        videoBackend = settings.videoBackend.empty() ? "vlc" : settings.videoBackend;
        LOG_DEBUG("VideoPlayerFactory: Requested videoBackend=" << videoBackend);
    } else {
        LOG_DEBUG("VideoPlayerFactory: No configService provided, defaulting to vlc");
    }

    // Create DUMMY player if specified
    if (videoBackend == "novideo") {
        auto player = std::make_unique<DummyVideoPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created DummyVideoPlayer for path=" << path);
            return player;
        }
        LOG_ERROR("VideoPlayerFactory: Failed to setup Dummy video player for path=" << path);
        return nullptr;
    } 
    // Create VLC player if specified
    if (videoBackend == "vlc") {
        auto player = std::make_unique<VlcVideoPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created VlcVideoPlayer for path=" << path);
            return player;
        }
        LOG_ERROR("VideoPlayerFactory: Failed to setup VLC video player for path=" << path);
        return nullptr;
    } 
    // Create FFmpeg player if specified
    else if (videoBackend == "ffmpeg") {
        auto player = std::make_unique<FFmpegPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created FFmpegPlayer for path=" << path);
            return player;
        }
        LOG_ERROR("VideoPlayerFactory: Failed to setup FFmpeg video player for path=" << path);
        return nullptr;
    }
    // Create DUMMY player if specified
    else if (videoBackend == "gstreamer") {
        auto player = std::make_unique<GStreamerVideoPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created GStreamerVideoPlayer for path=" << path);
            return player;
        }
        LOG_ERROR("VideoPlayerFactory: Failed to setup GStreamer video player for path=" << path);
        return nullptr;
    }

    // Fallback to VLC for unsupported backends
    LOG_DEBUG("VideoPlayerFactory: Unsupported videoBackend=" << videoBackend << ", falling back to vlc");
    auto player = std::make_unique<VlcVideoPlayer>();
    if (player->setup(renderer, path, width, height)) {
        LOG_DEBUG("VideoPlayerFactory: Created VlcVideoPlayer (fallback) for path=" << path);
        return player;
    }
    LOG_ERROR("VideoPlayerFactory: Failed to setup VLC video player (fallback) for path=" << path);
    return nullptr;
}