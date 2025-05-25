/**
 * @file screenshot_manager.h
 * @brief Defines the ScreenshotManager class for coordinating screenshot capture in ASAPCabinetFE.
 *
 * This header provides the ScreenshotManager class, which implements the IScreenshotManager
 * interface to manage screenshot mode. It launches a process, displays a UI via
 * ScreenshotWindow, captures screenshots with ScreenshotCapture, and provides audio
 * feedback via ISoundManager.
 */

#ifndef SCREENSHOT_MANAGER_H
#define SCREENSHOT_MANAGER_H

#include "capture/iscreenshot_manager.h"
#include "capture/screenshot_window.h"
#include "capture/screenshot_capture.h"
#include "capture/process_handler.h"
#include <string>

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class IKeybindProvider
 * @brief Interface for keybind providers (forward declaration).
 */
class IKeybindProvider;

/**
 * @class ISoundManager
 * @brief Interface for sound management (forward declaration).
 */
class ISoundManager;

/**
 * @class ScreenshotManager
 * @brief Manages screenshot capture mode.
 *
 * This class implements IScreenshotManager to coordinate screenshot capture by launching
 * a process (e.g., a VPX file), displaying a UI with ScreenshotWindow, and capturing
 * screenshots with ScreenshotCapture. It uses IConfigService for settings, IKeybindProvider
 * for inputs, and ISoundManager for audio feedback.
 */
class ScreenshotManager : public IScreenshotManager {
public:
    /**
     * @brief Constructs a ScreenshotManager instance.
     *
     * Initializes the manager with dependencies for configuration, keybindings, sound,
     * and the executable directory for file paths.
     *
     * @param exeDir The executable directory for resolving file paths.
     * @param configManager The configuration service for settings.
     * @param keybindProvider The keybind provider for input handling.
     * @param soundManager The sound manager for audio feedback.
     */
    ScreenshotManager(const std::string& exeDir, IConfigService* configManager,
                      IKeybindProvider* keybindProvider, ISoundManager* soundManager);

    /**
     * @brief Launches screenshot mode for a VPX file.
     *
     * Starts a process for the specified VPX file, displays the screenshot UI, and
     * prepares for capturing screenshots.
     *
     * @param vpxFile The path to the VPX file to launch.
     */
    void launchScreenshotMode(const std::string& vpxFile) override;

    /**
     * @brief Checks if screenshot mode is active.
     *
     * @return True if screenshot mode is running, false otherwise.
     */
    bool isActive() const override { return isRunning_; }

private:
    std::string exeDir_;                ///< Executable directory for resolving file paths.
    IConfigService* configManager_;     ///< Configuration service for settings.
    IKeybindProvider* keybindProvider_; ///< Keybind provider for input handling.
    ISoundManager* soundManager_;       ///< Sound manager for audio feedback.
    ScreenshotWindow window_;           ///< Window for the screenshot UI.
    ScreenshotCapture capture_;         ///< Handler for capturing screenshots.
    ProcessHandler process_;            ///< Handler for launching and managing the VPX process.
    bool isRunning_;                    ///< Flag indicating if screenshot mode is active.
};

#endif // SCREENSHOT_MANAGER_H