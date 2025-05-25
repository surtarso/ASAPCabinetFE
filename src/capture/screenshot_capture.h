/**
 * @file screenshot_capture.h
 * @brief Defines the ScreenshotCapture class for capturing screenshots in ASAPCabinetFE.
 *
 * This header provides the ScreenshotCapture class, which captures screenshots of
 * specific windows (e.g., playfield, backglass, DMD) during screenshot mode. It uses
 * the executable directory for output paths and interacts with SDL windows for capture.
 */

#ifndef SCREENSHOT_CAPTURE_H
#define SCREENSHOT_CAPTURE_H

#include <string>
#include <vector>
#include <SDL2/SDL.h>

/**
 * @class ScreenshotCapture
 * @brief Captures screenshots of specific windows.
 *
 * This class handles the capture of screenshots for designated windows, saving them
 * to specified output paths. It is used by ScreenshotManager to capture playfield,
 * backglass, and DMD images during screenshot mode, using the executable directory
 * for file path resolution.
 */
class ScreenshotCapture {
public:
    /**
     * @brief Constructs a ScreenshotCapture instance.
     *
     * Initializes the capture handler with the executable directory for output paths.
     *
     * @param exeDir The executable directory for resolving output file paths.
     */
    ScreenshotCapture(const std::string& exeDir);

    /**
     * @brief Captures screenshots for all specified windows.
     *
     * Captures screenshots of the playfield, backglass, and DMD windows, saving them
     * to the provided output paths, using the provided SDL window for context.
     *
     * @param playfieldImage The output path for the playfield screenshot.
     * @param backglassImage The output path for the backglass screenshot.
     * @param dmdImage The output path for the DMD screenshot.
     * @param window The SDL window associated with the capture process.
     */
    void captureAllScreenshots(const std::string& playfieldImage, const std::string& backglassImage,
                               const std::string& dmdImage, SDL_Window* window);

    /**
     * @brief Checks if a window is visible.
     *
     * Determines if the window with the specified title is currently visible.
     *
     * @param title The title of the window to check.
     * @return True if the window is visible, false otherwise.
     */
    bool isWindowVisible(const std::string& title);

private:
    std::string exeDir_; ///< Executable directory for resolving output file paths.

    /**
     * @brief Captures a screenshot of a specific window.
     *
     * Saves a screenshot of the window with the given name to the specified output path.
     *
     * @param windowName The name (title) of the window to capture.
     * @param outputPath The file path to save the screenshot.
     */
    void captureScreenshot(const std::string& windowName, const std::string& outputPath);

    /**
     * @brief Escapes a string for safe shell usage.
     *
     * Escapes special characters in the string to prevent shell injection.
     *
     * @param str The input string to escape.
     * @return The escaped string.
     */
    std::string shellEscape(const std::string& str);
};

#endif // SCREENSHOT_CAPTURE_H