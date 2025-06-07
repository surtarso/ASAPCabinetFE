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
        keyOrders_["TableMetadata"] = {
            "titleSource",
            "titleSortBy",
            "showMetadata",
            "fetchVPSdb",
            "forceRebuildMetadata",
            "metadataPanelWidth",
            "metadataPanelHeight",
            "metadataPanelAlpha"
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
        keyOrders_["Keybinds"] = {
            // TODO
        };
        keyOrders_["DPISettings"] = {
            "dpiScale",
            "enableDpiScaling"
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
        keyOrders_["MediaDimensions"] = {
            "forceImagesOnly",
            "wheelMediaHeight",
            "wheelMediaWidth",
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
        keyOrders_["VPX"] = {
            "VPXTablesPath",
            "VPinballXPath",
            "vpxIniPath",
            "vpxStartArgs",
            "vpxEndArgs"
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
        keyOrders_["UISounds"] = {
            // doesn't matter right now
        };
        keyOrders_["Internal"] = {
            // doesn't matter right now
        };
    }

    const std::vector<std::string>& getSectionOrder() const { return sectionOrder_; }
    const std::vector<std::string>& getKeyOrder(const std::string& sectionName) const {
        static const std::vector<std::string> empty;
        auto it = keyOrders_.find(sectionName);
        return it != keyOrders_.end() ? it->second : empty;
    }

private:
    std::vector<std::string> sectionOrder_;
    std::unordered_map<std::string, std::vector<std::string>> keyOrders_;
};

#endif // SECTION_CONFIG_H