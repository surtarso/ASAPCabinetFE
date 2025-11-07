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
    bool autoPatchTables = false;

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
    std::string videoBackend = "ffmpeg"; // + 'vlc', 'novideo', 'software'
    // When true, prefer SDL software renderers for all windows (diagnostic/testing)
    bool forceSoftwareRenderer = false;
    bool useVPinballXIni = false; // use vpx_ini_reader
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
    bool useVpxtool = false;
    bool ignoreScanners = false;
    bool forceRebuildMetadata = false;
    std::string titleSortBy = "title"; // + 'year', 'author', 'manufacturer', 'type'

    float titleWeight = 0.6f; //0.2 - 0.8
    float yearWeight = 0.2f; //0 - 0.4
    float manufacturerWeight = 0.1f; //0 - 0.3
    float romWeight = 0.25f; // 0 - 0.5
    float titleThreshold = 0.55f; //0.3 - 0.8
    float confidenceThreshold = 0.6f; //0.4 0.9

    // [UIWidgets]
    bool showMetadata = false;
    float metadataPanelWidth = 0.7f; // 0.1-1.0
    float metadataPanelHeight = 0.5f; // 0.1-1.0
    float metadataPanelAlpha = 0.6f; // 0.1-1.0

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
    bool fetchVpinMediaDb = false; // download images from vpinmedia
    bool resizeToWindows = false; // resize images to windows sizes
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
    std::string exeDir;
    std::string vpxSubCmd = "-Play";
    std::string vpsDbPath = "data/vpsdb.json";
    std::string vpsDbUpdateFrequency = "startup";
    std::string vpsDbLastUpdated = "data/lastUpdated.json";
    std::string vpxtoolBin = "";
    std::string vpxtoolExtractCmd = "extractvbs";
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
    void applyPostProcessing(const std::string& exeDirectory) {
        // Store exeDir
        this->exeDir = exeDirectory;

        // Resolve VPX paths
        VPXTablesPath = resolvePath(VPXTablesPath, exeDirectory);
        VPinballXPath = resolvePath(VPinballXPath, exeDirectory);

        // List all paths that need resolution
        std::vector<std::string> pathFields = {
            // Default Media Paths
            "defaultPlayfieldImage", "defaultBackglassImage", "defaultDmdImage",
            "defaultWheelImage", "defaultTopperImage", "defaultPlayfieldVideo",
            "defaultBackglassVideo", "defaultDmdVideo", "defaultTopperVideo",

            // UI Sounds Paths ("default sounds")
            "scrollNormalSound", "scrollFastSound", "scrollJumpSound",
            "scrollRandomSound", "launchTableSound", "launchScreenshotSound",
            "panelToggleSound", "screenshotTakeSound", "ambienceSound",

            // Other internal/external paths
            "vpsDbPath", "vpsDbLastUpdated", "indexPath",//"vpxtoolBin", "vpxtoolIndex", "vpxIniPath" // From Internal section
        };

        // Iterate through the list and resolve each path
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
            // UI Sounds
            else if (field == "scrollNormalSound") scrollNormalSound = resolvePath(scrollNormalSound, exeDir);
            else if (field == "scrollFastSound") scrollFastSound = resolvePath(scrollFastSound, exeDir);
            else if (field == "scrollJumpSound") scrollJumpSound = resolvePath(scrollJumpSound, exeDir);
            else if (field == "scrollRandomSound") scrollRandomSound = resolvePath(scrollRandomSound, exeDir);
            else if (field == "launchTableSound") launchTableSound = resolvePath(launchTableSound, exeDir);
            else if (field == "launchScreenshotSound") launchScreenshotSound = resolvePath(launchScreenshotSound, exeDir);
            else if (field == "panelToggleSound") panelToggleSound = resolvePath(panelToggleSound, exeDir);
            else if (field == "screenshotTakeSound") screenshotTakeSound = resolvePath(screenshotTakeSound, exeDir);
            else if (field == "ambienceSound") ambienceSound = resolvePath(ambienceSound, exeDir);
            // Other paths
            else if (field == "vpsDbPath") vpsDbPath = resolvePath(vpsDbPath, exeDir);
            else if (field == "vpsDbLastUpdated") vpsDbLastUpdated = resolvePath(vpsDbLastUpdated, exeDir);
            else if (field == "indexPath") indexPath = resolvePath(indexPath, exeDir);
        }

        // Apply DPI scaling to fontSize if enabled
        if (enableDpiScaling) {
            fontSize = static_cast<int>(static_cast<float>(fontSize) * dpiScale);
        }

        // Resolve font path
        if (fontPath.empty() || !std::filesystem::exists(fontPath)) {
            std::vector<std::string> candidates = {
                "/usr/share/fonts/truetype/dejavu/Arial.ttf",      // Debian/Ubuntu
                "/usr/share/fonts/truetype/dejavu/FreeSans.ttf",
                "/usr/share/fonts/truetype/dejavu/FreeMono.ttf",
                "/usr/share/fonts/TTF/DejaVuSans.ttf",             // Arch
                "/usr/share/fonts/TTF/Arial.ttf",
                "/usr/share/fonts/TTF/FreeSans.ttf",
                "/usr/share/fonts/TTF/FreeMono.ttf",
                "/usr/share/fonts/dejavu/DejaVuSans.ttf",          // Fedora/openSUSE
                "/usr/share/fonts/arial/Arial.ttf",
                "/usr/share/fonts/freesans/FreeSans.ttf",
                "/usr/share/fonts/freemono/FreeMono.ttf",
                "/usr/local/share/fonts/DejaVuSans.ttf",           // Custom system install
                "/usr/local/share/fonts/Arial.ttf",
                "/usr/local/share/fonts/FreeSans.ttf",
                "/usr/local/share/fonts/FreeMono.ttf",
                "/usr/share/fonts/TTF/HackNerdFont-Regular.ttf"    // Mine =]
            };
            for (const auto& path : candidates) {
                if (std::filesystem::exists(path)) {
                    fontPath = path;
                    break;
                }
            }
        }
    }

