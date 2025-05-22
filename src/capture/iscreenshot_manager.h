#ifndef ISCREENSHOT_MANAGER_H
#define ISCREENSHOT_MANAGER_H

#include <string>

/**
 * @brief Interface for managing screenshot capture operations.
 *
 * The IScreenshotManager interface declares the operations required for launching a screenshot
 * mode based on a provided file, and for checking if the screenshot functionality is currently active.
 *
 * @details The interface requires implementing classes to define logic for:
 *  - Initiating screenshot capture mode with a specified file.
 *  - Reporting whether the screenshot capture mode is active.
 */
class IScreenshotManager {
public:
    virtual ~IScreenshotManager() = default;
    virtual void launchScreenshotMode(const std::string& vpxFile) = 0;
    virtual bool isActive() const = 0;
};

#endif // ISCREENSHOT_MANAGER_H