#include "capture/screenshot_capture.h"
#include "utils/logging.h"
#include <SDL2/SDL.h>      // Added for SDL_Window and SDL_RaiseWindow
#include <sstream>         // Added for std::ostringstream
#include <thread>
#include <unistd.h>        // Already included, kept for clarity
#include <sys/stat.h>

ScreenshotCapture::ScreenshotCapture(const std::string& exeDir) : exeDir_(exeDir) {}

void ScreenshotCapture::captureAllScreenshots(const std::string& tableImage, const std::string& backglassImage,
                                              const std::string& dmdImage, SDL_Window* window) {
    std::vector<std::thread> threads;
    threads.emplace_back([this, tableImage]() {
        captureScreenshot("Visual Pinball Player", tableImage);
    });

    if (isWindowVisible("B2SBackglass")) {
        threads.emplace_back([this, backglassImage]() {
            captureScreenshot("B2SBackglass", backglassImage);
        });
    } else {
        LOG_DEBUG("Warning: Backglass window not visible.");
    }

    std::string dmdWindows[] = {"FlexDMD", "PinMAME", "B2SDMD", "PUPDMD", "PUPFullDMD"};
    bool dmdCaptured = false;
    for (const auto& dmd : dmdWindows) {
        if (isWindowVisible(dmd)) {
            threads.emplace_back([this, dmd, dmdImage]() {
                captureScreenshot(dmd, dmdImage);
            });
            dmdCaptured = true;
            break;
        }
    }
    if (!dmdCaptured) {
        LOG_DEBUG("Warning: No visible DMD window detected.");
    }

    for (auto& thread : threads) {
        if (thread.joinable()) thread.join();
    }

    SDL_RaiseWindow(window);
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Warning: Failed to refocus VPX Screenshot window.");
    }
}

void ScreenshotCapture::captureScreenshot(const std::string& windowName, const std::string& outputPath) {
    std::string cmd = "xdotool search --name " + shellEscape(windowName) + " | head -n 1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_DEBUG("Error: Failed to run xdotool search for " << windowName);
        return;
    }
    char buffer[128];
    std::string windowId;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        windowId = buffer;
        windowId.erase(windowId.find_last_not_of(" \t\r\n") + 1);
    }
    pclose(pipe);

    if (windowId.empty()) {
        LOG_DEBUG("Warning: Window '" << windowName << "' not found.");
        return;
    }

    cmd = "xdotool windowactivate " + windowId + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Warning: Failed to activate window " << windowName);
    }
    cmd = "xdotool windowraise " + windowId + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Warning: Failed to raise window " << windowName);
    }
    usleep(400000);

    cmd = "mkdir -p " + shellEscape(outputPath.substr(0, outputPath.find_last_of('/')));
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Error: Failed to create directory for " << outputPath);
        return;
    }
    cmd = "import -window " + windowId + " " + shellEscape(outputPath);
    if (std::system(cmd.c_str()) == 0) {
        LOG_DEBUG("Saved screenshot to " << outputPath);
    } else {
        LOG_DEBUG("Error: Failed to save screenshot to " << outputPath);
    }
}

bool ScreenshotCapture::isWindowVisible(const std::string& title) {
    std::string cmd = "xdotool search --name \"" + title + "\" getwindowname >/dev/null 2>&1";
    int ret = std::system(cmd.c_str());
    bool visible = (ret == 0);
    LOG_DEBUG("X11/Wayland check for '" << title << "': " << (visible ? "visible" : "not visible"));
    return visible;
}

std::string ScreenshotCapture::shellEscape(const std::string& str) {
    std::ostringstream escaped;
    escaped << "\"";
    for (char c : str) {
        if (c == '"' || c == '\\') escaped << "\\";
        escaped << c;
    }
    escaped << "\"";
    return escaped.str();
}