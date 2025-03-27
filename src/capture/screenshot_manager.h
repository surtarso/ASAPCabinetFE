#ifndef SCREENSHOT_MANAGER_H
#define SCREENSHOT_MANAGER_H

#include <string>
#include <SDL2/SDL.h>
#include "config/settings_manager.h"
#include "keybinds/keybind_manager.h" // Updated include

class ScreenshotManager {
public:
    ScreenshotManager(const std::string& exeDir, SettingsManager* configManager, KeybindManager* keybindManager);
    void launchScreenshotMode(const std::string& vpxFile);
    void showHelpWindow(SDL_Window*& helpWindow, SDL_Renderer*& helpRenderer);

private:
    void captureScreenshot(const std::string& windowName, const std::string& outputPath);
    void captureAllScreenshots(const std::string& tableImage, const std::string& backglassImage,
                               const std::string& dmdImage, SDL_Window* window);
    bool isWindowVisibleLog(const std::string& title);
    std::string shellEscape(const std::string& str);
    std::string keycodeToString(SDL_Keycode key);

    std::string exeDir_;
    std::string vpxLogFile;
    SettingsManager* configManager_;
    KeybindManager* keybindManager_; // Updated member
};

#endif // SCREENSHOT_MANAGER_H