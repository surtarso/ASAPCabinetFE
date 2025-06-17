#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <SDL2/SDL.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <filesystem>

struct Settings {
    // Enum to define reload requirements for settings changes
    enum class ReloadType {
        None,           // No reload needed
        Title,          // Title position
        Tables,         // Reload table related files
        Assets,         // Full table reloads/renderers
        Windows,        // Window reloads
        Overlay,        // UI widget reloads
        Audio,          // Audio settings reload
        Font            // Font for title display reload
    };

    // Metadata for each setting: field name -> reload type
    // static const std::unordered_map<std::string, ReloadType> settingsMetadata;
    static const std::map<std::string, std::pair<ReloadType, std::string>> settingsMetadata;
    
    // [VPX]
    std::string VPXTablesPath = "$HOME/Games/VPX_Tables/"; // imguifiledialog
    std::string VPinballXPath = "$HOME/Games/vpinball/build/VPinballX_BGFX"; // imguifiledialog
    std::string vpxIniPath = ""; // imguifiledialog
    std::string vpxStartArgs = "";
    std::string vpxEndArgs = "";

    // [DPISettings]
    float dpiScale = 1.0f; // 0.1-1.0
    bool enableDpiScaling = true;

    // [DefaultMedia]
    std::string defaultPlayfieldImage = "img/default_table.png";
    std::string defaultBackglassImage = "img/default_backglass.png";
    std::string defaultDmdImage = "img/default_dmd.png";
    std::string defaultWheelImage = "img/default_wheel.png";
    std::string defaultTopperImage = "img/default_topper.png";
    std::string defaultPlayfieldVideo = "img/default_table.mp4";
    std::string defaultBackglassVideo = "img/default_backglass.mp4";
    std::string defaultDmdVideo = "img/default_dmd.mp4";
    std::string defaultTopperVideo = "img/default_topper.mp4";

    // [CustomMedia]
    std::string customPlayfieldImage = "images/table.png";
    std::string customBackglassImage = "images/backglass.png";
    std::string customDmdImage = "images/dmd.png";
    std::string customWheelImage = "images/wheel.png";
    std::string customTopperImage = "images/topper.png";
    std::string customPlayfieldVideo = "video/table.mp4";
    std::string customBackglassVideo = "video/backglass.mp4";
    std::string customDmdVideo = "video/dmd.mp4";
    std::string customTopperVideo = "images/topper.mp4";
    std::string tableMusic = "audio/music.mp3";
    std::string customLaunchSound = "audio/launch.mp3";

    // [WindowSettings]
    std::string videoBackend = "vlc"; // + 'ffmpeg', 'gstreamer', 'novideo'
    bool useVPinballXIni = true; // use vpx_ini_reader
    int playfieldWindowWidth = 1080;
    int playfieldWindowHeight = 1920;
    int playfieldX = -1;
    int playfieldY = -1;

    bool showBackglass = true;
    int backglassWindowWidth = 1024;
    int backglassWindowHeight = 768;
    int backglassX = -1;
    int backglassY = -1;

    bool showDMD = true;
    int dmdWindowWidth = 1024;
    int dmdWindowHeight = 256;
    int dmdX = -1;
    int dmdY = -1;

    bool showTopper = false;
    int topperWindowWidth = 512;
    int topperWindowHeight = 128;
    int topperWindowX = -1;
    int topperWindowY = -1;

    // [TableMetadata]
    std::string titleSource = "filename"; // + 'metadata'
    bool fetchVPSdb = false;
    bool forceRebuildMetadata = false;
    std::string titleSortBy = "title"; // + 'year', 'author', 'manufacturer', 'type'
    bool showMetadata = false;

    float metadataPanelWidth = 0.7f; // 0.1-1.0
    float metadataPanelHeight = 0.5f; // 0.1-1.0
    float metadataPanelAlpha = 0.6f; // 0.1-1.0

    float titleWeight = 0.6f; //0.2 - 0.8
    float yearWeight = 0.2f; //0 - 0.4
    float manufacturerWeight = 0.1f; //0 - 0.3
    float romWeight = 0.25f; // 0 - 0.5
    float titleThreshold = 0.55f; //0.3 - 0.8
    float confidenceThreshold = 0.6f; //0.4 0.9

    // [UIWidgets]
    bool showArrowHint = true;
    float arrowHintWidth = 20.0f;
    float arrowHintHeight = 100.0f;
    float arrowThickness = 4.0f;
    float arrowAlpha = 0.6f; // 0.1-1.0
    float arrowGlow = 1.5f;
    SDL_Color arrowGlowColor = {200, 200, 200, 255};
    SDL_Color arrowColorTop = {100, 100, 100, 255};
    SDL_Color arrowColorBottom = {150, 150, 150, 255};

    bool showScrollbar = true;
    float scrollbarWidth = 12.0f;
    float thumbWidth = 15.0f;
    float scrollbarLength = 0.5f; // 0.1-1.0
    SDL_Color scrollbarColor = {50, 50, 50, 200};
    SDL_Color scrollbarThumbColor = {50, 150, 150, 255};

    // [TitleDisplay]
    bool showWheel = true;
    std::string wheelWindow = "playfield"; // + 'backglass', 'dmd', 'topper'
    bool showTitle = true;
    std::string titleWindow = "playfield"; // + 'backglass', 'dmd', 'topper'
    std::string fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    SDL_Color fontColor = {255, 255, 255, 255};
    SDL_Color fontBgColor = {0, 0, 0, 128};
    int fontSize = 28; // 1-80
    int titleX = 30;
    int titleY = 1850;

    // [MediaDimensions]
    bool forceImagesOnly = false;

    int wheelMediaHeight = 350;
    int wheelMediaWidth = 350;
    int wheelMediaX = 720;
    int wheelMediaY = 1550;

    int playfieldMediaWidth = 1080;
    int playfieldMediaHeight = 1920;
    int playfieldMediaX = 0;
    int playfieldMediaY = 0;
    int playfieldRotation = 0; // 0-90-180-270-360 +/-

    int backglassMediaWidth = 1024;
    int backglassMediaHeight = 768;
    int backglassMediaX = 0;
    int backglassMediaY = 0;
    int backglassRotation = 0; // 0-90-180-270-360 +/-

    int dmdMediaWidth = 1024;
    int dmdMediaHeight = 256;
    int dmdMediaX = 0;
    int dmdMediaY = 0;
    int dmdRotation = 0; // 0-90-180-270-360 +/-

    int topperMediaWidth = 512;
    int topperMediaHeight = 128;
    int topperMediaX = 0;
    int topperMediaY = 0;
    int topperRotation = 0; // 0-90-180-270-360 +/-

    // [AudioSettings]
    bool masterMute = false;
    float masterVol = 100.0f; // 0-100
    bool mediaAudioMute = false;
    float mediaAudioVol = 60.0f; // 0-100
    bool tableMusicMute = false;
    float tableMusicVol = 60.0f; // 0-100
    bool interfaceAudioMute = false;
    float interfaceAudioVol = 60.0f; // 0-100
    bool interfaceAmbienceMute = false;
    float interfaceAmbienceVol = 60.0f; // 0-100

    // [UiSounds]
    std::string scrollNormalSound = "snd/scroll_normal.mp3";
    std::string scrollFastSound = "snd/scroll_fast.mp3";
    std::string scrollJumpSound = "snd/scroll_jump.mp3";
    std::string scrollRandomSound = "snd/scroll_random.mp3";
    std::string launchTableSound = "snd/launch_table.mp3";
    std::string launchScreenshotSound = "snd/launch_screenshot.mp3";
    std::string panelToggleSound = "snd/panel_toggle.mp3";
    std::string screenshotTakeSound = "snd/screenshot_take.mp3";
    std::string ambienceSound = "snd/interface_ambience.mp3";

    // [Internal]
    std::string vpxSubCmd = "-Play";
    std::string vpsDbPath = "data/vpsdb.json";
    std::string vpsDbUpdateFrequency = "startup";
    std::string vpsDbLastUpdated = "data/lastUpdated.json";
    std::string vpxtoolBin = "";
    std::string vpxtoolIndex = "vpxtool_index.json";
    std::string indexPath = "data/asapcab_index.json";
    int screenshotWait = 4; // 0-60
    // defaults in ConfigUI::drawGUI()
    float configUIWidth = 0.7f;
    float configUIHeight = 0.5f;

