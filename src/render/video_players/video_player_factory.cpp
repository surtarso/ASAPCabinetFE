/**
 * @file video_player_factory.cpp
 * @brief Implementation of the VideoPlayerFactory class for creating video players with different backends.
 */

#include "video_player_factory.h"
#include "dummy_player.h"
#include "sdl_draw/alternate_media_player.h"
#include "default_media_player.h"
#if !defined(__APPLE__)  && !defined(BUILDING_FLATPAK)
    #include "vlc/vlc_player.h"
#endif
#include "ffmpeg/ffmpeg_player.h"
#include "config/iconfig_service.h"
#include "config/settings.h"
#include "log/logging.h"
#include <map>
#include <string>

enum class VideoBackendType {
    VLC,
    FFMPEG,
    NOVIDEO,  // dummy player
    NOMEDIA,  // for default no media animation
    ANIMATED, // for default generated animations
    UNKNOWN
};

VideoBackendType getVideoBackendType(const std::string& backendName) {
    static const std::map<std::string, VideoBackendType> backendMap = {
        {"vlc", VideoBackendType::VLC},
        {"ffmpeg", VideoBackendType::FFMPEG},
        {"novideo", VideoBackendType::NOVIDEO},
        {"default", VideoBackendType::NOMEDIA},
        {"stats", VideoBackendType::ANIMATED}
    };

    auto it = backendMap.find(backendName);
    return it != backendMap.end() ? it->second : VideoBackendType::UNKNOWN;
}

std::unique_ptr<IVideoPlayer> VideoPlayerFactory::createVideoPlayer(
    SDL_Renderer* renderer,
    const std::string& path,
    int width,
    int height,
    IConfigService* configService) {

    if (!renderer || path.empty() || width <= 0 || height <= 0) {
        LOG_ERROR("Invalid parameters - renderer=" + std::to_string(reinterpret_cast<uintptr_t>(renderer)) +
                  ", path=" + path + ", width=" + std::to_string(width) + ", height=" + std::to_string(height));
        return nullptr;
    }

    std::string videoBackendStr = "ffmpeg";
    if (configService) {
        const Settings& settings = configService->getSettings();
        videoBackendStr = settings.videoBackend.empty() ? "ffmpeg" : settings.videoBackend;
        LOG_DEBUG("Requested videoBackend=" + videoBackendStr);
    } else {
        LOG_DEBUG("No configService provided, defaulting to ffmpeg");
    }

    VideoBackendType backendType = getVideoBackendType(videoBackendStr);

    std::unique_ptr<IVideoPlayer> player = nullptr;

    switch (backendType) {
        case VideoBackendType::NOVIDEO:
            player = std::make_unique<DummyVideoPlayer>();
            if (player->setup(renderer, path, width, height)) {
                LOG_DEBUG("Created DummyVideoPlayer for path=" + path);
                return player;
            }
            LOG_ERROR("Failed to setup Dummy video player for path=" + path);
            return nullptr;

    #if !defined(__APPLE__) && !defined(BUILDING_FLATPAK)
        case VideoBackendType::VLC:
            player = std::make_unique<VlcVideoPlayer>();
            break;
    #endif

        case VideoBackendType::FFMPEG:
            player = std::make_unique<FFmpegPlayer>();
            break;
        case VideoBackendType::NOMEDIA:
            player = std::make_unique<DefaultMediaPlayer>(renderer, width, height, configService->getSettings().fontPath, "");
            if (player->setup(renderer, path, width, height)) {
                LOG_DEBUG("Created DefaultMediaPlayer for path=" + path);
                return player;
            }
            LOG_ERROR("Failed to setup DefaultMediaPlayer video player for path=" + path);
            return nullptr;
        case VideoBackendType::ANIMATED:
            player = std::make_unique<AlternativeMediaPlayer>(renderer, width, height, configService->getSettings().fontPath, "", "");
            if (player->setup(renderer, path, width, height)) {
                LOG_DEBUG("Created AlternativeMediaPlayer for path=" + path);
                return player;
            }
            LOG_ERROR("Failed to setup AlternativeMediaPlayer video player for path=" + path);
            return nullptr;
        case VideoBackendType::UNKNOWN:
        default:
            LOG_DEBUG("Unsupported videoBackend=" + videoBackendStr + ", attempting FFmpeg fallback.");
            player = std::make_unique<FFmpegPlayer>();
            break;
    }

    if (player) {
        LOG_DEBUG("Attempting setup for path=" + path + ", width=" + std::to_string(width) + ", height=" + std::to_string(height));
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("Successfully created player for path=" + path + " with backend=" + videoBackendStr);
            return player;
        } else {
            LOG_ERROR("Failed to setup " + videoBackendStr + " player for path=" + path);
        }
    }

    LOG_ERROR("Failed to setup any video player for path=" + path);
    return nullptr;
}

std::unique_ptr<IVideoPlayer> VideoPlayerFactory::createDefaultMediaPlayer(
    SDL_Renderer* renderer,
    int width,
    int height,
    std::string fontPath,
    std::string screenName)
{
    if (!renderer || width <= 0 || height <= 0) {
        LOG_ERROR("Invalid parameters for createDefaultMediaPlayer()");
        return nullptr;
    }

    auto player = std::make_unique<DefaultMediaPlayer>(renderer, width, height, fontPath, screenName);

    // Use empty path — DefaultMediaPlayer internally knows to use your animated fallback
    if (!player->setup(renderer, "", width, height)) {
        LOG_ERROR("DefaultMediaPlayer setup failed in createDefaultMediaPlayer()");
        return nullptr;
    }

    return player;
}

std::unique_ptr<IVideoPlayer> VideoPlayerFactory::createAlternativeMediaPlayer(
    SDL_Renderer* renderer,
    int width,
    int height,
    std::string fontPath,
    std::string screenName,
    std::string displayText,
    DmdSDLRenderer* sharedDmdRenderer)
{
    if (!renderer || width <= 0 || height <= 0) {
        LOG_ERROR("Invalid parameters for createAlternativeMediaPlayer()");
        return nullptr;
    }

    auto player = std::make_unique<AlternativeMediaPlayer>(renderer, width, height, fontPath, screenName, displayText, sharedDmdRenderer);

    // Use empty path — DefaultMediaPlayer internally knows to use your animated fallback
    if (!player->setup(renderer, "", width, height)) {
        LOG_ERROR("AlternativeMediaPlayer setup failed in createAlternativeMediaPlayer()");
        return nullptr;
    }

    return player;
}
