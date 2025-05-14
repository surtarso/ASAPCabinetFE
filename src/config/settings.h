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
    std::string defaultPlayfieldImage;
    std::string defaultBackglassImage;
    std::string defaultDmdImage;
    std::string defaultWheelImage;
    std::string defaultPlayfieldVideo;
    std::string defaultBackglassVideo;
    std::string defaultDmdVideo;

    // Custom media paths
    std::string customPlayfieldImage;
    std::string customBackglassImage;
    std::string customDmdImage;
    std::string customWheelImage;
    std::string customPlayfieldVideo;
    std::string customBackglassVideo;
    std::string customDmdVideo;

    // Window settings
    int playfieldWindowMonitor;
    int playfieldWindowWidth;
    int playfieldWindowHeight;
    int playfieldX;
    int playfieldY;

    int backglassWindowMonitor;
    int backglassWindowWidth;
    int backglassWindowHeight;
    int backglassX;
    int backglassY;

    int dmdWindowMonitor;
    int dmdWindowWidth;
    int dmdWindowHeight;
    int dmdX;
    int dmdY;

    // Title display
    std::string fontPath;
    SDL_Color fontColor;
    SDL_Color fontBgColor;
    int fontSize;
    bool showWheel;
    bool showTitle;
    int titleX;
    int titleY;

    // media sizes/positions
    int wheelMediaHeight;
    int wheelMediaWidth;
    int wheelMediaX;
    int wheelMediaY;
    
    int playfieldMediaWidth;
    int playfieldMediaHeight;
    int playfieldMediaX;
    int playfieldMediaY;
    int playfieldRotation;

    int backglassMediaWidth;
    int backglassMediaHeight;
    int backglassMediaX;
    int backglassMediaY;
    int backglassRotation;
    
    int dmdMediaWidth;
    int dmdMediaHeight;
    int dmdMediaX;
    int dmdMediaY;
    int dmdRotation;

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

    std::string logFile;
};

#endif // SETTINGS_H
