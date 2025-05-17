#ifndef SCREENSHOT_MANAGER_H
#define SCREENSHOT_MANAGER_H

#include "capture/iscreenshot_manager.h"
#include "capture/screenshot_window.h"
#include "capture/screenshot_capture.h"
#include "capture/process_handler.h"
#include <string>

class IConfigService;
class IKeybindProvider;
class ISoundManager;

class ScreenshotManager : public IScreenshotManager {
public:
    ScreenshotManager(const std::string& exeDir, IConfigService* configManager,
                      IKeybindProvider* keybindProvider, ISoundManager* soundManager);
    void launchScreenshotMode(const std::string& vpxFile) override;
    bool isActive() const override { return isRunning_; }

private:
    std::string exeDir_;
    IConfigService* configManager_;
    IKeybindProvider* keybindProvider_;
    ISoundManager* soundManager_;
    ScreenshotWindow window_;
    ScreenshotCapture capture_;
    ProcessHandler process_;
    bool isRunning_;
};

#endif // SCREENSHOT_MANAGER_H