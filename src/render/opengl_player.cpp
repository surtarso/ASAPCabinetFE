#include "opengl_player.h"
#include "utils/logging.h"

bool OpenGLPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    LOG_DEBUG("OpenGLPlayer: Mock setup called - renderer=" << renderer << ", path=" << path
              << ", width=" << width << ", height=" << height);
    return true; // Simulate successful setup
}

void OpenGLPlayer::play() {
    LOG_DEBUG("OpenGLPlayer: Mock play called");
    isPlaying_ = true;
}

void OpenGLPlayer::stop() {
    LOG_DEBUG("OpenGLPlayer: Mock stop called");
    isPlaying_ = false;
}

void OpenGLPlayer::update() {
    LOG_DEBUG("OpenGLPlayer: Mock update called");
}

SDL_Texture* OpenGLPlayer::getTexture() const {
    LOG_DEBUG("OpenGLPlayer: Mock getTexture called, returning nullptr");
    return nullptr; // Mock returns no texture
}

bool OpenGLPlayer::isPlaying() const {
    LOG_DEBUG("OpenGLPlayer: Mock isPlaying called, returning " << (isPlaying_ ? "true" : "false"));
    return isPlaying_;
}