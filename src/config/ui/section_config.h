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
            {"TableMetadata", "Table Metadata"},
            {"TitleDisplay", "Title Display"},
            {"UIWidgets", "UI Widgets"},
            {"AudioSettings", "Audio Settings"},
            {"Keybinds", "Keybinds"},
            {"DPISettings", "DPI Settings"},
            {"WindowSettings", "Window Settings"},
            {"MediaDimensions", "Media Dimensions"},
            {"CustomMedia", "Custom Media"},
            {"VPX", "VPX Settings"},
            {"DefaultMedia", "Default Media"},
            {"UISounds", "UI Sounds"},
            {"Internal", "Internal Settings"}
        };
        // -------------------- KEYS --------------------
        keyOrders_["TableMetadata"] = {
            "titleSource",
            "titleSortBy",
            "showMetadata",
            "metadataPanelWidth",
            "metadataPanelHeight",
            "metadataPanelAlpha",
            "fetchVPSdb",
            "forceRebuildMetadata",
            "titleWeight",
            "yearWeight",
            "manufacturerWeight",
            "romWeight",
            "titleThreshold",
            "confidenceThreshold"
        };
        keyDisplayNames_["TableMetadata"] = {
            {"titleSource", "Table Info"},
            {"titleSortBy", "Sort By"},
            {"showMetadata", "Show Metadata"},
            {"metadataPanelWidth", "Panel Width"},
            {"metadataPanelHeight", "Panel Height"},
            {"metadataPanelAlpha", "Panel Opacity"},
            {"fetchVPSdb", "Fetch VPS Database"},
            {"forceRebuildMetadata", "Rebuild Metadata"}
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
            {"showWheel", "Show Wheel"},
            {"wheelWindow", "Wheel Window"},
            {"showTitle", "Show Title"},
            {"titleWindow", "Title Window"},
            {"fontPath", "Font Path"},
            {"fontColor", "Font Color"},
            {"fontBgColor", "Font Background Color"},
            {"fontSize", "Font Size"},
            {"titleX", "Title X Position"},
            {"titleY", "Title Y Position"}
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
            {"showArrowHint", "Show Arrow Hint"},
            {"arrowHintWidth", "Arrow Width"},
            {"arrowHintHeight", "Arrow Height"},
            {"arrowThickness", "Arrow Thickness"},
            {"arrowAlpha", "Arrow Opacity"},
            {"arrowColorTop", "Arrow Top Color"},
            {"arrowColorBottom", "Arrow Bottom Color"},
            {"arrowGlow", "Arrow Glow"},
            {"arrowGlowColor", "Arrow Glow Color"},
            {"showScrollbar", "Show Scrollbar"},
            {"scrollbarWidth", "Scrollbar Width"},
            {"thumbWidth", "Thumb Width"},
            {"scrollbarLength", "Scrollbar Length"},
            {"scrollbarColor", "Scrollbar Color"},
            {"scrollbarThumbColor", "Scrollbar Thumb Color"}
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
            {"masterVol", "Master Volume"},
            {"masterMute", "Master Mute"},
            {"mediaAudioVol", "Media Volume"},
            {"mediaAudioMute", "Media Mute"},
            {"tableMusicVol", "Table Music Volume"},
            {"tableMusicMute", "Table Music Mute"},
            {"interfaceAudioVol", "Interface Volume"},
            {"interfaceAudioMute", "Interface Mute"},
            {"interfaceAmbienceVol", "Ambience Volume"},
            {"interfaceAmbienceMute", "Ambience Mute"}
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
            {"dpiScale", "DPI Scale"},
            {"enableDpiScaling", "Enable DPI Scaling"}
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
            {"videoBackend", "Video Backend"},
            {"useVPinballXIni", "Use VPX INI"},
            {"playfieldWindowWidth", "Playfield Width"},
            {"playfieldWindowHeight", "Playfield Height"},
            {"playfieldX", "Playfield X"},
            {"playfieldY", "Playfield Y"},
            {"showBackglass", "Show Backglass"},
            {"backglassWindowWidth", "Backglass Width"},
            {"backglassWindowHeight", "Backglass Height"},
            {"backglassX", "Backglass X"},
            {"backglassY", "Backglass Y"},
            {"showDMD", "Show DMD"},
            {"dmdWindowWidth", "DMD Width"},
            {"dmdWindowHeight", "DMD Height"},
            {"dmdX", "DMD X"},
            {"dmdY", "DMD Y"},
            {"showTopper", "Show Topper"},
            {"topperWindowWidth", "Topper Width"},
            {"topperWindowHeight", "Topper Height"},
            {"topperWindowX", "Topper X"},
            {"topperWindowY", "Topper Y"}
        };
        keyDropdownOptions_["WindowSettings"] = {
            {"videoBackend", {"vlc", "ffmpeg", "novideo"}}
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
            {"forceImagesOnly", "Force Images Only"},
            {"wheelMediaWidth", "Wheel Width"},
            {"wheelMediaHeight", "Wheel Height"},
            {"wheelMediaX", "Wheel X"},
            {"wheelMediaY", "Wheel Y"},
            {"playfieldMediaWidth", "Playfield Media Width"},
            {"playfieldMediaHeight", "Playfield Media Height"},
            {"playfieldMediaX", "Playfield Media X"},
            {"playfieldMediaY", "Playfield Media Y"},
            {"playfieldRotation", "Playfield Rotation"},
            {"backglassMediaWidth", "Backglass Media Width"},
            {"backglassMediaHeight", "Backglass Media Height"},
            {"backglassMediaX", "Backglass Media X"},
            {"backglassMediaY", "Backglass Media Y"},
            {"backglassRotation", "Backglass Rotation"},
            {"dmdMediaWidth", "DMD Media Width"},
            {"dmdMediaHeight", "DMD Media Height"},
            {"dmdMediaX", "DMD Media X"},
            {"dmdMediaY", "DMD Media Y"},
            {"dmdRotation", "DMD Rotation"},
            {"topperMediaWidth", "Topper Media Width"},
            {"topperMediaHeight", "Topper Media Height"},
            {"topperMediaX", "Topper Media X"},
            {"topperMediaY", "Topper Media Y"},
            {"topperRotation", "Topper Rotation"}
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
            {"customWheelImage", "Custom Wheel Image"},
            {"customPlayfieldImage", "Custom Playfield Image"},
            {"customBackglassImage", "Custom Backglass Image"},
            {"customDmdImage", "Custom DMD Image"},
            {"customTopperImage", "Custom Topper Image"},
            {"customPlayfieldVideo", "Custom Playfield Video"},
            {"customBackglassVideo", "Custom Backglass Video"},
            {"customDmdVideo", "Custom DMD Video"},
            {"customTopperVideo", "Custom Topper Video"},
            {"tableMusic", "Table Music"},
            {"customLaunchSound", "Custom Launch Sound"}
        };

        keyOrders_["VPX"] = {
            "VPXTablesPath",
            "VPinballXPath",
            "vpxIniPath",
            "vpxStartArgs",
            "vpxEndArgs"
        };
        keyDisplayNames_["VPX"] = {
            {"VPXTablesPath", "VPX Tables Path"},
            {"VPinballXPath", "VPinballX Path"},
            {"vpxIniPath", "VPX INI Path"},
            {"vpxStartArgs", "VPX Start Arguments"},
            {"vpxEndArgs", "VPX End Arguments"}
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
            {"defaultWheelImage", "Default Wheel Image"},
            {"defaultPlayfieldImage", "Default Playfield Image"},
            {"defaultBackglassImage", "Default Backglass Image"},
            {"defaultDmdImage", "Default DMD Image"},
            {"defaultTopperImage", "Default Topper Image"},
            {"defaultPlayfieldVideo", "Default Playfield Video"},
            {"defaultBackglassVideo", "Default Backglass Video"},
            {"defaultDmdVideo", "Default DMD Video"},
            {"defaultTopperVideo", "Default Topper Video"}
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
            {"scrollNormalSound", "Single Scroll Sound"},
            {"scrollFastSound", "Fast Scroll Sound"},
            {"scrollJumpSound", "Jump Scroll Sound"},
            {"scrollRandomSound", "Random Scroll Sound"},
            {"launchTableSound", "Launch Table Sound"},
            {"launchScreenshotSound", "Launch Screenshot Sound"},
            {"panelToggleSound", "Panel Toggle Sound"},
            {"screenshotTakeSound", "Screenshot Taken Sound"},
            {"ambienceSound", "Ambience Sound"}
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