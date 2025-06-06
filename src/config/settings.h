#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <SDL2/SDL.h>

struct Settings {
    // [VPX]
    std::string VPXTablesPath;
    std::string VPinballXPath;
    std::string vpxIniPath;
    std::string vpxStartArgs;
    std::string vpxEndArgs;

    // [DPISettings]
    float dpiScale = 1.0f;
    bool enableDpiScaling = true;

    // [DefaultMedia]
    std::string defaultPlayfieldImage;
    std::string defaultBackglassImage;
    std::string defaultDmdImage;
    std::string defaultWheelImage;
    std::string defaultTopperImage;
    std::string defaultPlayfieldVideo;
    std::string defaultBackglassVideo;
    std::string defaultDmdVideo;
    std::string defaultTopperVideo;

    // [CustomMedia]
    std::string customPlayfieldImage;
    std::string customBackglassImage;
    std::string customDmdImage;
    std::string customWheelImage;
    std::string customTopperImage;
    std::string customPlayfieldVideo;
    std::string customBackglassVideo;
    std::string customDmdVideo;
    std::string customTopperVideo;
    std::string tableMusic;
    std::string customLaunchSound;

    // [WindowSettings]
    std::string videoBackend;
    bool useVPinballXIni;
    int playfieldWindowWidth;
    int playfieldWindowHeight;
    int playfieldX;
    int playfieldY;

    bool showBackglass;
    int backglassWindowWidth;
    int backglassWindowHeight;
    int backglassX;
    int backglassY;

    bool showDMD;
    int dmdWindowWidth;
    int dmdWindowHeight;
    int dmdX;
    int dmdY;

    bool showTopper;
    int topperWindowWidth;
    int topperWindowHeight;
    int topperWindowX;
    int topperWindowY;

    // [TableMetadata]
    std::string titleSource;
    bool fetchVPSdb;
    bool forceRebuildMetadata;
    std::string titleSortBy;
    bool showMetadata;

    float metadataPanelWidth;
    float metadataPanelHeight;
    float metadataPanelAlpha;

    // [UIWidgets]
    bool showArrowHint;
    float arrowHintWidth;
    float arrowHintHeight;
    float arrowThickness;
    float arrowAlpha;
    float arrowGlow;
    SDL_Color arrowGlowColor;
    SDL_Color arrowColorTop;
    SDL_Color arrowColorBottom;

    bool showScrollbar;
    float scrollbarWidth;
    float thumbWidth;
    float scrollbarLength;
    SDL_Color scrollbarColor;
    SDL_Color scrollbarThumbColor;

    // [TitleDisplay]
    bool showWheel;
    std::string wheelWindow;
    bool showTitle;
    std::string titleWindow;
    std::string fontPath;
    SDL_Color fontColor;
    SDL_Color fontBgColor;
    int fontSize;
    int titleX;
    int titleY;

    // [MediaDimensions]
    bool forceImagesOnly;

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

    int topperMediaWidth;
    int topperMediaHeight;
    int topperMediaX;
    int topperMediaY;
    int topperRotation;

    // [AudioSettings]
    bool masterMute;
    float masterVol;
    bool mediaAudioMute;
    float mediaAudioVol;
    bool tableMusicMute;
    float tableMusicVol;
    bool interfaceAudioMute;
    float interfaceAudioVol;
    bool interfaceAmbienceMute;
    float interfaceAmbienceVol;

    // [UiSounds]
    std::string scrollPrevSound;
    std::string scrollNextSound;
    std::string scrollFastPrevSound;
    std::string scrollFastNextSound;
    std::string scrollJumpPrevSound;
    std::string scrollJumpNextSound;
    std::string scrollRandomSound;
    std::string launchTableSound;
    std::string launchScreenshotSound;
    std::string configToggleSound;
    std::string configSaveSound;
    std::string screenshotTakeSound;
    std::string screenshotQuitSound;
    std::string ambienceSound;

    // [Internal]
    std::string vpxSubCmd;
    std::string logFile;
    std::string vpsDbPath;
    std::string vpsDbUpdateFrequency;
    std::string vpsDbLastUpdated;
    std::string vpxtoolIndex;
    std::string indexPath;
    int screenshotWait;
};

#endif // SETTINGS_H