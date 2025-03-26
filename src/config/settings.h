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

    int secondWindowMonitor;
    int secondWindowWidth;
    int secondWindowHeight;
    int backglassMediaWidth;
    int backglassMediaHeight;
    int dmdMediaWidth;
    int dmdMediaHeight;

    // Fade and sound settings
    std::string tableChangeSound;
    std::string tableLoadSound;

    // Keybinds
    SDL_Keycode keyPreviousTable;
    SDL_Keycode keyNextTable;
    SDL_Keycode keyFastPrevTable;
    SDL_Keycode keyFastNextTable;
    SDL_Keycode keyJumpNextLetter;
    SDL_Keycode keyJumpPrevLetter;
    SDL_Keycode keyLaunchTable;
    SDL_Keycode keyToggleConfig;
    SDL_Keycode keyQuit;
    SDL_Keycode keyConfigSave;
    SDL_Keycode keyConfigClose;
    SDL_Keycode keyScreenshotMode;
    SDL_Keycode keyScreenshotKey;
    SDL_Keycode keyScreenshotQuit;
};

#endif // SETTINGS_H