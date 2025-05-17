#ifndef ISCREENSHOT_MANAGER_H
#define ISCREENSHOT_MANAGER_H

#include <string>

class IScreenshotManager {
public:
    virtual ~IScreenshotManager() = default;
    virtual void launchScreenshotMode(const std::string& vpxFile) = 0;
    virtual bool isActive() const = 0;
};

#endif // ISCREENSHOT_MANAGER_H