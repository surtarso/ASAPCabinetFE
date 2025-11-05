#ifndef SECTION_CONFIG_H
#define SECTION_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @class SectionConfig
 * @brief Manages section and key order for configuration UI rendering.
 */
class SectionConfig {
public:
    SectionConfig() {
        // ----------------- SECTIONS ------------------
        sectionOrder_ = {
            "TableMetadata",
            "TitleDisplay",
            "UIWidgets",
            "AudioSettings",
            "Keybinds",
            "DPISettings",
            "WindowSettings",
            "MediaDimensions",
            "CustomMedia",
            "VPX",
            "DefaultMedia",
            "UISounds",
            "Internal"
        };
        sectionDisplayNames_ = {
            {"TableMetadata", "Table matching & metadata (source, indexing)"},
            {"TitleDisplay", "On-screen titles & wheel (font, position)"},
            {"UIWidgets", "UI elements (arrows, scrollbars, visuals)"},
            {"AudioSettings", "Global audio levels and mutes"},
            {"Keybinds", "Keyboard / Controller shortcuts"},
            {"DPISettings", "Display scale / DPI behavior"},
            {"WindowSettings", "Window & placement settings"},
            {"MediaDimensions", "Media sizing & placement for each window"},
            {"CustomMedia", "Per-table custom media paths"},
            {"VPX", "VPX integration paths & launch options"},
            {"DefaultMedia", "Fallback default media (used when table media missing)"},
            {"UISounds", "Interface sound effects"},
            {"Internal", "Advanced / internal settings (indexes, DB paths)"}
        };
        // -------------------- KEYS --------------------
        keyOrders_["TableMetadata"] = {
            "titleSource",
            "titleSortBy",
            "showMetadata",
            "metadataPanelWidth",
            "metadataPanelHeight",
            "metadataPanelAlpha",
            "useVpxtool",
            "fetchVPSdb",
            "fetchVpinMediaDb",
            "resizeToWindows",
            "ignoreScanners",
            "forceRebuildMetadata",
            "titleWeight",
            "yearWeight",
            "manufacturerWeight",
            "romWeight",
            "titleThreshold",
            "confidenceThreshold"
        };
        keyDisplayNames_["TableMetadata"] = {
            {"titleSource", "Title Source"},
            {"titleSortBy", "Sort By"},
            {"useVpxtool", "Enable VPXTool"},
            {"fetchVPSdb", "Use VPS DB"},
            {"fetchVpinMediaDb", "Use VPinMedia DB"},
            {"resizeToWindows", "Resize media to window"},
            {"showMetadata", "Display Metadata"},
            {"metadataPanelWidth", "Panel Width"},
            {"metadataPanelHeight", "Panel Height"},
            {"metadataPanelAlpha", "Panel Opacity"},
            {"ignoreScanners", "Fast load (skip scanners)"},
            {"forceRebuildMetadata", "Force rebuild metadata"},
            {"titleWeight", "Title weight"},
            {"yearWeight", "Year weight"},
            {"manufacturerWeight", "Manufacturer weight"},
            {"romWeight", "ROM weight"},
            {"titleThreshold", "Title similarity"},
            {"confidenceThreshold", "Overall confidence"}
        };
        keyDropdownOptions_["TableMetadata"] = {
            {"titleSource", {"filename", "metadata"}},
            {"titleSortBy", {"title", "author", "type", "year", "manufacturer"}}
        };

        keyOrders_["TitleDisplay"] = {
            "showWheel",
            "wheelWindow",
            "showTitle",
            "titleWindow",
            "fontPath",
            "fontColor",
            "fontBgColor",
            "fontSize",
            "titleX",
            "titleY"
        };
        keyDisplayNames_["TitleDisplay"] = {
            {"showWheel", "Show wheel"},
            {"wheelWindow", "Wheel window"},
            {"showTitle", "Show title"},
            {"titleWindow", "Title window"},
            {"fontPath", "Font path"},
            {"fontColor", "Font color"},
            {"fontBgColor", "Font BG color"},
            {"fontSize", "Font size"},
            {"titleX", "Title X"},
            {"titleY", "Title Y"}
        };
        keyDropdownOptions_["TitleDisplay"] = {
            {"wheelWindow", {"playfield", "backglass", "dmd", "topper"}},
            {"titleWindow", {"playfield", "backglass", "dmd", "topper"}}
        };

        keyOrders_["UIWidgets"] = {
            "showArrowHint",
            "arrowHintWidth",
            "arrowHintHeight",
            "arrowThickness",
            "arrowAlpha",
            "arrowColorTop",
            "arrowColorBottom",
            "arrowGlow",
            "arrowGlowColor",
            "showScrollbar",
            "scrollbarWidth",
            "thumbWidth",
            "scrollbarLength",
            "scrollbarColor",
            "scrollbarThumbColor"
        };
        keyDisplayNames_["UIWidgets"] = {
            {"showArrowHint", "Show arrow hint"},
            {"arrowHintWidth", "Arrow width"},
            {"arrowHintHeight", "Arrow height"},
            {"arrowThickness", "Arrow thickness"},
            {"arrowAlpha", "Arrow opacity"},
            {"arrowColorTop", "Arrow color (top)"},
            {"arrowColorBottom", "Arrow color (bottom)"},
            {"arrowGlow", "Arrow glow"},
            {"arrowGlowColor", "Glow color"},
            {"showScrollbar", "Show scrollbar"},
            {"scrollbarWidth", "Scrollbar width"},
            {"thumbWidth", "Thumb width"},
            {"scrollbarLength", "Scrollbar length"},
            {"scrollbarColor", "Scrollbar color"},
            {"scrollbarThumbColor", "Scrollbar thumb color"}
        };

        keyOrders_["AudioSettings"] = {
            "masterVol",
            "masterMute",
            "mediaAudioVol",
            "mediaAudioMute",
            "tableMusicVol",
            "tableMusicMute",
            "interfaceAudioVol",
            "interfaceAudioMute",
            "interfaceAmbienceVol",
            "interfaceAmbienceMute"
        };
        keyDisplayNames_["AudioSettings"] = {
            {"masterVol", "Master volume"},
            {"masterMute", "Master mute"},
            {"mediaAudioVol", "Media volume"},
            {"mediaAudioMute", "Media mute"},
            {"tableMusicVol", "Table music vol"},
            {"tableMusicMute", "Table music mute"},
            {"interfaceAudioVol", "Interface volume"},
            {"interfaceAudioMute", "Interface mute"},
            {"interfaceAmbienceVol", "Ambience volume"},
            {"interfaceAmbienceMute", "Ambience mute"}
        };

        keyOrders_["Keybinds"] = {
            "Previous Table", "Next Table", "Fast Previous Table", "Fast Next Table",
            "Jump Next Letter", "Jump Previous Letter", "Random Table", "Launch Table",
            "Toggle Config", "Toggle Editor", "Toggle Catalog", "Screenshot Mode",
            "Screenshot Key", "Screenshot Quit", "Quit"
        };
        keyDisplayNames_["Keybinds"] = {
            {"Previous Table", "Previous Table"},
            {"Next Table", "Next Table"},
            {"Fast Previous Table", "Fast Previous Table"},
            {"Fast Next Table", "Fast Next Table"},
            {"Jump Next Letter", "Jump Next Letter"},
            {"Jump Previous Letter", "Jump Previous Letter"},
            {"Random Table", "Random Table"},
            {"Launch Table", "Launch Table"},
            {"Screenshot Mode", "Screenshot Mode"},
            {"Screenshot Key", "Screenshot Key"},
            {"Screenshot Quit", "Screenshot Quit"},
            {"Toggle Config", "Toggle Config"},
            {"Toggle Editor", "Toggle Editor"},
            {"Toggle Catalog", "Toggle Catalog"},
            {"Quit", "Quit"}
        };

        keyOrders_["DPISettings"] = {
            "dpiScale",
            "enableDpiScaling"
        };
        keyDisplayNames_["DPISettings"] = {
            {"dpiScale", "DPI scale"},
            {"enableDpiScaling", "Enable DPI scaling"}
        };

        keyOrders_["WindowSettings"] = {
            "videoBackend",
            "useVPinballXIni",
            "playfieldWindowWidth",
            "playfieldWindowHeight",
            "playfieldX",
            "playfieldY",
            "showBackglass",
            "backglassWindowWidth",
            "backglassWindowHeight",
            "backglassX",
            "backglassY",
            "showDMD",
            "dmdWindowWidth",
            "dmdWindowHeight",
            "dmdX",
            "dmdY",
            "showTopper",
            "topperWindowWidth",
            "topperWindowHeight",
            "topperWindowX",
            "topperWindowY"
        };
        keyDisplayNames_["WindowSettings"] = {
            {"videoBackend", "Video backend"},
            {"useVPinballXIni", "Use VPX INI"},
            {"playfieldWindowWidth", "Playfield width"},
            {"playfieldWindowHeight", "Playfield height"},
            {"playfieldX", "Playfield X"},
            {"playfieldY", "Playfield Y"},
            {"showBackglass", "Show backglass"},
            {"backglassWindowWidth", "Backglass width"},
            {"backglassWindowHeight", "Backglass height"},
            {"backglassX", "Backglass X"},
            {"backglassY", "Backglass Y"},
            {"showDMD", "Show DMD"},
            {"dmdWindowWidth", "DMD width"},
            {"dmdWindowHeight", "DMD height"},
            {"dmdX", "DMD X"},
            {"dmdY", "DMD Y"},
            {"showTopper", "Show topper"},
            {"topperWindowWidth", "Topper width"},
            {"topperWindowHeight", "Topper height"},
            {"topperWindowX", "Topper X"},
            {"topperWindowY", "Topper Y"}
        };
        keyDropdownOptions_["WindowSettings"] = {
            {"videoBackend", {"vlc", "ffmpeg", "novideo", "software"}}
        };

        keyOrders_["MediaDimensions"] = {
            "forceImagesOnly",
            "wheelMediaWidth",
            "wheelMediaHeight",
            "wheelMediaX",
            "wheelMediaY",
            "playfieldMediaWidth",
            "playfieldMediaHeight",
            "playfieldMediaX",
            "playfieldMediaY",
            "playfieldRotation",
            "backglassMediaWidth",
            "backglassMediaHeight",
            "backglassMediaX",
            "backglassMediaY",
            "backglassRotation",
            "dmdMediaWidth",
            "dmdMediaHeight",
            "dmdMediaX",
            "dmdMediaY",
            "dmdRotation",
            "topperMediaWidth",
            "topperMediaHeight",
            "topperMediaX",
            "topperMediaY",
            "topperRotation"
        };
        keyDisplayNames_["MediaDimensions"] = {
            {"forceImagesOnly", "Force images only"},
            {"wheelMediaWidth", "Wheel width"},
            {"wheelMediaHeight", "Wheel height"},
            {"wheelMediaX", "Wheel X"},
            {"wheelMediaY", "Wheel Y"},
            {"playfieldMediaWidth", "Playfield media width"},
            {"playfieldMediaHeight", "Playfield media height"},
            {"playfieldMediaX", "Playfield media X"},
            {"playfieldMediaY", "Playfield media Y"},
            {"playfieldRotation", "Playfield rotation"},
            {"backglassMediaWidth", "Backglass media width"},
            {"backglassMediaHeight", "Backglass media height"},
            {"backglassMediaX", "Backglass media X"},
            {"backglassMediaY", "Backglass media Y"},
            {"backglassRotation", "Backglass rotation"},
            {"dmdMediaWidth", "DMD media width"},
            {"dmdMediaHeight", "DMD media height"},
            {"dmdMediaX", "DMD media X"},
            {"dmdMediaY", "DMD media Y"},
            {"dmdRotation", "DMD rotation"},
            {"topperMediaWidth", "Topper media width"},
            {"topperMediaHeight", "Topper media height"},
            {"topperMediaX", "Topper media X"},
            {"topperMediaY", "Topper media Y"},
            {"topperRotation", "Topper rotation"}
        };

        keyOrders_["CustomMedia"] = {
            "customWheelImage",
            "customPlayfieldImage",
            "customBackglassImage",
            "customDmdImage",
            "customTopperImage",
            "customPlayfieldVideo",
            "customBackglassVideo",
            "customDmdVideo",
            "customTopperVideo",
            "tableMusic",
            "customLaunchSound"
        };
        keyDisplayNames_["CustomMedia"] = {
            {"customWheelImage", "Wheel image"},
            {"customPlayfieldImage", "Playfield image"},
            {"customBackglassImage", "Backglass image"},
            {"customDmdImage", "DMD image"},
            {"customTopperImage", "Topper image"},
            {"customPlayfieldVideo", "Playfield video"},
            {"customBackglassVideo", "Backglass video"},
            {"customDmdVideo", "DMD video"},
            {"customTopperVideo", "Topper video"},
            {"tableMusic", "Table music"},
            {"customLaunchSound", "Launch sound"}
        };

        keyOrders_["VPX"] = {
            "VPXTablesPath",
            "VPinballXPath",
            "vpxIniPath",
            "vpxStartArgs",
            "vpxEndArgs",
            "autoPatchTables"
        };
        keyDisplayNames_["VPX"] = {
            {"VPXTablesPath", "VPX Tables Path"},
            {"VPinballXPath", "VPinballX Path"},
            {"vpxIniPath", "VPX INI Path"},
            {"vpxStartArgs", "VPX Start Arguments"},
            {"vpxEndArgs", "VPX End Arguments"},
            {"autoPatchTables", "Automatically Patch Tables"}
        };

        keyOrders_["DefaultMedia"] = {
            "defaultWheelImage",
            "defaultPlayfieldImage",
            "defaultBackglassImage",
            "defaultDmdImage",
            "defaultTopperImage",
            "defaultPlayfieldVideo",
            "defaultBackglassVideo",
            "defaultDmdVideo",
            "defaultTopperVideo"
        };
        keyDisplayNames_["DefaultMedia"] = {
            {"defaultWheelImage", "Default wheel image"},
            {"defaultPlayfieldImage", "Default playfield image"},
            {"defaultBackglassImage", "Default backglass image"},
            {"defaultDmdImage", "Default DMD image"},
            {"defaultTopperImage", "Default topper image"},
            {"defaultPlayfieldVideo", "Default playfield video"},
            {"defaultBackglassVideo", "Default backglass video"},
            {"defaultDmdVideo", "Default DMD video"},
            {"defaultTopperVideo", "Default topper video"}
        };

        keyOrders_["UISounds"] = {
            "scrollNormalSound",
            "scrollFastSound",
            "scrollJumpSound",
            "scrollRandomSound",
            "launchTableSound",
            "launchScreenshotSound",
            "panelToggleSound",
            "screenshotTakeSound",
            "ambienceSound"
        };
        keyDisplayNames_["UISounds"] = {
            {"scrollNormalSound", "Scroll (normal)"},
            {"scrollFastSound", "Scroll (fast)"},
            {"scrollJumpSound", "Scroll (jump)"},
            {"scrollRandomSound", "Scroll (random)"},
            {"launchTableSound", "Launch table"},
            {"launchScreenshotSound", "Launch screenshot"},
            {"panelToggleSound", "Panel toggle"},
            {"screenshotTakeSound", "Screenshot take"},
            {"ambienceSound", "Ambience"}
        };

        keyOrders_["Internal"] = {
            "vpxSubCmd",
            "vpsDbPath",
            "vpsDbUpdateFrequency",
            "vpsDbLastUpdated",
            "vpxtoolIndex",
            "indexPath",
            "screenshotWait"
        };
        keyDisplayNames_["Internal"] = {
            {"vpxSubCmd", "VPX Subcommand"},
            {"vpsDbPath", "VPS Database Path"},
            {"vpsDbUpdateFrequency", "VPS Database Update Frequency"},
            {"vpsDbLastUpdated", "VPS Database Last Updated"},
            {"vpxtoolIndex", "VPX Tool Index"},
            {"indexPath", "Index Path"},
            {"screenshotWait", "Screenshot Wait Time"}
        };
    }

