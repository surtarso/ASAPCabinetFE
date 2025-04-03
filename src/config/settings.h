#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <SDL2/SDL.h>

struct Settings {
    // VPX settings
    std::string vpxTablesPath;
    std::string vpxExecutableCmd;
    std::string vpxSubCmd;
    std::string vpxStartArgs;
    std::string vpxEndArgs;

    // DPI settings
    float dpiScale = 1.0f;
    bool enableDpiScaling = true;

    // Default media paths
    std::string defaultTableImage;
    std::string defaultBackglassImage;
    std::string defaultDmdImage;
    std::string defaultWheelImage;
    std::string defaultTableVideo;
    std::string defaultBackglassVideo;
    std::string defaultDmdVideo;

    // Custom media paths
    std::string customTableImage;
    std::string customBackglassImage;
    std::string customDmdImage;
    std::string customWheelImage;
    std::string customTableVideo;
    std::string customBackglassVideo;
    std::string customDmdVideo;

    // Window settings
    int mainWindowMonitor;
    int mainWindowWidth;
    int mainWindowHeight;
    int wheelImageSize;
    int wheelImageMargin;

    // Title display
    std::string fontPath;
    SDL_Color fontColor;
    SDL_Color fontBgColor;
    int fontSize;
    bool showWheel;
    bool showTitle;

    int secondWindowMonitor;
    int secondWindowWidth;
    int secondWindowHeight;
    int backglassMediaWidth;
    int backglassMediaHeight;
    int dmdMediaWidth;
    int dmdMediaHeight;

    // sound settings
    std::string configToggleSound;
    std::string scrollPrevSound;
    std::string scrollNextSound;
    std::string scrollFastPrevSound;
    std::string scrollFastNextSound;
    std::string scrollJumpPrevSound;
    std::string scrollJumpNextSound;
    std::string scrollRandomSound;
    std::string launchTableSound;
    std::string launchScreenshotSound;
    std::string configSaveSound;
    std::string configCloseSound;
    std::string quitSound;
    std::string screenshotTakeSound;
    std::string screenshotQuitSound;
};

#endif // SETTINGS_H
