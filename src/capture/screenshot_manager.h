#ifndef SCREENSHOT_MANAGER_H
#define SCREENSHOT_MANAGER_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class ConfigManager;
class InputManager;

class ScreenshotManager {
public:
    ScreenshotManager(const std::string& exeDir, ConfigManager* configManager, InputManager* inputManager);
    void launchScreenshotMode(const std::string& vpxFile);
    void captureScreenshot(const std::string& windowName, const std::string& outputPath);
    void captureAllScreenshots(const std::string& tableImage, const std::string& backglassImage, 
                               const std::string& dmdImage, SDL_Window* window);
    std::string keycodeToString(SDL_Keycode key);
private:
    std::string exeDir_;  // Add this to store base path
    std::string vpxLogFile;
    ConfigManager* configManager_;
    InputManager* inputManager_; // Add InputManager
    std::string shellEscape(const std::string& str);
    bool isWindowVisibleLog(const std::string& title);
    void showHelpWindow(SDL_Window*& helpWindow, SDL_Renderer*& helpRenderer); // Added declaration
};

#endif // SCREENSHOT_MANAGER_H