    const std::vector<std::string>& getSectionOrder() const { return sectionOrder_; }
    const std::vector<std::string>& getKeyOrder(const std::string& sectionName) const {
        static const std::vector<std::string> empty;
        auto it = keyOrders_.find(sectionName);
        return it != keyOrders_.end() ? it->second : empty;
    }
    std::string getSectionDisplayName(const std::string& sectionName) const {
        auto it = sectionDisplayNames_.find(sectionName);
        return it != sectionDisplayNames_.end() ? it->second : sectionName;
    }
    std::string getKeyDisplayName(const std::string& sectionName, const std::string& key) const {
        auto sectionIt = keyDisplayNames_.find(sectionName);
        if (sectionIt != keyDisplayNames_.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                return keyIt->second;
            }
        }
        return key;
    }
    const std::vector<std::string>& getDropdownOptions(const std::string& sectionName, const std::string& key) const {
        static const std::vector<std::string> empty;
        auto sectionIt = keyDropdownOptions_.find(sectionName);
        if (sectionIt != keyDropdownOptions_.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                return keyIt->second;
            }
        }
        return empty;
    }

private:
    std::vector<std::string> sectionOrder_;
    std::unordered_map<std::string, std::string> sectionDisplayNames_;
    std::unordered_map<std::string, std::vector<std::string>> keyOrders_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> keyDisplayNames_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> keyDropdownOptions_;
};

#endif // SECTION_CONFIG_H