public:
    // Resolve relative paths and environment variables
    std::string resolvePath(const std::string& value, const std::string& exeDirectory) const {
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
        if (result.empty()) return exeDirectory + value;
        if (result[0] == '/' || result[0] == '\\') return result;
        return exeDirectory + result;
    }

private:
    // JSON serialization
    friend void to_json(nlohmann::json& j, const Settings& s) {
        j = nlohmann::json{
            {"VPX", {
                {"VPXTablesPath", s.VPXTablesPath},
                {"VPinballXPath", s.VPinballXPath},
                {"vpxIniPath", s.vpxIniPath},
                {"vpxStartArgs", s.vpxStartArgs},
                {"vpxEndArgs", s.vpxEndArgs},
                {"autoPatchTables", s.autoPatchTables}
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
                {"useVpxtool", s.useVpxtool},
                {"ignoreScanners", s.ignoreScanners},
                {"forceRebuildMetadata", s.forceRebuildMetadata},
                {"titleSortBy", s.titleSortBy},
                {"titleWeight", s.titleWeight},
                {"yearWeight", s.yearWeight},
                {"manufacturerWeight", s.manufacturerWeight},
                {"romWeight", s.romWeight},
                {"titleThreshold", s.titleThreshold},
                {"confidenceThreshold", s.confidenceThreshold}
            }},
            {"UIWidgets", {
                {"showMetadata", s.showMetadata},
                {"metadataPanelWidth", s.metadataPanelWidth},
                {"metadataPanelHeight", s.metadataPanelHeight},
                {"metadataPanelAlpha", s.metadataPanelAlpha},
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
                {"fetchVpinMediaDb", s.fetchVpinMediaDb},
                {"resizeToWindows", s.resizeToWindows},
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
                {"exeDir", s.exeDir},
                {"vpxSubCmd", s.vpxSubCmd},
                {"vpsDbPath", s.vpsDbPath},
                {"vpsDbUpdateFrequency", s.vpsDbUpdateFrequency},
                {"vpsDbLastUpdated", s.vpsDbLastUpdated},
                {"vpxtoolBin", s.vpxtoolBin},
                {"vpxtoolExtractCmd", s.vpxtoolExtractCmd},
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
        s.autoPatchTables = j.value("VPX", nlohmann::json{}).value("autoPatchTables", s.autoPatchTables);

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
        s.useVpxtool = j.value("TableMetadata", nlohmann::json{}).value("useVpxtool", s.useVpxtool);
        s.forceRebuildMetadata = j.value("TableMetadata", nlohmann::json{}).value("forceRebuildMetadata", s.forceRebuildMetadata);
        s.ignoreScanners = j.value("TableMetadata", nlohmann::json{}).value("ignoreScanners", s.ignoreScanners);
        s.titleSortBy = j.value("TableMetadata", nlohmann::json{}).value("titleSortBy", s.titleSortBy);

        s.titleWeight = j.value("TableMetadata", nlohmann::json{}).value("titleWeight", s.titleWeight);
        s.yearWeight = j.value("TableMetadata", nlohmann::json{}).value("yearWeight", s.yearWeight);
        s.manufacturerWeight = j.value("TableMetadata", nlohmann::json{}).value("manufacturerWeight", s.manufacturerWeight);
        s.romWeight = j.value("TableMetadata", nlohmann::json{}).value("romWeight", s.romWeight);
        s.titleThreshold = j.value("TableMetadata", nlohmann::json{}).value("titleThreshold", s.titleThreshold);
        s.confidenceThreshold = j.value("TableMetadata", nlohmann::json{}).value("confidenceThreshold", s.confidenceThreshold);

        // UIWidgets
        s.showMetadata = j.value("UIWidgets", nlohmann::json{}).value("showMetadata", s.showMetadata);
        s.metadataPanelWidth = j.value("UIWidgets", nlohmann::json{}).value("metadataPanelWidth", s.metadataPanelWidth);
        s.metadataPanelHeight = j.value("UIWidgets", nlohmann::json{}).value("metadataPanelHeight", s.metadataPanelHeight);
        s.metadataPanelAlpha = j.value("UIWidgets", nlohmann::json{}).value("metadataPanelAlpha", s.metadataPanelAlpha);
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
        s.fetchVpinMediaDb = j.value("MediaDimensions", nlohmann::json{}).value("fetchVpinMediaDb", s.fetchVpinMediaDb);
        s.resizeToWindows = j.value("MediaDimensions", nlohmann::json{}).value("resizeToWindows", s.resizeToWindows);
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
        s.exeDir = j.value("Internal", nlohmann::json{}).value("exeDir", s.exeDir);
        s.vpxSubCmd = j.value("Internal", nlohmann::json{}).value("vpxSubCmd", s.vpxSubCmd);
        s.vpsDbPath = j.value("Internal", nlohmann::json{}).value("vpsDbPath", s.vpsDbPath);
        s.vpsDbUpdateFrequency = j.value("Internal", nlohmann::json{}).value("vpsDbUpdateFrequency", s.vpsDbUpdateFrequency);
        s.vpsDbLastUpdated = j.value("Internal", nlohmann::json{}).value("vpsDbLastUpdated", s.vpsDbLastUpdated);
        s.vpxtoolIndex = j.value("Internal", nlohmann::json{}).value("vpxtoolIndex", s.vpxtoolIndex);
        s.vpxtoolBin = j.value("Internal", nlohmann::json{}).value("vpxtoolBin", s.vpxtoolBin);
        s.vpxtoolExtractCmd = j.value("Internal", nlohmann::json{}).value("vpxtoolExtractCmd", s.vpxtoolExtractCmd);
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

// Define settingsMetadata (user-facing help strings)
inline const std::map<std::string, std::pair<Settings::ReloadType, std::string>> Settings::settingsMetadata = {
    // VPX (table playback paths and helpers)
    {"VPXTablesPath", {Settings::ReloadType::Tables, "Absolute path to your VPX table folders.\n\n"
                                                    "Enter the full path where your .vpx table folders are stored.\n"
                                                    "Example: /home/you/Games/VPX_Tables/\n\n"
                                                    "Used when launching tables with VPinballX."}},
    {"VPinballXPath", {Settings::ReloadType::None, "Absolute path to the VPinballX executable.\n\n"}},
    {"vpxIniPath", {Settings::ReloadType::None, "Path to VPinballX.ini (optional).\n\n"
                                               "Leave empty to use the default location (~/.vpinball/VPinballX.ini)."}},
    {"vpxStartArgs", {Settings::ReloadType::None, "Extra command-line arguments to prepend when launching VPinballX.\n\n"
                                                 "These start the command line, added before the launch command."}},
    {"vpxEndArgs", {Settings::ReloadType::None, "Extra command-line arguments to append when launching VPinballX.\n\n"
                                               "These end the command line, added after the launch command."}},
    {"autoPatchTables",{Settings::ReloadType::Tables, "Automatically download and apply community VBScript patch files when scanning tables."}},

    // DPI settings
    {"dpiScale", {Settings::ReloadType::Windows, "Manual DPI scale (used only when automatic DPI scaling is disabled).\n\n"
                                                "Use 1.0 for 100%, 1.5 for 150%, etc. Adjust if UI elements look too small or too large."}},
    {"enableDpiScaling", {Settings::ReloadType::Windows, "When enabled, the frontend will scale the UI automatically based on your monitor DPI.\n\n"
                                                        "Disable this to set a custom DPI with 'dpiScale'."}},

    // Default media files
    {"defaultPlayfieldImage", {Settings::ReloadType::Tables, "Absolute path to the default playfield preview image.\n\n"
                                                            "Shown when a table has no custom preview image."}},
    {"defaultBackglassImage", {Settings::ReloadType::Tables, "Absolute path to the default backglass image used when none is provided by a table."}},
    {"defaultDmdImage", {Settings::ReloadType::Tables, "Absolute path to the default DMD image used when a table provides none."}},
    {"defaultWheelImage", {Settings::ReloadType::Tables, "Absolute path to the default wheel image used when a table provides none."}},
    {"defaultTopperImage", {Settings::ReloadType::Tables, "Absolute path to the default Topper image used when a table provides none."}},
    {"defaultPlayfieldVideo", {Settings::ReloadType::Tables, "Absolute path to the default preview video for playfields (used when table lacks video)."}},
    {"defaultBackglassVideo", {Settings::ReloadType::Tables, "Absolute path to the default backglass preview video (used when table lacks video)."}},
    {"defaultDmdVideo", {Settings::ReloadType::Tables, "Absolute path to the default DMD preview video (used when table lacks video)."}},
    {"defaultTopperVideo", {Settings::ReloadType::Tables, "Absolute path to the default Topper preview video (used when table lacks video)."}},

    // Custom media per-table (relative to each table folder)
    {"customPlayfieldImage", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the playfield preview image."}},
    {"customBackglassImage", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the backglass image."}},
    {"customDmdImage", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the DMD image."}},
    {"customWheelImage", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the wheel image."}},
    {"customTopperImage", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the Topper image."}},
    {"customPlayfieldVideo", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the playfield preview video."}},
    {"customBackglassVideo", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the backglass preview video."}},
    {"customDmdVideo", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the DMD preview video."}},
    {"customTopperVideo", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to the Topper preview video."}},
    {"tableMusic", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to optional table music."}},
    {"customLaunchSound", {Settings::ReloadType::Tables, "Relative path (inside a table folder) to a custom launch sound file."}},

    // Window and renderer settings
    {"videoBackend", {Settings::ReloadType::Assets, "Choose which video backend to use for in-app previews and table media:\n\n"
                                                   "vlc - libVLC-based playback: broad codec support and good compatibility.\n"
                                                   "ffmpeg - internal FFmpeg playback: usually faster and lighter on modern Linux systems.\n"
                                                   "novideo - disable all in-app video (useful for debugging or low-power setups).\n"
                                                   "software - force SDL software rendering for window renderers (diagnostic/testing only)."}},
    {"useVPinballXIni", {Settings::ReloadType::Windows, "If enabled, read sizes and positions from ~/.vpinball/VPinballX.ini.\n\n"
                                                       "NOTE: This will override the manual window position/size settings below."}},
    {"playfieldWindowWidth", {Settings::ReloadType::Assets, "Playfield window width in pixels (match this to your playfield media for best results)."}},
    {"playfieldWindowHeight", {Settings::ReloadType::Assets, "Playfield window height in pixels (match this to your playfield media for best results)."}},
    {"playfieldX", {Settings::ReloadType::Assets, "Playfield window X position on screen.\n\nDrag a window and double-click to save its position."}},
    {"playfieldY", {Settings::ReloadType::Assets, "Playfield window Y position on screen.\n\nDrag a window and double-click to save its position."}},
    {"showBackglass", {Settings::ReloadType::Assets, "Show or hide the Backglass window."}},
    {"backglassWindowWidth", {Settings::ReloadType::Assets, "Backglass window width in pixels."}},
    {"backglassWindowHeight", {Settings::ReloadType::Assets, "Backglass window height in pixels."}},
    {"backglassX", {Settings::ReloadType::Assets, "Backglass window X position on screen.\n\nDrag and double-click a window to save position."}},
    {"backglassY", {Settings::ReloadType::Assets, "Backglass window Y position on screen.\n\nDrag and double-click a window to save position."}},
    {"showDMD", {Settings::ReloadType::Assets, "Show or hide the DMD window."}},
    {"dmdWindowWidth", {Settings::ReloadType::Assets, "DMD window width in pixels."}},
    {"dmdWindowHeight", {Settings::ReloadType::Assets, "DMD window height in pixels."}},
    {"dmdX", {Settings::ReloadType::Assets, "DMD window X position on screen."}},
    {"dmdY", {Settings::ReloadType::Assets, "DMD window Y position on screen."}},
    {"showTopper", {Settings::ReloadType::Assets, "Show or hide the Topper window."}},
    {"topperWindowWidth", {Settings::ReloadType::Assets, "Topper window width in pixels."}},
    {"topperWindowHeight", {Settings::ReloadType::Assets, "Topper window height in pixels."}},
    {"topperWindowX", {Settings::ReloadType::Assets, "Topper window X position on screen."}},
    {"topperWindowY", {Settings::ReloadType::Assets, "Topper window Y position on screen."}},

    // Table metadata matching and scanning
    {"titleSource", {Settings::ReloadType::Tables, "Choose how table metadata is found:\n\n"
                                                  "filename' - use filename-based heuristics;\n"
                                                  "metadata' - run the scanners to extract embedded metadata."}},
    {"fetchVPSdb", {Settings::ReloadType::Tables, "Download and use the Virtual Pinball Spreadsheet db (community database)\nto improve automatic table matching and metadata accuracy."}},
    {"useVpxtool",{Settings::ReloadType::Tables, "Use an external vpxtool index (if available) or run vpxtool to extract table metadata\ninstead of built-in VPin scanner."}},
    {"ignoreScanners", {Settings::ReloadType::Tables, "Skip local scanners and read metadata only from the index file.\n\nUseful for when you have a trusted pre-built index."}},
    {"forceRebuildMetadata", {Settings::ReloadType::Tables, "Rebuild all table metadata from scratch.\n\nUse this if metadata is incorrect or you changed scanner settings."}},
    {"titleSortBy", {Settings::ReloadType::Tables, "Choose the field used to sort the table list (requires VPSdb metadata for some options)."}},
    {"titleWeight", {Settings::ReloadType::None, "How much title similarity influences automatic matching (higher = stronger influence)."}},
    {"yearWeight", {Settings::ReloadType::None, "How much year similarity influences automatic matching (higher = stronger influence)."}},
    {"manufacturerWeight", {Settings::ReloadType::None, "How much manufacturer similarity influences automatic matching (higher = stronger influence)."}},
    {"romWeight", {Settings::ReloadType::None, "How much ROM name similarity influences automatic matching (higher = stronger influence)."}},
    {"titleThreshold", {Settings::ReloadType::None, "Minimum similarity score required for a title to be considered a match (higher = stricter)."}},
    {"confidenceThreshold", {Settings::ReloadType::None, "Overall confidence threshold required to accept a metadata match (higher = stricter)."}},

    // UI widget settings
    {"showMetadata", {Settings::ReloadType::Overlay, "Show or hide the metadata overlay panel on the playfield window."}},
    {"metadataPanelWidth", {Settings::ReloadType::Overlay, "Metadata panel width as a fraction of the screen (0.1-1.0)."}},
    {"metadataPanelHeight", {Settings::ReloadType::Overlay, "Metadata panel height as a fraction of the screen (0.1-1.0)."}},
    {"metadataPanelAlpha", {Settings::ReloadType::Overlay, "Transparency of the metadata panel (0.0-1.0)."}},
    {"showArrowHint", {Settings::ReloadType::None, "Toggle the small arrow hint widget in the UI."}},
    {"arrowHintWidth", {Settings::ReloadType::None, "Width of the arrow hint in pixels."}},
    {"arrowHintHeight", {Settings::ReloadType::None, "Height of the arrow hint in pixels."}},
    {"arrowThickness", {Settings::ReloadType::None, "Thickness of the arrow hint in pixels."}},
    {"arrowAlpha", {Settings::ReloadType::None, "Transparency of the arrow hint (0.0-1.0)."}},
    {"arrowGlow", {Settings::ReloadType::None, "Glow size for the arrow hint."}},
    {"arrowGlowColor", {Settings::ReloadType::None, "Glow color for the arrow hint (RGBA)."}},
    {"arrowColorTop", {Settings::ReloadType::None, "Top gradient color for the arrow hint (RGBA)."}},
    {"arrowColorBottom", {Settings::ReloadType::None, "Bottom gradient color for the arrow hint (RGBA)."}},
    {"showScrollbar", {Settings::ReloadType::None, "Show or hide the sidebar scrollbar."}},
    {"scrollbarWidth", {Settings::ReloadType::None, "Scrollbar width in pixels."}},
    {"thumbWidth", {Settings::ReloadType::None, "Scrollbar thumb width in pixels."}},
    {"scrollbarLength", {Settings::ReloadType::None, "Scrollbar length as a fraction of the window (0.1-1.0)."}},
    {"scrollbarColor", {Settings::ReloadType::None, "Scrollbar color (RGBA)."}},
    {"scrollbarThumbColor", {Settings::ReloadType::None, "Scrollbar thumb color (RGBA)."}},

    // Title display
    {"showWheel", {Settings::ReloadType::None, "Show or hide the wheel artwork in it's window."}},
    {"wheelWindow", {Settings::ReloadType::Tables, "Which window should display the wheel artwork."}},
    {"showTitle", {Settings::ReloadType::None, "Show or hide the table title text in it's window."}},
    {"titleWindow", {Settings::ReloadType::Tables, "Which window should display the table title text."}},
    {"fontPath", {Settings::ReloadType::Font, "Path to the font used for table titles."}},
    {"fontColor", {Settings::ReloadType::Font, "Color used to render table title text."}},
    {"fontBgColor", {Settings::ReloadType::Font, "Background color behind table title text (RGBA)."}},
    {"fontSize", {Settings::ReloadType::Font, "Font size (points) used for the table title."}},
    {"titleX", {Settings::ReloadType::Title, "Horizontal position (X) for the title text."}},
    {"titleY", {Settings::ReloadType::Title, "Vertical position (Y) for the title text."}},

    // Media dimensions and behavior
    {"fetchVpinMediaDb", {Settings::ReloadType::Tables, "Download images from the VPin Media Database (requires VPSdb metadata)."}},
    {"resizeToWindows", {Settings::ReloadType::Tables, "Automatically resize VPin Media Database downloaded images to match your\ncurrent window dimensions to save memory and keep layout consistent."}},
    {"forceImagesOnly", {Settings::ReloadType::Tables, "If enabled, the frontend will load images only and skip videos."}},
    {"wheelMediaHeight", {Settings::ReloadType::None, "Wheel image height in pixels."}},
    {"wheelMediaWidth", {Settings::ReloadType::None, "Wheel image width in pixels."}},
    {"wheelMediaX", {Settings::ReloadType::None, "Wheel image X coordinate inside its window."}},
    {"wheelMediaY", {Settings::ReloadType::None, "Wheel image Y coordinate inside its window."}},
    {"playfieldMediaWidth", {Settings::ReloadType::None, "Playfield media width in pixels."}},
    {"playfieldMediaHeight", {Settings::ReloadType::None, "Playfield media height in pixels."}},
    {"playfieldMediaX", {Settings::ReloadType::None, "Playfield media X coordinate (relative to playfield window)."}},
    {"playfieldMediaY", {Settings::ReloadType::None, "Playfield media Y coordinate (relative to playfield window)."}},
    {"playfieldRotation", {Settings::ReloadType::None, "Rotation to apply to playfield media (degrees)."}},
    {"backglassMediaWidth", {Settings::ReloadType::None, "Backglass media width in pixels."}},
    {"backglassMediaHeight", {Settings::ReloadType::None, "Backglass media height in pixels."}},
    {"backglassMediaX", {Settings::ReloadType::None, "Backglass media X coordinate (relative to backglass window)."}},
    {"backglassMediaY", {Settings::ReloadType::None, "Backglass media Y coordinate (relative to backglass window)."}},
    {"backglassRotation", {Settings::ReloadType::None, "Rotation to apply to backglass media (degrees)."}},
    {"dmdMediaWidth", {Settings::ReloadType::None, "DMD media width in pixels."}},
    {"dmdMediaHeight", {Settings::ReloadType::None, "DMD media height in pixels."}},
    {"dmdMediaX", {Settings::ReloadType::None, "DMD media X coordinate (relative to DMD window)."}},
    {"dmdMediaY", {Settings::ReloadType::None, "DMD media Y coordinate (relative to DMD window)."}},
    {"dmdRotation", {Settings::ReloadType::None, "Rotation to apply to DMD media (degrees)."}},
    {"topperMediaWidth", {Settings::ReloadType::None, "Topper media width in pixels."}},
    {"topperMediaHeight", {Settings::ReloadType::None, "Topper media height in pixels."}},
    {"topperMediaX", {Settings::ReloadType::None, "Topper media X coordinate (relative to Topper window)."}},
    {"topperMediaY", {Settings::ReloadType::None, "Topper media Y coordinate (relative to Topper window)."}},
    {"topperRotation", {Settings::ReloadType::None, "Rotation to apply to Topper media (degrees)."}},

    // Audio
    {"masterMute", {Settings::ReloadType::Audio, "Mute all audio output."}},
    {"masterVol", {Settings::ReloadType::Audio, "Master volume for all audio output (0-100)."}},
    {"mediaAudioMute", {Settings::ReloadType::Audio, "Mute playfield, backglass, and DMD audio."}},
    {"mediaAudioVol", {Settings::ReloadType::Audio, "Volume for playfield/backglass/DMD audio (0-100)."}},
    {"tableMusicMute", {Settings::ReloadType::Audio, "Mute background table music."}},
    {"tableMusicVol", {Settings::ReloadType::Audio, "Volume for background table music (0-100)."}},
    {"interfaceAudioMute", {Settings::ReloadType::Audio, "Mute UI/interface sounds."}},
    {"interfaceAudioVol", {Settings::ReloadType::Audio, "Volume for UI/interface sounds (0-100)."}},
    {"interfaceAmbienceMute", {Settings::ReloadType::Audio, "Mute ambient interface sounds."}},
    {"interfaceAmbienceVol", {Settings::ReloadType::Audio, "Volume for ambient interface sounds (0-100)."}},

    // UI sound clips
    {"scrollNormalSound", {Settings::ReloadType::Tables, "Sound played when moving the selection by one."}},
    {"scrollFastSound", {Settings::ReloadType::Tables, "Sound played when fast-scrolling through the list."}},
    {"scrollJumpSound", {Settings::ReloadType::Tables, "Sound played when jumping by letter."}},
    {"scrollRandomSound", {Settings::ReloadType::Tables, "Sound played when selecting a random table."}},
    {"launchTableSound", {Settings::ReloadType::Tables, "Sound played when launching a table."}},
    {"launchScreenshotSound", {Settings::ReloadType::Tables, "Sound played when entering screenshot mode."}},
    {"panelToggleSound", {Settings::ReloadType::Tables, "Sound played when opening or closing UI panels."}},
    {"screenshotTakeSound", {Settings::ReloadType::Tables, "Sound played when taking a screenshot."}},
    {"ambienceSound", {Settings::ReloadType::Tables, "Ambient background sound used when a table has no music."}},

    // Internal paths and timing
    {"exeDir", {Settings::ReloadType::None, "Path to the application executable directory."}},
    {"vpxSubCmd", {Settings::ReloadType::None, "VPinballX command used to play .vpx tables (internal helper)."}},
    {"vpsDbPath", {Settings::ReloadType::None, "Path to the VPS database file (relative to the executable directory)."}},
    {"vpsDbUpdateFrequency", {Settings::ReloadType::None, "When to check for VPSdb updates (e.g., 'startup')."}},
    {"vpsDbLastUpdated", {Settings::ReloadType::None, "Path to the VPS database timestamp file (relative to executable)."}},
    {"vpxtoolIndex", {Settings::ReloadType::None, "Path to the vpxtool index file (defaults to vpxtool_index.json)."}},
    {"vpxtoolBin", {Settings::ReloadType::None, "Path to the vpxtool binary, if it is not on your PATH."}},
    {"vpxtoolExtractCmd", {Settings::ReloadType::None, "The vpxtool subcommand for extracting VBScripts."}},
    {"indexPath", {Settings::ReloadType::None, "Path to the main table index file (relative to the executable)."}},
    {"screenshotWait", {Settings::ReloadType::None, "Seconds to wait for visible windows when using the screenshot tool."}},
    {"configUIWidth", {Settings::ReloadType::None, "Configuration UI width (fraction of screen)."}},
    {"configUIHeight", {Settings::ReloadType::None, "Configuration UI height (fraction of screen)."}},

    // Keybind descriptions
    {"Previous Table", {Settings::ReloadType::None, "Key to select the previous table."}},
    {"Next Table", {Settings::ReloadType::None, "Key to select the next table."}},
    {"Fast Previous Table", {Settings::ReloadType::None, "Key to jump back by 10 tables."}},
    {"Fast Next Table", {Settings::ReloadType::None, "Key to jump forward by 10 tables."}},
    {"Jump Next Letter", {Settings::ReloadType::None, "Key to jump to the next table starting with a different letter."}},
    {"Jump Previous Letter", {Settings::ReloadType::None, "Key to jump to the previous table starting with a different letter."}},
    {"Random Table", {Settings::ReloadType::None, "Key to select a random table from the list."}},
    {"Launch Table", {Settings::ReloadType::None, "Key to launch the currently selected table."}},
    {"Toggle Config", {Settings::ReloadType::None, "Key to open or close the configuration menu."}},
    {"Quit", {Settings::ReloadType::None, "Key to quit the application or close menus."}},
    {"Screenshot Mode", {Settings::ReloadType::None, "Key to open screenshot launch mode for capturing table images."}},
    {"Screenshot Key", {Settings::ReloadType::None, "Key to capture a screenshot while in screenshot mode."}},
    {"Screenshot Quit", {Settings::ReloadType::None, "Key to exit screenshot mode."}},
    {"Toggle Editor", {Settings::ReloadType::None, "Key to open the metadata editor."}},
    {"Toggle Catalog", {Settings::ReloadType::None, "Key to open the metadata catalog."}}
};

#endif // SETTINGS_H
