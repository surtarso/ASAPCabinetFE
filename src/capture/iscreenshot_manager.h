/**
 * @file iscreenshot_manager.h
 * @brief Defines the IScreenshotManager interface for screenshot capture operations in ASAPCabinetFE.
 *
 * This header provides the IScreenshotManager interface, which specifies the contract
 * for launching screenshot mode and checking its active state. It is implemented by
 * classes like ScreenshotManager to manage screenshot capture processes.
 */

#ifndef ISCREENSHOT_MANAGER_H
#define ISCREENSHOT_MANAGER_H

#include <string>

/**
 * @class IScreenshotManager
 * @brief Interface for managing screenshot capture operations.
 *
 * The IScreenshotManager interface declares pure virtual methods for initiating
 * screenshot capture mode with a specified VPX file and checking whether the screenshot
 * functionality is active. Implementing classes must provide logic for launching a
 * process, displaying a UI, and capturing screenshots.
 */
class IScreenshotManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes.
     */
    virtual ~IScreenshotManager() = default;

    /**
     * @brief Launches screenshot mode for a VPX file.
     *
     * Initiates screenshot capture mode by launching a process for the specified VPX
     * file and preparing the UI and capture mechanisms.
     *
     * @param vpxFile The path to the VPX file to launch for screenshot capture.
     */
    virtual void launchScreenshotMode(const std::string& vpxFile) = 0;

    /**
     * @brief Checks if screenshot mode is active.
     *
     * @return True if screenshot mode is currently running, false otherwise.
     */
    virtual bool isActive() const = 0;
};

#endif // ISCREENSHOT_MANAGER_H