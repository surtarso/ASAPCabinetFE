#ifndef SCREENSHOT_CAPTURE_H
#define SCREENSHOT_CAPTURE_H

#include <string>
#include <vector>
#include <SDL2/SDL.h>  // Added for SDL_Window

class ScreenshotCapture {
public:
    ScreenshotCapture(const std::string& exeDir);
    void captureAllScreenshots(const std::string& playfieldImage, const std::string& backglassImage,
                               const std::string& dmdImage, SDL_Window* window);
    bool isWindowVisible(const std::string& title);  // Moved to public

private:
    void captureScreenshot(const std::string& windowName, const std::string& outputPath);
    std::string shellEscape(const std::string& str);
    std::string exeDir_;
};

#endif // SCREENSHOT_CAPTURE_H