    // [Keybinds]
    std::map<std::string, std::string> keybinds_ = {
        {"Previous Table", "Left Shift"},
        {"Next Table", "Right Shift"},
        {"Fast Previous Table", "Left Ctrl"},
        {"Fast Next Table", "Right Ctrl"},
        {"Jump Next Letter", "/"},
        {"Jump Previous Letter", "Z"},
        {"Random Table", "R"},
        {"Launch Table", "Return"},
        {"Toggle Config", "C"},
        {"Quit", "Q"},
        {"Screenshot Mode", "S"},
        {"Screenshot Key", "S"},
        {"Screenshot Quit", "Q"},
        {"Toggle Editor", "M"},
        {"Toggle Catalog", "N"}
    };

    // Apply post-processing (e.g., DPI scaling, path resolution)
    void applyPostProcessing(const std::string& exeDir) {

        // ✅ Resolve VPX paths
        VPXTablesPath = resolvePath(VPXTablesPath, exeDir);
        VPinballXPath = resolvePath(VPinballXPath, exeDir);
        // vpxIniPath    = resolvePath(vpxIniPath, exeDir);

        // Resolve paths for fields marked with needsPathResolution in config_schema.h
        std::vector<std::string> pathFields = {
            "defaultPlayfieldImage", "defaultBackglassImage", "defaultDmdImage",
            "defaultWheelImage", "defaultTopperImage", "defaultPlayfieldVideo",
            "defaultBackglassVideo", "defaultDmdVideo", "defaultTopperVideo"
        };
        for (const auto& field : pathFields) {
            if (field == "defaultPlayfieldImage") defaultPlayfieldImage = resolvePath(defaultPlayfieldImage, exeDir);
            else if (field == "defaultBackglassImage") defaultBackglassImage = resolvePath(defaultBackglassImage, exeDir);
            else if (field == "defaultDmdImage") defaultDmdImage = resolvePath(defaultDmdImage, exeDir);
            else if (field == "defaultWheelImage") defaultWheelImage = resolvePath(defaultWheelImage, exeDir);
            else if (field == "defaultTopperImage") defaultTopperImage = resolvePath(defaultTopperImage, exeDir);
            else if (field == "defaultPlayfieldVideo") defaultPlayfieldVideo = resolvePath(defaultPlayfieldVideo, exeDir);
            else if (field == "defaultBackglassVideo") defaultBackglassVideo = resolvePath(defaultBackglassVideo, exeDir);
            else if (field == "defaultDmdVideo") defaultDmdVideo = resolvePath(defaultDmdVideo, exeDir);
            else if (field == "defaultTopperVideo") defaultTopperVideo = resolvePath(defaultTopperVideo, exeDir);
        }

        // Apply DPI scaling to fontSize if enabled
        if (enableDpiScaling) {
            fontSize = static_cast<int>(fontSize * dpiScale);
        }
    }

private:
    // Resolve relative paths and environment variables (e.g., $USER)
    std::string resolvePath(const std::string& value, const std::string& exeDir) const {
        std::string result = value;
        // Replace $HOME with actual home directory
        const char* home = std::getenv("HOME");
        if (home) {
            size_t pos;
            while ((pos = result.find("$HOME")) != std::string::npos) {
                result.replace(pos, 5, home);
            }
            if (!result.empty() && result[0] == '~') {
                result.replace(0, 1, home);
            }
        }
        // Resolve relative paths
        if (result.empty()) return exeDir + value;
        if (result[0] == '/' || result[0] == '\\') return result;
        return exeDir + result;
    }

    // JSON serialization
    friend void to_json(nlohmann::json& j, const Settings& s) {
        j = nlohmann::json{
            {"VPX", {
                {"VPXTablesPath", s.VPXTablesPath},
                {"VPinballXPath", s.VPinballXPath},
                {"vpxIniPath", s.vpxIniPath},
                {"vpxStartArgs", s.vpxStartArgs},
                {"vpxEndArgs", s.vpxEndArgs}
            }},
            {"DPISettings", {
                {"dpiScale", s.dpiScale},
                {"enableDpiScaling", s.enableDpiScaling}
            }},
            {"DefaultMedia", {
                {"defaultPlayfieldImage", s.defaultPlayfieldImage},
                {"defaultBackglassImage", s.defaultBackglassImage},
                {"defaultDmdImage", s.defaultDmdImage},
                {"defaultWheelImage", s.defaultWheelImage},
                {"defaultTopperImage", s.defaultTopperImage},
                {"defaultPlayfieldVideo", s.defaultPlayfieldVideo},
                {"defaultBackglassVideo", s.defaultBackglassVideo},
                {"defaultDmdVideo", s.defaultDmdVideo},
                {"defaultTopperVideo", s.defaultTopperVideo}
            }},
            {"CustomMedia", {
                {"customPlayfieldImage", s.customPlayfieldImage},
                {"customBackglassImage", s.customBackglassImage},
                {"customDmdImage", s.customDmdImage},
                {"customWheelImage", s.customWheelImage},
                {"customTopperImage", s.customTopperImage},
                {"customPlayfieldVideo", s.customPlayfieldVideo},
                {"customBackglassVideo", s.customBackglassVideo},
                {"customDmdVideo", s.customDmdVideo},
                {"customTopperVideo", s.customTopperVideo},
                {"tableMusic", s.tableMusic},
                {"customLaunchSound", s.customLaunchSound}
            }},
            {"WindowSettings", {
                {"videoBackend", s.videoBackend},
                {"useVPinballXIni", s.useVPinballXIni},
                {"playfieldWindowWidth", s.playfieldWindowWidth},
                {"playfieldWindowHeight", s.playfieldWindowHeight},
                {"playfieldX", s.playfieldX},
                {"playfieldY", s.playfieldY},
                {"showBackglass", s.showBackglass},
                {"backglassWindowWidth", s.backglassWindowWidth},
                {"backglassWindowHeight", s.backglassWindowHeight},
                {"backglassX", s.backglassX},
                {"backglassY", s.backglassY},
                {"showDMD", s.showDMD},
                {"dmdWindowWidth", s.dmdWindowWidth},
                {"dmdWindowHeight", s.dmdWindowHeight},
                {"dmdX", s.dmdX},
                {"dmdY", s.dmdY},
                {"showTopper", s.showTopper},
                {"topperWindowWidth", s.topperWindowWidth},
                {"topperWindowHeight", s.topperWindowHeight},
                {"topperWindowX", s.topperWindowX},
                {"topperWindowY", s.topperWindowY}
            }},
            {"TableMetadata", {
                {"titleSource", s.titleSource},
                {"fetchVPSdb", s.fetchVPSdb},
                {"forceRebuildMetadata", s.forceRebuildMetadata},
                {"titleSortBy", s.titleSortBy},
                {"showMetadata", s.showMetadata},
                {"metadataPanelWidth", s.metadataPanelWidth},
                {"metadataPanelHeight", s.metadataPanelHeight},
                {"metadataPanelAlpha", s.metadataPanelAlpha},
                {"titleWeight", s.titleWeight},
                {"yearWeight", s.yearWeight},
                {"manufacturerWeight", s.manufacturerWeight},
                {"romWeight", s.romWeight},
                {"titleThreshold", s.titleThreshold},
                {"confidenceThreshold", s.confidenceThreshold}
            }},
            {"UIWidgets", {
                {"showArrowHint", s.showArrowHint},
                {"arrowHintWidth", s.arrowHintWidth},
                {"arrowHintHeight", s.arrowHintHeight},
                {"arrowThickness", s.arrowThickness},
                {"arrowAlpha", s.arrowAlpha},
                {"arrowGlow", s.arrowGlow},
                {"arrowGlowColor", {s.arrowGlowColor.r, s.arrowGlowColor.g, s.arrowGlowColor.b, s.arrowGlowColor.a}},
                {"arrowColorTop", {s.arrowColorTop.r, s.arrowColorTop.g, s.arrowColorTop.b, s.arrowColorTop.a}},
                {"arrowColorBottom", {s.arrowColorBottom.r, s.arrowColorBottom.g, s.arrowColorBottom.b, s.arrowColorBottom.a}},
                {"showScrollbar", s.showScrollbar},
                {"scrollbarWidth", s.scrollbarWidth},
                {"thumbWidth", s.thumbWidth},
                {"scrollbarLength", s.scrollbarLength},
                {"scrollbarColor", {s.scrollbarColor.r, s.scrollbarColor.g, s.scrollbarColor.b, s.scrollbarColor.a}},
                {"scrollbarThumbColor", {s.scrollbarThumbColor.r, s.scrollbarThumbColor.g, s.scrollbarThumbColor.b, s.scrollbarThumbColor.a}}
            }},
            {"TitleDisplay", {
                {"showWheel", s.showWheel},
                {"wheelWindow", s.wheelWindow},
                {"showTitle", s.showTitle},
                {"titleWindow", s.titleWindow},
                {"fontPath", s.fontPath},
                {"fontColor", {s.fontColor.r, s.fontColor.g, s.fontColor.b, s.fontColor.a}},
                {"fontBgColor", {s.fontBgColor.r, s.fontBgColor.g, s.fontBgColor.b, s.fontBgColor.a}},
                {"fontSize", s.fontSize},
                {"titleX", s.titleX},
                {"titleY", s.titleY}
            }},
            {"MediaDimensions", {
                {"forceImagesOnly", s.forceImagesOnly},
                {"wheelMediaHeight", s.wheelMediaHeight},
                {"wheelMediaWidth", s.wheelMediaWidth},
                {"wheelMediaX", s.wheelMediaX},
                {"wheelMediaY", s.wheelMediaY},
                {"playfieldMediaWidth", s.playfieldMediaWidth},
                {"playfieldMediaHeight", s.playfieldMediaHeight},
                {"playfieldMediaX", s.playfieldMediaX},
                {"playfieldMediaY", s.playfieldMediaY},
                {"playfieldRotation", s.playfieldRotation},
                {"backglassMediaWidth", s.backglassMediaWidth},
                {"backglassMediaHeight", s.backglassMediaHeight},
                {"backglassMediaX", s.backglassMediaX},
                {"backglassMediaY", s.backglassMediaY},
                {"backglassRotation", s.backglassRotation},
                {"dmdMediaWidth", s.dmdMediaWidth},
                {"dmdMediaHeight", s.dmdMediaHeight},
                {"dmdMediaX", s.dmdMediaX},
                {"dmdMediaY", s.dmdMediaY},
                {"dmdRotation", s.dmdRotation},
                {"topperMediaWidth", s.topperMediaWidth},
                {"topperMediaHeight", s.topperMediaHeight},
                {"topperMediaX", s.topperMediaX},
                {"topperMediaY", s.topperMediaY},
                {"topperRotation", s.topperRotation}
            }},
            {"AudioSettings", {
                {"masterMute", s.masterMute},
                {"masterVol", s.masterVol},
                {"mediaAudioMute", s.mediaAudioMute},
                {"mediaAudioVol", s.mediaAudioVol},
                {"tableMusicMute", s.tableMusicMute},
                {"tableMusicVol", s.tableMusicVol},
                {"interfaceAudioMute", s.interfaceAudioMute},
                {"interfaceAudioVol", s.interfaceAudioVol},
                {"interfaceAmbienceMute", s.interfaceAmbienceMute},
                {"interfaceAmbienceVol", s.interfaceAmbienceVol}
            }},
            {"UISounds", {
                {"scrollNormalSound", s.scrollNormalSound},
                {"scrollFastSound", s.scrollFastSound},
                {"scrollJumpSound", s.scrollJumpSound},
                {"scrollRandomSound", s.scrollRandomSound},
                {"launchTableSound", s.launchTableSound},
                {"launchScreenshotSound", s.launchScreenshotSound},
                {"panelToggleSound", s.panelToggleSound},
                {"screenshotTakeSound", s.screenshotTakeSound},
                {"ambienceSound", s.ambienceSound}
            }},
            {"Internal", {
                {"vpxSubCmd", s.vpxSubCmd},
                {"vpsDbPath", s.vpsDbPath},
                {"vpsDbUpdateFrequency", s.vpsDbUpdateFrequency},
                {"vpsDbLastUpdated", s.vpsDbLastUpdated},
                {"vpxtoolBin", s.vpxtoolBin},
                {"vpxtoolIndex", s.vpxtoolIndex},
                {"indexPath", s.indexPath},
                {"screenshotWait", s.screenshotWait},
                {"configUIWidth", s.configUIWidth},
                {"configUIHeight", s.configUIHeight}
            }},
            {"Keybinds", s.keybinds_}
        };
    }

