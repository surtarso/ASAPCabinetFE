#include "capture/screenshot_window.h"
#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "utils/logging.h"
#include <SDL2/SDL_ttf.h>
#include <unistd.h>

ScreenshotWindow::ScreenshotWindow(IConfigService* configManager, IKeybindProvider* keybindProvider)
    : configManager_(configManager), keybindProvider_(keybindProvider),
      window_(nullptr), renderer_(nullptr), font_(nullptr), textTexture_(nullptr) {}

ScreenshotWindow::~ScreenshotWindow() {
    if (textTexture_) SDL_DestroyTexture(textTexture_);
    if (font_) TTF_CloseFont(font_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
}

bool ScreenshotWindow::initialize(int width, int height) {
    const Settings& settings = configManager_->getSettings();

    // Log SDL version and video driver for debugging
    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    const char* videoDriver = SDL_GetCurrentVideoDriver();
    LOG_DEBUG("ScreenshotWindow: SDL Compiled Version: " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch);
    LOG_DEBUG("ScreenshotWindow: SDL Linked Version: " << (int)linked.major << "." << (int)linked.minor << "." << (int)linked.patch);
    LOG_DEBUG("ScreenshotWindow: Video Driver: " << (videoDriver ? videoDriver : "unknown"));

    // Use SDL_WINDOWPOS_CENTERED for x and y, and proper flags
    Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;
    // Only add SDL_WINDOW_ALWAYS_ON_TOP if not running on Wayland, as it may cause issues
    if (videoDriver && strcmp(videoDriver, "wayland") != 0) {
        windowFlags |= SDL_WINDOW_ALWAYS_ON_TOP;
    }
    LOG_DEBUG("ScreenshotWindow: Creating window with flags: 0x" << std::hex << windowFlags << std::dec
              << ", position: SDL_WINDOWPOS_CENTERED");

    window_ = SDL_CreateWindow("VPX Screenshot",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height, windowFlags);
    if (!window_) {
        LOG_ERROR("ScreenshotWindow: SDL_CreateWindow Error: " << SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        LOG_ERROR("ScreenshotWindow: SDL_CreateRenderer Error: " << SDL_GetError());
        return false;
    }

    font_ = TTF_OpenFont(settings.fontPath.c_str(), 14);
    if (!font_) {
        LOG_ERROR("ScreenshotWindow: TTF_OpenFont Error: " << TTF_GetError());
        return false;
    }

    SDL_Keycode screenshotKey = keybindProvider_->getKey("ScreenshotKey");
    SDL_Keycode screenshotQuit = keybindProvider_->getKey("ScreenshotQuit");
    buttonText_ = "'" + std::string(SDL_GetKeyName(screenshotKey)) + "' to Screenshot, '" +
                  std::string(SDL_GetKeyName(screenshotQuit)) + "' to Quit";

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font_, buttonText_.c_str(), white);
    if (!textSurface) {
        LOG_ERROR("ScreenshotWindow: TTF_RenderText_Solid Error: " << TTF_GetError());
        return false;
    }
    textTexture_ = SDL_CreateTextureFromSurface(renderer_, textSurface);
    SDL_FreeSurface(textSurface);
    if (!textTexture_) {
        LOG_ERROR("ScreenshotWindow: SDL_CreateTextureFromSurface Error: " << SDL_GetError());
        return false;
    }

    buttonRect_ = {0, 0, width, height};
    raiseAndFocus();
    return true;
}

void ScreenshotWindow::render() {
    SDL_SetRenderDrawColor(renderer_, 50, 50, 50, 255);
    SDL_RenderClear(renderer_);
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer_, &buttonRect_);
    SDL_Rect textRect = {buttonRect_.x + 10, buttonRect_.y + 10, 0, 0};
    SDL_QueryTexture(textTexture_, nullptr, nullptr, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer_, textTexture_, nullptr, &textRect);
    SDL_RenderPresent(renderer_);
}

void ScreenshotWindow::raiseAndFocus() {
    SDL_RaiseWindow(window_);
    SDL_SetWindowInputFocus(window_);
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    for (int i = 0; i < 5; i++) {
        if (std::system(cmd.c_str()) == 0) {
            LOG_INFO("ScreenshotWindow: Focus stolen to VPX Screenshot window after " << i << " attempts.");
            break;
        }
        LOG_DEBUG("ScreenshotWindow: Focus steal attempt " << i << " failed.");
        SDL_RaiseWindow(window_);
        usleep(1000000);
        if (i == 4) {
            LOG_INFO("ScreenshotWindow: Warning: Failed to steal focus to VPX Screenshot window after 5 attempts.");
        }
    }
}

void ScreenshotWindow::cleanup() {
    if (textTexture_) {
        SDL_DestroyTexture(textTexture_);
        textTexture_ = nullptr;
    }
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    LOG_INFO("ScreenshotWindow: ScreenshotWindow cleaned up.");
}