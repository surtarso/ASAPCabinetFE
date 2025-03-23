#ifndef SCREENSHOT_UTILS_H
#define SCREENSHOT_UTILS_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class ScreenshotManager {
public:
    ScreenshotManager(const std::string& exeDir);  // Constructor takes exeDir
    void launchScreenshotMode(const std::string& vpxFile);  // Main entry for screenshot mode
    void captureScreenshot(const std::string& windowName, const std::string& outputPath);  // Capture single screenshot
    void captureAllScreenshots(const std::string& tableImage, const std::string& backglassImage, 
                               const std::string& dmdImage, SDL_Window* window);  // Capture all needed screenshots
    std::string keycodeToString(SDL_Keycode key);
private:
    std::string vpxLogFile;  // Member variable for log path
    std::string shellEscape(const std::string& str);  // Escape shell strings
    bool isWindowVisibleLog(const std::string& title);  // Check log for window visibility
};

#endif // SCREENSHOT_UTILS_H