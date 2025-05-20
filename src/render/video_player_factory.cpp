#include "render/video_player_factory.h"
#include "render/vlc_player.h"
#include "render/opengl_player.h"
#include "config/iconfig_service.h"
#include "config/settings.h"
#include "utils/logging.h"

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

    std::string videoBackend = "vlc"; // Default
    if (configService) {
        const Settings& settings = configService->getSettings();
        videoBackend = settings.videoBackend.empty() ? "vlc" : settings.videoBackend;
        LOG_DEBUG("VideoPlayerFactory: Requested videoBackend=" << videoBackend);
    } else {
        LOG_DEBUG("VideoPlayerFactory: No configService provided, defaulting to vlc");
    }

    if (videoBackend == "vlc") {
        auto player = std::make_unique<VlcVideoPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created VlcVideoPlayer for path=" << path);
            return player;
        }
        LOG_ERROR("VideoPlayerFactory: Failed to setup VLC video player for path=" << path);
        return nullptr;
    } else if (videoBackend == "opengl") {
        auto player = std::make_unique<OpenGLPlayer>();
        if (player->setup(renderer, path, width, height)) {
            LOG_DEBUG("VideoPlayerFactory: Created OpenGLPlayer for path=" << path);
            return player;
        }
        LOG_ERROR("VideoPlayerFactory: Failed to setup OpenGL video player for path=" << path);
        return nullptr;
    }

    LOG_DEBUG("VideoPlayerFactory: Unsupported videoBackend=" << videoBackend << ", falling back to vlc");
    auto player = std::make_unique<VlcVideoPlayer>();
    if (player->setup(renderer, path, width, height)) {
        LOG_DEBUG("VideoPlayerFactory: Created VlcVideoPlayer (fallback) for path=" << path);
        return player;
    }
    LOG_ERROR("VideoPlayerFactory: Failed to setup VLC video player (fallback) for path=" << path);
    return nullptr;
}