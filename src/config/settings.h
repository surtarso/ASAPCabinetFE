#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <SDL2/SDL.h>

struct Settings {
    // [VPX]
    std::string VPXTablesPath;
    std::string VPinballXPath;
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
    std::string defaultPlayfieldVideo;
    std::string defaultBackglassVideo;
    std::string defaultDmdVideo;
    std::string defaultPuPPlayfieldImage;
    std::string defaultPuPBackglassImage;
    std::string defaultPuPDmdImage;
    std::string defaultPuPFullDmdImage;
    std::string defaultPupTopperImage;
    std::string defaultPuPPlayfieldVideo;
    std::string defaultPuPBackglassVideo;
    std::string defaultPuPDmdVideo;
    std::string defaultPuPFullDmdVideo;
    std::string defaultPuPTopperVideo;

    // [CustomMedia]
    std::string customPlayfieldImage;
    std::string customBackglassImage;
    std::string customDmdImage;
    std::string customWheelImage;
    std::string customPlayfieldVideo;
    std::string customBackglassVideo;
    std::string customDmdVideo;
    std::string puPTopperImage;
    std::string puPPlayfieldImage;
    std::string puPBackglassImage;
    std::string puPDmdImage;
    std::string puPFullDmdImage;
    std::string puPTopperVideo;
    std::string puPPlayfieldVideo;
    std::string puPBackglassVideo;
    std::string puPDmdVideo;
    std::string puPFullDmdVideo;
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

    // [TitleDisplay]
    std::string fontPath;
    SDL_Color fontColor;
    SDL_Color fontBgColor;
    int fontSize;
    std::string titleSource;
    bool showMetadata;
    bool showWheel;
    bool showTitle;
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

    // [AudioSettings]
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
};

#endif // SETTINGS_H