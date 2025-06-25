#include "video_player_factory.h"
#include "dummy_player.h"
#include "vlc/vlc_player.h"
#include "ffmpeg/ffmpeg_player.h"
#include "config/iconfig_service.h"
#include "config/settings.h"
#include "log/logging.h"
#include <map>

enum class VideoBackendType {
    VLC,
    FFMPEG,
    NOVIDEO,
    UNKNOWN
};

VideoBackendType getVideoBackendType(const std::string& backendName) {
    static const std::map<std::string, VideoBackendType> backendMap = {
        {"vlc", VideoBackendType::VLC},
        {"ffmpeg", VideoBackendType::FFMPEG},
        {"novideo", VideoBackendType::NOVIDEO}
    };

    auto it = backendMap.find(backendName);
    if (it != backendMap.end()) {
        return it->second;
    }
    return VideoBackendType::UNKNOWN;
}

std::unique_ptr<IVideoPlayer> VideoPlayerFactory::createVideoPlayer(
    SDL_Renderer* renderer,
    const std::string& path,
    int width,
    int height,
    IConfigService* configService) {

    if (!renderer || path.empty() || width <= 0 || height <= 0) {
        LOG_ERROR("VideoPlayerFactory: Invalid parameters - renderer=" << renderer
                  << ", path=" << path << ", width=" << width << ", height=" << height);
        return nullptr;
    }

    std::string videoBackendStr = "vlc";
    if (configService) {
        const Settings& settings = configService->getSettings();
        videoBackendStr = settings.videoBackend.empty() ? "vlc" : settings.videoBackend;
        LOG_DEBUG("VideoPlayerFactory: Requested videoBackend=" << videoBackendStr);
    } else {
        LOG_DEBUG("VideoPlayerFactory: No configService provided, defaulting to vlc");
    }

    VideoBackendType backendType = getVideoBackendType(videoBackendStr);

    std::unique_ptr<IVideoPlayer> player = nullptr;

    switch (backendType) {
        case VideoBackendType::NOVIDEO:
            player = std::make_unique<DummyVideoPlayer>();
            if (player->setup(renderer, path, width, height)) {
                LOG_DEBUG("VideoPlayerFactory: Created DummyVideoPlayer for path=" << path);
                return player;
            }
            LOG_ERROR("VideoPlayerFactory: Failed to setup Dummy video player for path=" << path);
            return nullptr;
        case VideoBackendType::VLC:
            player = std::make_unique<VlcVideoPlayer>();
            break;
        case VideoBackendType::FFMPEG:
            player = std::make_unique<FFmpegPlayer>();
            break;
        case VideoBackendType::UNKNOWN:
        default:
            LOG_DEBUG("VideoPlayerFactory: Unsupported videoBackend=" << videoBackendStr << ", attempting VLC fallback.");
            player = std::make_unique<VlcVideoPlayer>();
            break;
    }

    if (player) {
        LOG_DEBUG("VideoPlayerFactory: Attempting setup for path=" << path << ", width=" << width << ", height=" << height);
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Successfully created player for path=" << path << " with backend=" << videoBackendStr);
            return player;
        } else {
            LOG_ERROR("VideoPlayerFactory: Failed to setup " << videoBackendStr << " player for path=" << path);
        }
    }

    LOG_ERROR("VideoPlayerFactory: Failed to setup any video player for path=" << path);
    return nullptr;
}