    friend void from_json(const nlohmann::json& j, Settings& s) {
        // VPX
        s.VPXTablesPath = j.value("VPX", nlohmann::json{}).value("VPXTablesPath", s.VPXTablesPath);
        s.VPinballXPath = j.value("VPX", nlohmann::json{}).value("VPinballXPath", s.VPinballXPath);
        s.vpxIniPath = j.value("VPX", nlohmann::json{}).value("vpxIniPath", s.vpxIniPath);
        s.vpxStartArgs = j.value("VPX", nlohmann::json{}).value("vpxStartArgs", s.vpxStartArgs);
        s.vpxEndArgs = j.value("VPX", nlohmann::json{}).value("vpxEndArgs", s.vpxEndArgs);

        // DPISettings
        s.dpiScale = j.value("DPISettings", nlohmann::json{}).value("dpiScale", s.dpiScale);
        s.enableDpiScaling = j.value("DPISettings", nlohmann::json{}).value("enableDpiScaling", s.enableDpiScaling);

        // DefaultMedia
        s.defaultPlayfieldImage = j.value("DefaultMedia", nlohmann::json{}).value("defaultPlayfieldImage", s.defaultPlayfieldImage);
        s.defaultBackglassImage = j.value("DefaultMedia", nlohmann::json{}).value("defaultBackglassImage", s.defaultBackglassImage);
        s.defaultDmdImage = j.value("DefaultMedia", nlohmann::json{}).value("defaultDmdImage", s.defaultDmdImage);
        s.defaultWheelImage = j.value("DefaultMedia", nlohmann::json{}).value("defaultWheelImage", s.defaultWheelImage);
        s.defaultTopperImage = j.value("DefaultMedia", nlohmann::json{}).value("defaultTopperImage", s.defaultTopperImage);
        s.defaultPlayfieldVideo = j.value("DefaultMedia", nlohmann::json{}).value("defaultPlayfieldVideo", s.defaultPlayfieldVideo);
        s.defaultBackglassVideo = j.value("DefaultMedia", nlohmann::json{}).value("defaultBackglassVideo", s.defaultBackglassVideo);
        s.defaultDmdVideo = j.value("DefaultMedia", nlohmann::json{}).value("defaultDmdVideo", s.defaultDmdVideo);
        s.defaultTopperVideo = j.value("DefaultMedia", nlohmann::json{}).value("defaultTopperVideo", s.defaultTopperVideo);

        // CustomMedia
        s.customPlayfieldImage = j.value("CustomMedia", nlohmann::json{}).value("customPlayfieldImage", s.customPlayfieldImage);
        s.customBackglassImage = j.value("CustomMedia", nlohmann::json{}).value("customBackglassImage", s.customBackglassImage);
        s.customDmdImage = j.value("CustomMedia", nlohmann::json{}).value("customDmdImage", s.customDmdImage);
        s.customWheelImage = j.value("CustomMedia", nlohmann::json{}).value("customWheelImage", s.customWheelImage);
        s.customTopperImage = j.value("CustomMedia", nlohmann::json{}).value("customTopperImage", s.customTopperImage);
        s.customPlayfieldVideo = j.value("CustomMedia", nlohmann::json{}).value("customPlayfieldVideo", s.customPlayfieldVideo);
        s.customBackglassVideo = j.value("CustomMedia", nlohmann::json{}).value("customBackglassVideo", s.customBackglassVideo);
        s.customDmdVideo = j.value("CustomMedia", nlohmann::json{}).value("customDmdVideo", s.customDmdVideo);
        s.customTopperVideo = j.value("CustomMedia", nlohmann::json{}).value("customTopperVideo", s.customTopperVideo);
        s.tableMusic = j.value("CustomMedia", nlohmann::json{}).value("tableMusic", s.tableMusic);
        s.customLaunchSound = j.value("CustomMedia", nlohmann::json{}).value("customLaunchSound", s.customLaunchSound);

        // WindowSettings
        s.videoBackend = j.value("WindowSettings", nlohmann::json{}).value("videoBackend", s.videoBackend);
        s.useVPinballXIni = j.value("WindowSettings", nlohmann::json{}).value("useVPinballXIni", s.useVPinballXIni);
        s.playfieldWindowWidth = j.value("WindowSettings", nlohmann::json{}).value("playfieldWindowWidth", s.playfieldWindowWidth);
        s.playfieldWindowHeight = j.value("WindowSettings", nlohmann::json{}).value("playfieldWindowHeight", s.playfieldWindowHeight);
        s.playfieldX = j.value("WindowSettings", nlohmann::json{}).value("playfieldX", s.playfieldX);
        s.playfieldY = j.value("WindowSettings", nlohmann::json{}).value("playfieldY", s.playfieldY);
        s.showBackglass = j.value("WindowSettings", nlohmann::json{}).value("showBackglass", s.showBackglass);
        s.backglassWindowWidth = j.value("WindowSettings", nlohmann::json{}).value("backglassWindowWidth", s.backglassWindowWidth);
        s.backglassWindowHeight = j.value("WindowSettings", nlohmann::json{}).value("backglassWindowHeight", s.backglassWindowHeight);
        s.backglassX = j.value("WindowSettings", nlohmann::json{}).value("backglassX", s.backglassX);
        s.backglassY = j.value("WindowSettings", nlohmann::json{}).value("backglassY", s.backglassY);
        s.showDMD = j.value("WindowSettings", nlohmann::json{}).value("showDMD", s.showDMD);
        s.dmdWindowWidth = j.value("WindowSettings", nlohmann::json{}).value("dmdWindowWidth", s.dmdWindowWidth);
        s.dmdWindowHeight = j.value("WindowSettings", nlohmann::json{}).value("dmdWindowHeight", s.dmdWindowHeight);
        s.dmdX = j.value("WindowSettings", nlohmann::json{}).value("dmdX", s.dmdX);
        s.dmdY = j.value("WindowSettings", nlohmann::json{}).value("dmdY", s.dmdY);
        s.showTopper = j.value("WindowSettings", nlohmann::json{}).value("showTopper", s.showTopper);
        s.topperWindowWidth = j.value("WindowSettings", nlohmann::json{}).value("topperWindowWidth", s.topperWindowWidth);
        s.topperWindowHeight = j.value("WindowSettings", nlohmann::json{}).value("topperWindowHeight", s.topperWindowHeight);
        s.topperWindowX = j.value("WindowSettings", nlohmann::json{}).value("topperWindowX", s.topperWindowX);
        s.topperWindowY = j.value("WindowSettings", nlohmann::json{}).value("topperWindowY", s.topperWindowY);

        // TableMetadata
        s.titleSource = j.value("TableMetadata", nlohmann::json{}).value("titleSource", s.titleSource);
        s.fetchVPSdb = j.value("TableMetadata", nlohmann::json{}).value("fetchVPSdb", s.fetchVPSdb);
        s.forceRebuildMetadata = j.value("TableMetadata", nlohmann::json{}).value("forceRebuildMetadata", s.forceRebuildMetadata);
        s.titleSortBy = j.value("TableMetadata", nlohmann::json{}).value("titleSortBy", s.titleSortBy);
        s.showMetadata = j.value("TableMetadata", nlohmann::json{}).value("showMetadata", s.showMetadata);
        s.metadataPanelWidth = j.value("TableMetadata", nlohmann::json{}).value("metadataPanelWidth", s.metadataPanelWidth);
        s.metadataPanelHeight = j.value("TableMetadata", nlohmann::json{}).value("metadataPanelHeight", s.metadataPanelHeight);
        s.metadataPanelAlpha = j.value("TableMetadata", nlohmann::json{}).value("metadataPanelAlpha", s.metadataPanelAlpha);

        s.titleWeight = j.value("TableMetadata", nlohmann::json{}).value("titleWeight", s.titleWeight);
        s.yearWeight = j.value("TableMetadata", nlohmann::json{}).value("yearWeight", s.yearWeight);
        s.manufacturerWeight = j.value("TableMetadata", nlohmann::json{}).value("manufacturerWeight", s.manufacturerWeight);
        s.romWeight = j.value("TableMetadata", nlohmann::json{}).value("romWeight", s.romWeight);
        s.titleThreshold = j.value("TableMetadata", nlohmann::json{}).value("titleThreshold", s.titleThreshold);
        s.confidenceThreshold = j.value("TableMetadata", nlohmann::json{}).value("confidenceThreshold", s.confidenceThreshold);

        // UIWidgets
        s.showArrowHint = j.value("UIWidgets", nlohmann::json{}).value("showArrowHint", s.showArrowHint);
        s.arrowHintWidth = j.value("UIWidgets", nlohmann::json{}).value("arrowHintWidth", s.arrowHintWidth);
        s.arrowHintHeight = j.value("UIWidgets", nlohmann::json{}).value("arrowHintHeight", s.arrowHintHeight);
        s.arrowThickness = j.value("UIWidgets", nlohmann::json{}).value("arrowThickness", s.arrowThickness);
        s.arrowAlpha = j.value("UIWidgets", nlohmann::json{}).value("arrowAlpha", s.arrowAlpha);
        s.arrowGlow = j.value("UIWidgets", nlohmann::json{}).value("arrowGlow", s.arrowGlow);
        if (j.value("UIWidgets", nlohmann::json{}).contains("arrowGlowColor") && j["UIWidgets"]["arrowGlowColor"].is_array() && j["UIWidgets"]["arrowGlowColor"].size() == 4) {
            s.arrowGlowColor = {static_cast<Uint8>(j["UIWidgets"]["arrowGlowColor"][0]),
                                static_cast<Uint8>(j["UIWidgets"]["arrowGlowColor"][1]),
                                static_cast<Uint8>(j["UIWidgets"]["arrowGlowColor"][2]),
                                static_cast<Uint8>(j["UIWidgets"]["arrowGlowColor"][3])};
        }
        if (j.value("UIWidgets", nlohmann::json{}).contains("arrowColorTop") && j["UIWidgets"]["arrowColorTop"].is_array() && j["UIWidgets"]["arrowColorTop"].size() == 4) {
            s.arrowColorTop = {static_cast<Uint8>(j["UIWidgets"]["arrowColorTop"][0]),
                               static_cast<Uint8>(j["UIWidgets"]["arrowColorTop"][1]),
                               static_cast<Uint8>(j["UIWidgets"]["arrowColorTop"][2]),
                               static_cast<Uint8>(j["UIWidgets"]["arrowColorTop"][3])};
        }
        if (j.value("UIWidgets", nlohmann::json{}).contains("arrowColorBottom") && j["UIWidgets"]["arrowColorBottom"].is_array() && j["UIWidgets"]["arrowColorBottom"].size() == 4) {
            s.arrowColorBottom = {static_cast<Uint8>(j["UIWidgets"]["arrowColorBottom"][0]),
                                  static_cast<Uint8>(j["UIWidgets"]["arrowColorBottom"][1]),
                                  static_cast<Uint8>(j["UIWidgets"]["arrowColorBottom"][2]),
                                  static_cast<Uint8>(j["UIWidgets"]["arrowColorBottom"][3])};
        }
        s.showScrollbar = j.value("UIWidgets", nlohmann::json{}).value("showScrollbar", s.showScrollbar);
        s.scrollbarWidth = j.value("UIWidgets", nlohmann::json{}).value("scrollbarWidth", s.scrollbarWidth);
        s.thumbWidth = j.value("UIWidgets", nlohmann::json{}).value("thumbWidth", s.thumbWidth);
        s.scrollbarLength = j.value("UIWidgets", nlohmann::json{}).value("scrollbarLength", s.scrollbarLength);
        if (j.value("UIWidgets", nlohmann::json{}).contains("scrollbarColor") && j["UIWidgets"]["scrollbarColor"].is_array() && j["UIWidgets"]["scrollbarColor"].size() == 4) {
            s.scrollbarColor = {static_cast<Uint8>(j["UIWidgets"]["scrollbarColor"][0]),
                                static_cast<Uint8>(j["UIWidgets"]["scrollbarColor"][1]),
                                static_cast<Uint8>(j["UIWidgets"]["scrollbarColor"][2]),
                                static_cast<Uint8>(j["UIWidgets"]["scrollbarColor"][3])};
        }
        if (j.value("UIWidgets", nlohmann::json{}).contains("scrollbarThumbColor") && j["UIWidgets"]["scrollbarThumbColor"].is_array() && j["UIWidgets"]["scrollbarThumbColor"].size() == 4) {
            s.scrollbarThumbColor = {static_cast<Uint8>(j["UIWidgets"]["scrollbarThumbColor"][0]),
                                     static_cast<Uint8>(j["UIWidgets"]["scrollbarThumbColor"][1]),
                                     static_cast<Uint8>(j["UIWidgets"]["scrollbarThumbColor"][2]),
                                     static_cast<Uint8>(j["UIWidgets"]["scrollbarThumbColor"][3])};
        }

        // TitleDisplay
        s.showWheel = j.value("TitleDisplay", nlohmann::json{}).value("showWheel", s.showWheel);
        s.wheelWindow = j.value("TitleDisplay", nlohmann::json{}).value("wheelWindow", s.wheelWindow);
        s.showTitle = j.value("TitleDisplay", nlohmann::json{}).value("showTitle", s.showTitle);
        s.titleWindow = j.value("TitleDisplay", nlohmann::json{}).value("titleWindow", s.titleWindow);
        s.fontPath = j.value("TitleDisplay", nlohmann::json{}).value("fontPath", s.fontPath);
        if (j.value("TitleDisplay", nlohmann::json{}).contains("fontColor") && j["TitleDisplay"]["fontColor"].is_array() && j["TitleDisplay"]["fontColor"].size() == 4) {
            s.fontColor = {static_cast<Uint8>(j["TitleDisplay"]["fontColor"][0]),
                           static_cast<Uint8>(j["TitleDisplay"]["fontColor"][1]),
                           static_cast<Uint8>(j["TitleDisplay"]["fontColor"][2]),
                           static_cast<Uint8>(j["TitleDisplay"]["fontColor"][3])};
        }
        if (j.value("TitleDisplay", nlohmann::json{}).contains("fontBgColor") && j["TitleDisplay"]["fontBgColor"].is_array() && j["TitleDisplay"]["fontBgColor"].size() == 4) {
            s.fontBgColor = {static_cast<Uint8>(j["TitleDisplay"]["fontBgColor"][0]),
                             static_cast<Uint8>(j["TitleDisplay"]["fontBgColor"][1]),
                             static_cast<Uint8>(j["TitleDisplay"]["fontBgColor"][2]),
                             static_cast<Uint8>(j["TitleDisplay"]["fontBgColor"][3])};
        }
        s.fontSize = j.value("TitleDisplay", nlohmann::json{}).value("fontSize", s.fontSize);
        s.titleX = j.value("TitleDisplay", nlohmann::json{}).value("titleX", s.titleX);
        s.titleY = j.value("TitleDisplay", nlohmann::json{}).value("titleY", s.titleY);

        // MediaDimensions
        s.forceImagesOnly = j.value("MediaDimensions", nlohmann::json{}).value("forceImagesOnly", s.forceImagesOnly);
        s.wheelMediaHeight = j.value("MediaDimensions", nlohmann::json{}).value("wheelMediaHeight", s.wheelMediaHeight);
        s.wheelMediaWidth = j.value("MediaDimensions", nlohmann::json{}).value("wheelMediaWidth", s.wheelMediaWidth);
        s.wheelMediaX = j.value("MediaDimensions", nlohmann::json{}).value("wheelMediaX", s.wheelMediaX);
        s.wheelMediaY = j.value("MediaDimensions", nlohmann::json{}).value("wheelMediaY", s.wheelMediaY);
        s.playfieldMediaWidth = j.value("MediaDimensions", nlohmann::json{}).value("playfieldMediaWidth", s.playfieldMediaWidth);
        s.playfieldMediaHeight = j.value("MediaDimensions", nlohmann::json{}).value("playfieldMediaHeight", s.playfieldMediaHeight);
        s.playfieldMediaX = j.value("MediaDimensions", nlohmann::json{}).value("playfieldMediaX", s.playfieldMediaX);
        s.playfieldMediaY = j.value("MediaDimensions", nlohmann::json{}).value("playfieldMediaY", s.playfieldMediaY);
        s.playfieldRotation = j.value("MediaDimensions", nlohmann::json{}).value("playfieldRotation", s.playfieldRotation);
        s.backglassMediaWidth = j.value("MediaDimensions", nlohmann::json{}).value("backglassMediaWidth", s.backglassMediaWidth);
        s.backglassMediaHeight = j.value("MediaDimensions", nlohmann::json{}).value("backglassMediaHeight", s.backglassMediaHeight);
        s.backglassMediaX = j.value("MediaDimensions", nlohmann::json{}).value("backglassMediaX", s.backglassMediaX);
        s.backglassMediaY = j.value("MediaDimensions", nlohmann::json{}).value("backglassMediaY", s.backglassMediaY);
        s.backglassRotation = j.value("MediaDimensions", nlohmann::json{}).value("backglassRotation", s.backglassRotation);
        s.dmdMediaWidth = j.value("MediaDimensions", nlohmann::json{}).value("dmdMediaWidth", s.dmdMediaWidth);
        s.dmdMediaHeight = j.value("MediaDimensions", nlohmann::json{}).value("dmdMediaHeight", s.dmdMediaHeight);
        s.dmdMediaX = j.value("MediaDimensions", nlohmann::json{}).value("dmdMediaX", s.dmdMediaX);
        s.dmdMediaY = j.value("MediaDimensions", nlohmann::json{}).value("dmdMediaY", s.dmdMediaY);
        s.dmdRotation = j.value("MediaDimensions", nlohmann::json{}).value("dmdRotation", s.dmdRotation);
        s.topperMediaWidth = j.value("MediaDimensions", nlohmann::json{}).value("topperMediaWidth", s.topperMediaWidth);
        s.topperMediaHeight = j.value("MediaDimensions", nlohmann::json{}).value("topperMediaHeight", s.topperMediaHeight);
        s.topperMediaX = j.value("MediaDimensions", nlohmann::json{}).value("topperMediaX", s.topperMediaX);
        s.topperMediaY = j.value("MediaDimensions", nlohmann::json{}).value("topperMediaY", s.topperMediaY);
        s.topperRotation = j.value("MediaDimensions", nlohmann::json{}).value("topperRotation", s.topperRotation);

        // AudioSettings
        s.masterMute = j.value("AudioSettings", nlohmann::json{}).value("masterMute", s.masterMute);
        s.masterVol = j.value("AudioSettings", nlohmann::json{}).value("masterVol", s.masterVol);
        s.mediaAudioMute = j.value("AudioSettings", nlohmann::json{}).value("mediaAudioMute", s.mediaAudioMute);
        s.mediaAudioVol = j.value("AudioSettings", nlohmann::json{}).value("mediaAudioVol", s.mediaAudioVol);
        s.tableMusicMute = j.value("AudioSettings", nlohmann::json{}).value("tableMusicMute", s.tableMusicMute);
        s.tableMusicVol = j.value("AudioSettings", nlohmann::json{}).value("tableMusicVol", s.tableMusicVol);
        s.interfaceAudioMute = j.value("AudioSettings", nlohmann::json{}).value("interfaceAudioMute", s.interfaceAudioMute);
        s.interfaceAudioVol = j.value("AudioSettings", nlohmann::json{}).value("interfaceAudioVol", s.interfaceAudioVol);
        s.interfaceAmbienceMute = j.value("AudioSettings", nlohmann::json{}).value("interfaceAmbienceMute", s.interfaceAmbienceMute);
        s.interfaceAmbienceVol = j.value("AudioSettings", nlohmann::json{}).value("interfaceAmbienceVol", s.interfaceAmbienceVol);

        // UISounds
        s.scrollNormalSound = j.value("UISounds", nlohmann::json{}).value("scrollNormalSound", s.scrollNormalSound);
        s.scrollFastSound = j.value("UISounds", nlohmann::json{}).value("scrollFastSound", s.scrollFastSound);
        s.scrollJumpSound = j.value("UISounds", nlohmann::json{}).value("scrollJumpSound", s.scrollJumpSound);
        s.scrollRandomSound = j.value("UISounds", nlohmann::json{}).value("scrollRandomSound", s.scrollRandomSound);
        s.launchTableSound = j.value("UISounds", nlohmann::json{}).value("launchTableSound", s.launchTableSound);
        s.launchScreenshotSound = j.value("UISounds", nlohmann::json{}).value("launchScreenshotSound", s.launchScreenshotSound);
        s.panelToggleSound = j.value("UISounds", nlohmann::json{}).value("panelToggleSound", s.panelToggleSound);
        s.screenshotTakeSound = j.value("UISounds", nlohmann::json{}).value("screenshotTakeSound", s.screenshotTakeSound);
        s.ambienceSound = j.value("UISounds", nlohmann::json{}).value("ambienceSound", s.ambienceSound);

        // Internal
        s.vpxSubCmd = j.value("Internal", nlohmann::json{}).value("vpxSubCmd", s.vpxSubCmd);
        s.vpsDbPath = j.value("Internal", nlohmann::json{}).value("vpsDbPath", s.vpsDbPath);
        s.vpsDbUpdateFrequency = j.value("Internal", nlohmann::json{}).value("vpsDbUpdateFrequency", s.vpsDbUpdateFrequency);
        s.vpsDbLastUpdated = j.value("Internal", nlohmann::json{}).value("vpsDbLastUpdated", s.vpsDbLastUpdated);
        s.vpxtoolIndex = j.value("Internal", nlohmann::json{}).value("vpxtoolIndex", s.vpxtoolIndex);
        s.vpxtoolBin = j.value("Internal", nlohmann::json{}).value("vpxtoolBin", s.vpxtoolBin);
        s.indexPath = j.value("Internal", nlohmann::json{}).value("indexPath", s.indexPath);
        s.configUIWidth = j.value("Internal", nlohmann::json{}).value("configUIWidth", s.configUIWidth);
        s.configUIHeight = j.value("Internal", nlohmann::json{}).value("configUIHeight", s.configUIHeight);
        s.screenshotWait = j.value("Internal", nlohmann::json{}).value("screenshotWait", s.screenshotWait);
        
        // [Keybinds]
        if (j.contains("Keybinds") && j["Keybinds"].is_object()) {
            s.keybinds_.clear();
            for (auto& [key, value] : j["Keybinds"].items()) {
                if (value.is_string()) {
                    s.keybinds_[key] = value.get<std::string>();
                }
            }
        }
    }
};

