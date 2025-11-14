/**
 * @file screenshot_manager.cpp
 * @brief Implementation of the ScreenshotManager class for handling screenshot capture workflows.
 */

#include "capture/screenshot_manager.h"
#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "sound/isound_manager.h"
#include "log/logging.h"
#include <SDL2/SDL.h>
#include <string>
#include <unistd.h>

ScreenshotManager::ScreenshotManager(const std::string& exeDir, IConfigService* configManager,
                                    IKeybindProvider* keybindProvider, ISoundManager* soundManager)
    : exeDir_(exeDir), configManager_(configManager), keybindProvider_(keybindProvider),
      soundManager_(soundManager), window_(configManager, keybindProvider),
      capture_(exeDir), process_(exeDir, configManager), isRunning_(false) {}

void ScreenshotManager::launchScreenshotMode(const std::string& vpxFile) {
    LOG_DEBUG("Starting launchScreenshotMode for: " + vpxFile + " with exeDir: " + exeDir_);
    const Settings& settings = configManager_->getSettings();
    std::string tableFolder = vpxFile.substr(0, vpxFile.find_last_of('/'));
    std::string playfieldImage = tableFolder + "/" + settings.customPlayfieldImage;
    std::string backglassImage = tableFolder + "/" + settings.customBackglassImage;
    std::string dmdImage = tableFolder + "/" + settings.customDmdImage;

    if (!process_.launchVPX(vpxFile)) {
        LOG_ERROR("Failed to launch VPX, aborting screenshot mode.");
        return;
    }

    LOG_INFO("Waiting " + std::to_string(settings.screenshotWait) + "s for VPX to fully initialize");
    sleep(settings.screenshotWait);

    if (!capture_.isWindowVisible("Visual Pinball Player")) {
        LOG_ERROR("Aborting screenshot mode - VPX window not found after " + std::to_string(settings.screenshotWait) + "s");
        process_.terminateVPX();
        return;
    }
    LOG_INFO("VPX playfield window detected after " + std::to_string(settings.screenshotWait) + "s.");

    LOG_DEBUG("Waiting an additional 1s for VPX to settle");
    sleep(1);

    if (!window_.initialize(215, 35)) {
        LOG_ERROR("Failed to initialize screenshot window, aborting.");
        process_.terminateVPX();
        return;
    }

    isRunning_ = true;
    SDL_Event event;
    while (isRunning_) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                LOG_DEBUG("Quit via SDL_QUIT");
                isRunning_ = false;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_KeyboardEvent keyEvent = event.key;
                if (keybindProvider_->isAction(keyEvent, "Screenshot Key")) {
                    LOG_INFO("Capture key pressed");
                    if (soundManager_) soundManager_->playUISound("screenshot_take");
                    capture_.captureAllScreenshots(playfieldImage, backglassImage, dmdImage, window_.getWindow());
                } else if (keybindProvider_->isAction(keyEvent, "Screenshot Quit")) {
                    LOG_WARN("Quit key pressed");
                    isRunning_ = false;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                SDL_Rect button = {0, 0, 215, 35};
                if (x >= button.x && x <= button.x + button.w && y >= button.y && y <= button.y + button.h) {
                    LOG_INFO("Capturing screenshots with mouse click...");
                    soundManager_->playUISound("screenshot_take");
                    capture_.captureAllScreenshots(playfieldImage, backglassImage, dmdImage, window_.getWindow());
                }
            }
        }
        window_.render();
        SDL_Delay(10);
    }

    process_.terminateVPX();
    window_.cleanup();
    while (SDL_PollEvent(&event)) {
        LOG_DEBUG("Draining leftover event: type " + std::to_string(event.type));
    }
    isRunning_ = false;
    LOG_INFO("Screenshot mode exited");
}
