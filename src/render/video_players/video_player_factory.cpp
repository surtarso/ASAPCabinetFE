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
#include "log/logging.h"
#include <map> // Required for backend string to enum mapping

// ---

// Helper enum to represent video backend types
enum class VideoBackendType {
    VLC,
    FFMPEG,
    GSTREAMER,
    NOVIDEO,
    UNKNOWN // For unsupported or default
};

// ---

/**
 * @brief Converts a string backend name to its corresponding enum type.
 * @param backendName The string name of the video backend.
 * @return The corresponding VideoBackendType enum value.
 */
VideoBackendType getVideoBackendType(const std::string& backendName) {
    static const std::map<std::string, VideoBackendType> backendMap = {
        {"vlc", VideoBackendType::VLC},
        {"ffmpeg", VideoBackendType::FFMPEG},
        {"gstreamer", VideoBackendType::GSTREAMER},
        {"novideo", VideoBackendType::NOVIDEO}
    };

    auto it = backendMap.find(backendName);
    if (it != backendMap.end()) {
        return it->second;
    }
    return VideoBackendType::UNKNOWN;
}

// ---

/**
 * @brief Creates a video player instance based on the configured backend.
 *
 * Constructs a video player (e.g., VlcVideoPlayer, FFmpegPlayer, GStreamerVideoPlayer, or DummyVideoPlayer)
 * depending on the video backend specified in the configuration settings. If the backend is unsupported
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
    std::string videoBackendStr = "vlc"; // Default
    if (configService) {
        const Settings& settings = configService->getSettings();
        videoBackendStr = settings.videoBackend.empty() ? "vlc" : settings.videoBackend;
        LOG_DEBUG("VideoPlayerFactory: Requested videoBackend=" << videoBackendStr);
    } else {
        LOG_DEBUG("VideoPlayerFactory: No configService provided, defaulting to vlc");
    }

    // Convert string backend to enum for switch-case
    VideoBackendType backendType = getVideoBackendType(videoBackendStr);

    std::unique_ptr<IVideoPlayer> player = nullptr;

    // ---
    switch (backendType) {
        case VideoBackendType::NOVIDEO: {
            player = std::make_unique<DummyVideoPlayer>();
            if (player->setup(renderer, path, width, height)) {
                LOG_DEBUG("VideoPlayerFactory: Created DummyVideoPlayer for path=" << path);
                return player;
            }
            LOG_ERROR("VideoPlayerFactory: Failed to setup Dummy video player for path=" << path);
            return nullptr; // No fallback for novideo
        }
        case VideoBackendType::VLC: {
            player = std::make_unique<VlcVideoPlayer>();
            break; // Try to setup VLC
        }
        case VideoBackendType::FFMPEG: {
            player = std::make_unique<FFmpegPlayer>();
            break; // Try to setup FFmpeg
        }
        case VideoBackendType::GSTREAMER: {
            player = std::make_unique<GStreamerVideoPlayer>();
            break; // Try to setup GStreamer
        }
        case VideoBackendType::UNKNOWN:
        default: {
            LOG_DEBUG("VideoPlayerFactory: Unsupported videoBackend=" << videoBackendStr << ", attempting VLC fallback.");
            player = std::make_unique<VlcVideoPlayer>(); // Default/fallback to VLC
            break;
        }
    }

    // Attempt to setup the selected player (or fallback VLC)
    if (player && player->setup(renderer, path, width, height)) {
        LOG_DEBUG("VideoPlayerFactory: Successfully created player for path=" << path << " with backend=" << videoBackendStr);
        return player;
    }

    // If initial player setup failed, try NOVIDEO as a last resort fallback.
    // This handles cases where VLC, FFmpeg, GStreamer, or an unknown backend failed to initialize.
    if (backendType != VideoBackendType::NOVIDEO) {
        LOG_ERROR("VideoPlayerFactory: Failed to setup " << videoBackendStr << " player for path=" << path << ", attempting NOVIDEO fallback.");
        player = std::make_unique<VlcVideoPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created NOVIDEO (fallback) for path=" << path);
            return player;
        }
    }

    LOG_ERROR("VideoPlayerFactory: Failed to setup any video player for path=" << path);
    return nullptr;
}