// Define settingsMetadata
inline const std::map<std::string, std::pair<Settings::ReloadType, std::string>> Settings::settingsMetadata = {
    // VPX
    {"VPXTablesPath", {Settings::ReloadType::Tables, "Defines the absolute path to the folder containing VPX table files.\n"
                                                    "\n"
                                                    "It must be a full path.\n"
                                                    "(e.g., /home/user/tables/).\n"
                                                    "\n"
                                                    "Final command:\n"
                                                    "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"}},
    {"VPinballXPath", {Settings::ReloadType::None, "Defines the absolute path to the VPinballX executable.\n"
                                                  "\n"
                                                  "Final command:\n"
                                                  "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"}},
    {"vpxIniPath", {Settings::ReloadType::None, "Defines the absolute path to the VPinballX.ini file.\n"
                                               "If left empty it will search the default location\n"
                                               "~/.vpinball/VPinballX.ini"}},
    {"vpxStartArgs", {Settings::ReloadType::None, "Optional command-line arguments to prepend to the executable.\n"
                                                 "\n"
                                                 "Final command:\n"
                                                 "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"}},
    {"vpxEndArgs", {Settings::ReloadType::None, "Optional arguments to append after the table file in the command.\n"
                                               "\n"
                                               "Final command:\n"
                                               "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"}},
    // DPISettings (TODO: Should also reload fonts!)
    {"dpiScale", {Settings::ReloadType::Windows, "Manual DPI scale override.\n"
                                                "Only used when EnableDpiScaling is false.\n"
                                                "1.0 = 100%, 1.5 = 150%, etc."}},
    {"enableDpiScaling", {Settings::ReloadType::Windows, "Enable automatic DPI scaling based on system settings.\n"
                                                        "When enabled, the frontend will scale according to your monitor's DPI.\n"
                                                        "Disable for manual control via DpiScale."}},
    // DefaultMedia
    {"defaultPlayfieldImage", {Settings::ReloadType::Tables, "Relative path to the default table preview image.\n"
                                                            "Used when a table has no custom image."}},
    {"defaultBackglassImage", {Settings::ReloadType::Tables, "Relative path to the default backglass image.\n"
                                                            "Used when a table has no custom backglass."}},
    {"defaultDmdImage", {Settings::ReloadType::Tables, "Relative path to the default DMD image.\n"
                                                      "Used when a table has no custom DMD image."}},
    {"defaultWheelImage", {Settings::ReloadType::Tables, "Relative path to the default wheel image.\n"
                                                        "Used when a table has no custom wheel art."}},
    {"defaultTopperImage", {Settings::ReloadType::Tables, "Relative path to the default Topper image.\n"
                                                         "Used when a table has no custom Topper art."}},
    {"defaultPlayfieldVideo", {Settings::ReloadType::Tables, "Relative path to the default table preview video.\n"
                                                            "Used when a table has no custom video."}},
    {"defaultBackglassVideo", {Settings::ReloadType::Tables, "Relative path to the default backglass video.\n"
                                                            "Used when a table has no custom backglass video."}},
    {"defaultDmdVideo", {Settings::ReloadType::Tables, "Relative path to the default DMD video.\n"
                                                      "Used when a table has no custom DMD video."}},
    {"defaultTopperVideo", {Settings::ReloadType::Tables, "Relative path to the default Topper video.\n"
                                                         "Used when a table has no custom Topper video."}},
    // CustomMedia
    {"customPlayfieldImage", {Settings::ReloadType::Tables, "Relative path to the table's preview image.\n"
                                                           "These are relative to your table folder.\n"
                                                           "/path/to/tables/<table_folder>/"}},
    {"customBackglassImage", {Settings::ReloadType::Tables, "Relative path to the backglass image.\n"
                                                           "These are relative to your table folder.\n"
                                                           "/path/to/tables/<table_folder>/"}},
    {"customDmdImage", {Settings::ReloadType::Tables, "Relative path to the DMD image.\n"
                                                     "These are relative to your table folder.\n"
                                                     "/path/to/tables/<table_folder>/"}},
    {"customWheelImage", {Settings::ReloadType::Tables, "Relative path to the wheel image for the table.\n"
                                                       "These are relative to your table folder.\n"
                                                       "/path/to/tables/<table_folder>/"}},
    {"customTopperImage", {Settings::ReloadType::Tables, "Relative path to the topper image.\n"
                                                        "These are relative to your table folder.\n"
                                                        "/path/to/tables/<table_folder>/"}},
    {"customPlayfieldVideo", {Settings::ReloadType::Tables, "Relative path to the table preview video.\n"
                                                           "These are relative to your table folder.\n"
                                                           "/path/to/tables/<table_folder>/"}},
    {"customBackglassVideo", {Settings::ReloadType::Tables, "Relative path to the backglass video.\n"
                                                           "These are relative to your table folder.\n"
                                                           "/path/to/tables/<table_folder>/"}},
    {"customDmdVideo", {Settings::ReloadType::Tables, "Relative path to the DMD video.\n"
                                                     "These are relative to your table folder.\n"
                                                     "/path/to/tables/<table_folder>/"}},
    {"customTopperVideo", {Settings::ReloadType::Tables, "Relative path to the topper video.\n"
                                                        "These are relative to your table folder.\n"
                                                        "/path/to/tables/<table_folder>/"}},
    {"tableMusic", {Settings::ReloadType::Tables, "Relative path to the table music file.\n"
                                                 "These are relative to your table folder.\n"
                                                 "/path/to/tables/<table_folder>/"}},
    {"customLaunchSound", {Settings::ReloadType::Tables, "Relative path to the table launch audio file.\n"
                                                        "These are relative to your table folder.\n"
                                                        "/path/to/tables/<table_folder>/"}},
    // WindowSettings
    {"videoBackend", {Settings::ReloadType::Assets, "Select video playback backend:\n"
                                                   "VLC: Reliable software-based playback with broad format support. Ideal for compatibility.\n"
                                                   "FFmpeg: Efficient software-based playback with extensive codec support. Suitable for most systems.\n"
                                                   "GStreamer: Flexible playback with plugin-based architecture. Good for customized setups."}},
    {"useVPinballXIni", {Settings::ReloadType::Windows, "Uses sizes and positions from ~/.vpinball/VPinballX.ini\n"
                                                       "Using this option will override options below."}},
    {"playfieldWindowWidth", {Settings::ReloadType::Assets, "Width of the Playfield window in pixels.\n"
                                                           "This should be relative to your Playfield media width."}},
    {"playfieldWindowHeight", {Settings::ReloadType::Assets, "Height of the Playfield window in pixels.\n"
                                                            "This should be relative to your Playfield media height."}},
    {"playfieldX", {Settings::ReloadType::Assets, "X position of the Playfield window.\n"
                                                 "You can drag and double-click a window to save its position"}},
    {"playfieldY", {Settings::ReloadType::Assets, "Y position of the Playfield window.\n"
                                                 "You can drag and double-click a window to save its position"}},
    {"showBackglass", {Settings::ReloadType::Assets, "Show/hide the backglass window."}},
    {"backglassWindowWidth", {Settings::ReloadType::Assets, "Width of the Backglass window in pixels.\n"
                                                           "This should be relative to your Backglass media width."}},
    {"backglassWindowHeight", {Settings::ReloadType::Assets, "Height of the Backglass window in pixels.\n"
                                                            "This should be relative to your Backglass media height."}},
    {"backglassX", {Settings::ReloadType::Assets, "X position of the Backglass window.\n"
                                                 "You can drag and double-click a window to save its position"}},
    {"backglassY", {Settings::ReloadType::Assets, "Y position of the Backglass window.\n"
                                                 "You can drag and double-click a window to save its position"}},
    {"showDMD", {Settings::ReloadType::Assets, "Show/hide the DMD window."}},
    {"dmdWindowWidth", {Settings::ReloadType::Assets, "Width of the DMD window in pixels.\n"
                                                     "This should be relative to your DMD media width."}},
    {"dmdWindowHeight", {Settings::ReloadType::Assets, "Height of the DMD window in pixels.\n"
                                                      "This should be relative to your DMD media height."}},
    {"dmdX", {Settings::ReloadType::Assets, "X position of the DMD window.\n"
                                           "You can drag and double-click a window to save its position"}},
    {"dmdY", {Settings::ReloadType::Assets, "Y position of the DMD window.\n"
                                           "You can drag and double-click a window to save its position"}},
    {"showTopper", {Settings::ReloadType::Assets, "Show/hide the Topper window."}},
    {"topperWindowWidth", {Settings::ReloadType::Assets, "Width of the Topper window in pixels.\n"
                                                        "This should be relative to your Topper media width."}},
    {"topperWindowHeight", {Settings::ReloadType::Assets, "Height of the Topper window in pixels.\n"
                                                         "This should be relative to your Topper media height."}},
    {"topperWindowX", {Settings::ReloadType::Assets, "X position of the Topper window.\n"
                                                    "You can drag and double-click a window to save its position"}},
    {"topperWindowY", {Settings::ReloadType::Assets, "Y position of the Topper window.\n"
                                                    "You can drag and double-click a window to save its position"}},
    // TableMetadata
    {"titleSource", {Settings::ReloadType::Tables, "Select how table info will be displayed.\n"
                                                  "- filename: The only source of info is the filename for the table title, metadata panel will be empty.\n"
                                                  "- metadata: Extract metadata from files to display as they come.\n"
                                                  "TIP: If you already have vpxtool_index.json, it will read it instead of re-scanning your files."}},
    {"fetchVPSdb", {Settings::ReloadType::Tables, "Fetches Virtual Pinball Spreadsheet database and\n"
                                               "attempts to match with file metadata to improve information.\n"
                                               "TIP: Leave this OFF after you're happy with the metadata.\n"}},
    {"forceRebuildMetadata", {Settings::ReloadType::Tables, "Forces re-building metadata from scratch.\n"
                                                           "You need to rebuild if changing metadata options.\n"
                                                           "TIP: Leave this OFF after rebuilding metadata."}},
    {"titleSortBy", {Settings::ReloadType::Tables, "Select the sorting of tables.\n"
                                                  "- Requires VPSdb metadata."}},
    {"showMetadata", {Settings::ReloadType::Overlay, "Show/hide the metadata panel overlay on the playfield window.\n"
                                                    "TitleSource must be set to 'metadata' for the panel to display something."}},
    {"metadataPanelWidth", {Settings::ReloadType::Overlay, "Width of the metadata panel as a fraction of the screen (0.1-1.0)"}},
    {"metadataPanelHeight", {Settings::ReloadType::Overlay, "Height of the metadata panel as a fraction of the screen (0.1-1.0)"}},
    {"metadataPanelAlpha", {Settings::ReloadType::Overlay, "Transparency of the metadata panel (0.1-1.0)"}},
    {"titleWeight", {Settings::ReloadType::None, "Weight applied to title matching in the scoring process.\n"
                                                    "Higher values prioritize title similarity when determining a match."}},
    {"yearWeight", {Settings::ReloadType::None, "Weight applied to year matching in the scoring process.\n"
                                                    "Higher values emphasize year consistency for better accuracy."}},
    {"manufacturerWeight", {Settings::ReloadType::None, "Weight applied to manufacturer matching in the scoring process.\n"
                                                    "Higher values increase the importance of manufacturer data."}},
    {"romWeight", {Settings::ReloadType::None, "Weight applied to ROM matching in the scoring process.\n"
                                                    "Higher values boost the score when ROM names align."}},
    {"titleThreshold", {Settings::ReloadType::None, "Minimum similarity score (based on Levenshtein distance) required for a title to be considered a potential match."}},
    {"confidenceThreshold", {Settings::ReloadType::None, "Minimum total score required to confirm a table as a match in the database.\n"
                                                    "Higher values ensure stricter matching."}},
 
    // UIWidgets
    {"showArrowHint", {Settings::ReloadType::None, "Toggle visibility of the arrow hint widget."}},
    {"arrowHintWidth", {Settings::ReloadType::None, "Width of the arrow hint in pixels."}},
    {"arrowHintHeight", {Settings::ReloadType::None, "Height of the arrow hint in pixels."}},
    {"arrowThickness", {Settings::ReloadType::None, "Thickness of the arrow hint in pixels."}},
    {"arrowAlpha", {Settings::ReloadType::None, "Transparency of the arrow hint (0.1-1.0)"}},
    {"arrowGlow", {Settings::ReloadType::None, "Glow effect size of the arrow hint."}},
    {"arrowGlowColor", {Settings::ReloadType::None, "Color of the arrow hint glow."}},
    {"arrowColorTop", {Settings::ReloadType::None, "Top color of the arrow hint gradient."}},
    {"arrowColorBottom", {Settings::ReloadType::None, "Bottom color of the arrow hint gradient."}},
    {"showScrollbar", {Settings::ReloadType::None, "Toggle visibility of the scrollbar."}},
    {"scrollbarWidth", {Settings::ReloadType::None, "Width of the scrollbar in pixels."}},
    {"thumbWidth", {Settings::ReloadType::None, "Width of the scrollbar thumb in pixels."}},
    {"scrollbarLength", {Settings::ReloadType::None, "Length of the scrollbar as a fraction of the screen (0.1-1.0)"}},
    {"scrollbarColor", {Settings::ReloadType::None, "Color of the scrollbar."}},
    {"scrollbarThumbColor", {Settings::ReloadType::None, "Color of the scrollbar thumb."}},
    // TitleDisplay
    {"showWheel", {Settings::ReloadType::None, "Toggle visibility of the wheel image in the main window.\n"
                                              "Set to true to show the wheel, false to hide it."}},
    {"wheelWindow", {Settings::ReloadType::Tables, "Select the window to display the wheel art."}},
    {"showTitle", {Settings::ReloadType::None, "Toggle visibility of table titles in the main window.\n"
                                              "Set to true to show titles, false to hide them."}},
    {"titleWindow", {Settings::ReloadType::Tables, "Select the window to display the table title."}},
    {"fontPath", {Settings::ReloadType::Font, "Select a font for the table title display."}},
    {"fontColor", {Settings::ReloadType::Font, "Color of the table title display text."}},
    {"fontBgColor", {Settings::ReloadType::Font, "Background color behind the table title."}},
    {"fontSize", {Settings::ReloadType::Font, "Font size in points for table title text rendering."}},
    {"titleX", {Settings::ReloadType::Title, "X position of the table title"}},
    {"titleY", {Settings::ReloadType::Title, "Y position of the table title"}},
    // MediaDimensions
    {"forceImagesOnly", {Settings::ReloadType::Tables, "Use only images (skip videos)."}},
    {"wheelMediaHeight", {Settings::ReloadType::None, "Height of the wheel image in pixels."}},
    {"wheelMediaWidth", {Settings::ReloadType::None, "Width of the wheel image in pixels."}},
    {"wheelMediaX", {Settings::ReloadType::None, "X position of the wheel image."}},
    {"wheelMediaY", {Settings::ReloadType::None, "Y position of the wheel image."}},
    {"playfieldMediaWidth", {Settings::ReloadType::None, "Width of the playfield media in pixels."}},
    {"playfieldMediaHeight", {Settings::ReloadType::None, "Height of the playfield media in pixels."}},
    {"playfieldMediaX", {Settings::ReloadType::None, "X position of the playfield media.\n"
                                                    "This position is relative to the playfield window."}},
    {"playfieldMediaY", {Settings::ReloadType::None, "Y position of the playfield media.\n"
                                                    "This position is relative to the playfield window."}},
    {"playfieldRotation", {Settings::ReloadType::None, "Rotation of the Playfield media.\n"
                                                      "0 = no rotation\n"
                                                      "90, 180, 270, -90, etc"}},
    {"backglassMediaWidth", {Settings::ReloadType::None, "Width of the backglass media in pixels."}},
    {"backglassMediaHeight", {Settings::ReloadType::None, "Height of the backglass media in pixels."}},
    {"backglassMediaX", {Settings::ReloadType::None, "X position of the backglass media.\n"
                                                    "This position is relative to the backglass window."}},
    {"backglassMediaY", {Settings::ReloadType::None, "Y position of the backglass media.\n"
                                                    "This position is relative to the backglass window."}},
    {"backglassRotation", {Settings::ReloadType::None, "Rotation of the Backglass media.\n"
                                                      "0 = no rotation\n"
                                                      "90, 180, 270, -90, etc"}},
    {"dmdMediaWidth", {Settings::ReloadType::None, "Width of the DMD media in pixels."}},
    {"dmdMediaHeight", {Settings::ReloadType::None, "Height of the DMD media in pixels.\n"
                                                   "This should match your DMD window height."}},
    {"dmdMediaX", {Settings::ReloadType::None, "X position of the DMD media.\n"
                                              "This position is relative to the DMD window."}},
    {"dmdMediaY", {Settings::ReloadType::None, "Y position of the DMD media.\n"
                                              "This position is relative to the DMD window."}},
    {"dmdRotation", {Settings::ReloadType::None, "Rotation of the DMD media.\n"
                                                "0 = no rotation\n"
                                                "90, 180, 270, -90, etc"}},
    {"topperMediaWidth", {Settings::ReloadType::None, "Width of the Topper media in pixels."}},
    {"topperMediaHeight", {Settings::ReloadType::None, "Height of the Topper media in pixels.\n"
                                                      "This should match your Topper window height."}},
    {"topperMediaX", {Settings::ReloadType::None, "X position of the Topper media.\n"
                                                 "This position is relative to the Topper window."}},
    {"topperMediaY", {Settings::ReloadType::None, "Y position of the Topper media.\n"
                                                 "This position is relative to the Topper window."}},
    {"topperRotation", {Settings::ReloadType::None, "Rotation of the Topper media.\n"
                                                   "0 = no rotation\n"
                                                   "90, 180, 270, -90, etc"}},
    // AudioSettings
    {"masterMute", {Settings::ReloadType::Audio, "Mute all audio"}},
    {"masterVol", {Settings::ReloadType::Audio, "Adjust all volume."}},
    {"mediaAudioMute", {Settings::ReloadType::Audio, "Mute playfield, backglass and DMD audio"}},
    {"mediaAudioVol", {Settings::ReloadType::Audio, "Adjust playfield, backglass and DMD video volume."}},
    {"tableMusicMute", {Settings::ReloadType::Audio, "Mute current table music."}},
    {"tableMusicVol", {Settings::ReloadType::Audio, "Adjust current table music volume."}},
    {"interfaceAudioMute", {Settings::ReloadType::Audio, "Mute interface sounds."}},
    {"interfaceAudioVol", {Settings::ReloadType::Audio, "Adjust interface sounds volume."}},
    {"interfaceAmbienceMute", {Settings::ReloadType::Audio, "Mute interface ambience."}},
    {"interfaceAmbienceVol", {Settings::ReloadType::Audio, "Adjust interface ambience volume."}},
    // UISounds
    {"scrollNormalSound", {Settings::ReloadType::Tables, "Sound played when scrolling single tables."}},
    {"scrollFastSound", {Settings::ReloadType::Tables, "Sound played when fast scrolling in 10's."}},
    {"scrollJumpSound", {Settings::ReloadType::Tables, "Sound played when jumping by letter."}},
    {"scrollRandomSound", {Settings::ReloadType::Tables, "Sound played when selecting a random table."}},
    {"launchTableSound", {Settings::ReloadType::Tables, "Sound played when launching a table."}},
    {"launchScreenshotSound", {Settings::ReloadType::Tables, "Sound played when entering screenshot mode."}},
    {"panelToggleSound", {Settings::ReloadType::Tables, "Sound played when opening or closing panels."}},
    {"screenshotTakeSound", {Settings::ReloadType::Tables, "Sound played when taking a screenshot."}},
    {"ambienceSound", {Settings::ReloadType::Tables, "Sound played on the background if there is no table music."}},
    // Internal
    {"vpxSubCmd", {Settings::ReloadType::None, "VPinballX internal command to play .vpx tables.\n"
                                              "Use VPinballX --help command line menu to see more."}},
    {"vpsDbPath", {Settings::ReloadType::None, "Path to the VPS database file, relative to exec dir."}},
    {"vpsDbUpdateFrequency", {Settings::ReloadType::None, "Choose when to fetch for updates in VPS database.\n"
                                                         "The only option for now is 'startup'."}},
    {"vpsDbLastUpdated", {Settings::ReloadType::None, "Path to the VPS database update file, relative to exec dir."}},
    {"vpxtoolIndex", {Settings::ReloadType::None, "Path to the vpxtool index file, relative to tables folder by default."}},
    {"vpxtoolBin", {Settings::ReloadType::None, "Path to the vpxtool binary file if not in $PATH."}},
    {"indexPath", {Settings::ReloadType::None, "Path to the main table index file, relative to exec dir."}},
    {"screenshotWait", {Settings::ReloadType::None, "Time for the screenshot tool to wait until there are visible windows in VPX."}},
    {"configUIWidth", {Settings::ReloadType::None, "Config window width."}},
    {"configUIHeight", {Settings::ReloadType::None, "Config window height."}},
    // Keybinds
    {"Previous Table", {Settings::ReloadType::None, "Key to select the previous table in the list."}},
    {"Next Table", {Settings::ReloadType::None, "Key to select the next table in the list."}},
    {"Fast Previous Table", {Settings::ReloadType::None, "Key to quickly jump back 10 tables."}},
    {"Fast Next Table", {Settings::ReloadType::None, "Key to quickly jump forward 10 tables."}},
    {"Jump Next Letter", {Settings::ReloadType::None, "Key to jump to the next table starting with a different letter."}},
    {"Jump Previous Letter", {Settings::ReloadType::None, "Key to jump to the previous table starting with a different letter."}},
    {"Random Table", {Settings::ReloadType::None, "Key to jump to a random table."}},
    {"Launch Table", {Settings::ReloadType::None, "Key to launch the selected table."}},
    {"Toggle Config", {Settings::ReloadType::None, "Key to open or close the configuration menu."}},
    {"Quit", {Settings::ReloadType::None, "Key to exit menus and application."}},
    {"Screenshot Mode", {Settings::ReloadType::None, "Key to launch a table in screenshot mode."}},
    {"Screenshot Key", {Settings::ReloadType::None, "Key to take a screenshot while in screenshot mode."}},
    {"Screenshot Quit", {Settings::ReloadType::None, "Key to quit screenshot mode."}},
    {"Toggle Editor", {Settings::ReloadType::None, "Key to open metadata editor."}},
    {"Toggle Catalog", {Settings::ReloadType::None, "Key to open metadata catalog."}}
};

#endif // SETTINGS_H