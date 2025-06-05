#include "config_schema.h"
#include "utils/logging.h"
#include <sstream>

ConfigSchema::ConfigSchema() {
    variables_ = {
        // [VPX]
        {"VPXTablesPath", "VPX", "VPXTablesPath", std::string("/home/$USER/VPX_Tables/"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "VPXTablesPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "VPXTablesPath"); }},
        {"VPinballXPath", "VPX", "VPinballXPath", std::string("/home/$USER/VPinballX_BGFX"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "VPinballXPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "VPinballXPath"); }},
         {"vpxIniPath", "VPX", "VPXIniPath", std::string(""), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxIniPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxIniPath"); }},
         {"vpxSubCmd", "Internal", "SubCmd", std::string("-Play"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxSubCmd"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxSubCmd"); }},
        {"vpxStartArgs", "VPX", "StartArgs", std::string(""), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxStartArgs"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxStartArgs"); }},
        {"vpxEndArgs", "VPX", "EndArgs", std::string(""), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxEndArgs"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxEndArgs"); }},

        // [DpiScale]
        {"dpiScale", "DPISettings", "DpiScale", 1.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "dpiScale"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "dpiScale"); }},
        {"enableDpiScaling", "DPISettings", "EnableDpiScaling", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "enableDpiScaling"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "enableDpiScaling"); }},

        // [DefaultMedia]
        {"defaultPlayfieldImage", "DefaultMedia", "DefaultPlayfieldImage", std::string("img/default_table.png"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPlayfieldImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPlayfieldImage"); }},
        {"defaultBackglassImage", "DefaultMedia", "DefaultBackglassImage", std::string("img/default_backglass.png"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultBackglassImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultBackglassImage"); }},
        {"defaultDmdImage", "DefaultMedia", "DefaultDmdImage", std::string("img/default_dmd.png"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultDmdImage"); }},
        {"defaultWheelImage", "DefaultMedia", "DefaultWheelImage", std::string("img/default_wheel.png"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultWheelImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultWheelImage"); }},
        {"defaultTopperImage", "DefaultMedia", "DefaultTopperImage", std::string("img/default_topper.png"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultTopperImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultTopperImage"); }},
        {"defaultPlayfieldVideo", "DefaultMedia", "DefaultPlayfieldVideo", std::string("img/default_table.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPlayfieldVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPlayfieldVideo"); }},
        {"defaultBackglassVideo", "DefaultMedia", "DefaultBackglassVideo", std::string("img/default_backglass.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultBackglassVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultBackglassVideo"); }},
        {"defaultDmdVideo", "DefaultMedia", "DefaultDmdVideo", std::string("img/default_dmd.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultDmdVideo"); }},
        {"defaultTopperVideo", "DefaultMedia", "DefaultTopperVideo", std::string("img/default_topper.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultTopperVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultTopperVideo"); }},
        
        // [CustomMedia]
        {"customPlayfieldImage", "CustomMedia", "PlayfieldImage", std::string("images/table.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customPlayfieldImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customPlayfieldImage"); }},
        {"customBackglassImage", "CustomMedia", "BackglassImage", std::string("images/backglass.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customBackglassImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customBackglassImage"); }},
        {"customDmdImage", "CustomMedia", "DmdImage", std::string("images/dmd.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customDmdImage"); }},
        {"customWheelImage", "CustomMedia", "WheelImage", std::string("images/wheel.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customWheelImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customWheelImage"); }},
        {"customTopperImage", "CustomMedia", "TopperImage", std::string("images/topper.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customTopperImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customTopperImage"); }},
        {"customPlayfieldVideo", "CustomMedia", "PlayfieldVideo", std::string("video/table.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customPlayfieldVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customPlayfieldVideo"); }},
        {"customBackglassVideo", "CustomMedia", "BackglassVideo", std::string("video/backglass.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customBackglassVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customBackglassVideo"); }},
        {"customDmdVideo", "CustomMedia", "DmdVideo", std::string("video/dmd.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customDmdVideo"); }},
        {"customTopperVideo", "CustomMedia", "TopperVideo", std::string("images/topper.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customTopperVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customTopperVideo"); }},
        {"tableMusic", "CustomMedia", "TableMusic", std::string("audio/music.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "tableMusic"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "tableMusic"); }},
        {"customLaunchSound", "CustomMedia", "CustomLaunchSound", std::string("audio/launch.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customLaunchSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customLaunchSound"); }},

        // [WindowSettings]
        {"videoBackend", "WindowSettings", "VideoBackend", std::string("vlc"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "videoBackend"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "videoBackend"); }},
        {"useVPinballXIni", "WindowSettings", "UseVPinballXIni", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "useVPinballXIni"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "useVPinballXIni"); }},      
        {"playfieldWindowWidth", "WindowSettings", "PlayfieldWidth", 1080, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldWindowWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldWindowWidth"); }},
        {"playfieldWindowHeight", "WindowSettings", "PlayfieldHeight", 1920, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldWindowHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldWindowHeight"); }},
        {"playfieldX", "WindowSettings", "PlayfieldX", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldX"); }},
        {"playfieldY", "WindowSettings", "PlayfieldY", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldY"); }},
        {"showBackglass", "WindowSettings", "ShowBackglass", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showBackglass"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showBackglass"); }},
        {"backglassWindowWidth", "WindowSettings", "BackglassWidth", 1024, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassWindowWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassWindowWidth"); }},
        {"backglassWindowHeight", "WindowSettings", "BackglassHeight", 768, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassWindowHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassWindowHeight"); }},
        {"backglassX", "WindowSettings", "BackglassX", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassX"); }},
        {"backglassY", "WindowSettings", "BackglassY", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassY"); }},
        {"showDMD", "WindowSettings", "ShowDMD", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showDMD"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showDMD"); }},
        {"dmdWindowWidth", "WindowSettings", "DMDWidth", 1024, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdWindowWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdWindowWidth"); }},
        {"dmdWindowHeight", "WindowSettings", "DMDHeight", 256, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdWindowHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdWindowHeight"); }},
        {"dmdX", "WindowSettings", "DMDX", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdX"); }},
        {"dmdY", "WindowSettings", "DMDY", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdY"); }},
        {"showTopper", "WindowSettings", "ShowTopper", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showTopper"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showTopper"); }},
        {"topperWindowWidth", "WindowSettings", "TopperWidth", 512, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperWindowWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperWindowWidth"); }},
        {"topperWindowHeight", "WindowSettings", "TopperHeight", 128, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperWindowHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperWindowHeight"); }},
        {"topperWindowX", "WindowSettings", "TopperX", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperWindowX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperWindowX"); }},
        {"topperWindowY", "WindowSettings", "TopperY", -1, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperWindowY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperWindowY"); }},

        // [MediaDimensions]
        {"forceImagesOnly", "MediaDimensions", "ForceImagesOnly", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "forceImagesOnly"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "forceImagesOnly"); }},
        {"wheelMediaHeight", "MediaDimensions", "WheelMediaHeight", 350, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaHeight"); }},
        {"wheelMediaWidth", "MediaDimensions", "WheelMediaWidth", 350, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaWidth"); }},
        {"wheelMediaX", "MediaDimensions", "WheelMediaX", 720, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaX"); }},
        {"wheelMediaY", "MediaDimensions", "WheelMediaY", 1550, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaY"); }},
        // Playfield media
        {"playfieldMediaWidth", "MediaDimensions", "PlayfieldMediaWidth", 1080, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldMediaWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldMediaWidth"); }},
        {"playfieldMediaHeight", "MediaDimensions", "PlayfieldMediaHeight", 1920, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldMediaHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldMediaHeight"); }},
        {"playfieldMediaX", "MediaDimensions", "PlayfieldMediaX", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldMediaX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldMediaX"); }},
        {"playfieldMediaY", "MediaDimensions", "PlayfieldMediaY", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldMediaY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldMediaY"); }},
        {"playfieldRotation", "MediaDimensions", "PlayfieldRotation", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "playfieldRotation"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "playfieldRotation"); }},
        // Backglass media
        {"backglassMediaWidth", "MediaDimensions", "BackglassMediaWidth", 1024, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassMediaWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassMediaWidth"); }},
        {"backglassMediaHeight", "MediaDimensions", "BackglassMediaHeight", 768, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassMediaHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassMediaHeight"); }},
        {"backglassMediaX", "MediaDimensions", "BackglassMediaX", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassMediaX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassMediaX"); }},
        {"backglassMediaY", "MediaDimensions", "BackglassMediaY", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassMediaY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassMediaY"); }},
        {"backglassRotation", "MediaDimensions", "BackglassRotation", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "backglassRotation"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "backglassRotation"); }},
        // DMD media
        {"dmdMediaWidth", "MediaDimensions", "DMDMediaWidth", 1024, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdMediaWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdMediaWidth"); }},
        {"dmdMediaHeight", "MediaDimensions", "DMDMediaHeight", 256, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdMediaHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdMediaHeight"); }},
        {"dmdMediaX", "MediaDimensions", "DMDMediaX", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdMediaX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdMediaX"); }},
        {"dmdMediaY", "MediaDimensions", "DMDMediaY", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdMediaY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdMediaY"); }},
        {"dmdRotation", "MediaDimensions", "DMDRotation", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "dmdRotation"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "dmdRotation"); }},
        // Topper media
        {"topperMediaWidth", "MediaDimensions", "TopperMediaWidth", 512, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperMediaWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperMediaWidth"); }},
        {"topperMediaHeight", "MediaDimensions", "TopperMediaHeight", 128, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperMediaHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperMediaHeight"); }},
        {"topperMediaX", "MediaDimensions", "TopperMediaX", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperMediaX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperMediaX"); }},
        {"topperMediaY", "MediaDimensions", "TopperMediaY", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperMediaY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperMediaY"); }},
        {"topperRotation", "MediaDimensions", "TopperRotation", 0, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "topperRotation"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "topperRotation"); }},

        // [TableMetadata]
        {"titleSortBy", "TableMetadata", "TitleSortBy", std::string("title"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "titleSortBy"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "titleSortBy"); }},
        {"fetchVPSdb", "TableMetadata", "FetchVPSdb", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "fetchVPSdb"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "fetchVPSdb"); }},
        {"forceRebuildMetadata", "TableMetadata", "ForceRebuild", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "forceRebuildMetadata"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "forceRebuildMetadata"); }},
        {"showMetadata", "TableMetadata", "ShowMetadata", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showMetadata"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showMetadata"); }},
        {"titleSource", "TableMetadata", "TitleSource", std::string("filename"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "titleSource"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "titleSource"); }},
        



         {"metadataPanelWidth", "TableMetadata", "MetadataPanelWidth", 0.7f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "metadataPanelWidth"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "metadataPanelWidth"); }},
        {"metadataPanelHeight", "TableMetadata", "MetadataPanelHeight", 0.5f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "metadataPanelHeight"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "metadataPanelHeight"); }},
        {"metadataPanelAlpha", "TableMetadata", "MetadataPanelAlpha", 0.6f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "metadataPanelAlpha"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "metadataPanelAlpha"); }},
        
        // [UIWidgets]
        // Arrow widget
        {"showArrowHint", "UIWidgets", "ShowArrowHint", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showArrowHint"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showArrowHint"); }},
        {"arrowHintWidth", "UIWidgets", "ArrowHintWidth", 20.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "arrowHintWidth"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "arrowHintWidth"); }},
        {"arrowHintHeight", "UIWidgets", "ArrowHintHeight", 100.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "arrowHintHeight"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "arrowHintHeight"); }},
        {"arrowThickness", "UIWidgets", "ArrowThickness", 4.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "arrowThickness"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "arrowThickness"); }},
        {"arrowAlpha", "UIWidgets", "ArrowAlpha", 0.6f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "arrowAlpha"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "arrowAlpha"); }},
        {"arrowGlow", "UIWidgets", "ArrowGlow", 1.5f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "arrowGlow"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "arrowGlow"); }},
        {"arrowGlowColor", "UIWidgets", "ArrowGlowColor", SDL_Color{200, 200, 200, 255}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "arrowGlowColor"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "arrowGlowColor"); }},
        {"arrowColorTop", "UIWidgets", "ArrowColorTop", SDL_Color{100, 100, 100, 255}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "arrowColorTop"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "arrowColorTop"); }},
        {"arrowColorBottom", "UIWidgets", "ArrowColorBottom", SDL_Color{150, 150, 150, 255}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "arrowColorBottom"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "arrowColorBottom"); }},
        // Scrollbar widget
        {"showScrollbar", "UIWidgets", "ShowScrollbar", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showScrollbar"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showScrollbar"); }},
        {"scrollbarWidth", "UIWidgets", "ScrollbarWidth", 12.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "scrollbarWidth"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "scrollbarWidth"); }},
        {"scrollbarHeight", "UIWidgets", "ScrollbarHeight", 15.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "scrollbarHeight"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "scrollbarHeight"); }},
        {"scrollbarLength", "UIWidgets", "ScrollbarLength", 0.5f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "scrollbarLength"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "scrollbarLength"); }},
        {"scrollbarColor", "UIWidgets", "ScrollbarColor", SDL_Color{50, 50, 50, 200}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "scrollbarColor"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "scrollbarColor"); }},
        {"scrollbarThumbColor", "UIWidgets", "ScrollbarThumbColor", SDL_Color{50, 150, 150, 255}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "scrollbarThumbColor"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "scrollbarThumbColor"); }},

        // [TitleDisplay]
        {"fontPath", "TitleDisplay", "FontPath", std::string("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "fontPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "fontPath"); }},
        {"fontColor", "TitleDisplay", "FontColor", SDL_Color{255, 255, 255, 255}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "fontColor"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "fontColor"); }},
        {"fontBgColor", "TitleDisplay", "FontBgColor", SDL_Color{0, 0, 0, 128}, Type::SDLColor, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseSDLColor(s, val, "fontBgColor"); },
         [this](Settings& s, const auto& val) { defaultSDLColor(s, val, "fontBgColor"); }},
        {"fontSize", "TitleDisplay", "FontSize", 28, Type::Int, false, PostProcess::DpiScaleFontSize,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "fontSize"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "fontSize"); }},
        {"showWheel", "TitleDisplay", "ShowWheel", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showWheel"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showWheel"); }},
        {"wheelWindow", "TitleDisplay", "WheelWindow", std::string("playfield"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "wheelWindow"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "wheelWindow"); }},
        {"showTitle", "TitleDisplay", "ShowTitle", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showTitle"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showTitle"); }},
        {"titleWindow", "TitleDisplay", "TitleWindow", std::string("playfield"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "titleWindow"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "titleWindow"); }},
        {"titleX", "TitleDisplay", "TitleX", 30, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "titleX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "titleX"); }},
        {"titleY", "TitleDisplay", "TitleY", 1850, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "titleY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "titleY"); }},
        
        // [AudioSettings]
        {"masterVol", "AudioSettings", "MasterVol", 100.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "masterVol"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "masterVol"); }},
        {"mediaAudioVol", "AudioSettings", "MediaAudioVol", 60.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "mediaAudioVol"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "mediaAudioVol"); }},
        {"tableMusicVol", "AudioSettings", "TableMusicVol", 60.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "tableMusicVol"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "tableMusicVol"); }},
        {"interfaceAudioVol", "AudioSettings", "InterfaceAudioVol", 60.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "interfaceAudioVol"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "interfaceAudioVol"); }},
        {"interfaceAmbienceVol", "AudioSettings", "InterfaceAmbienceVol", 60.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "interfaceAmbienceVol"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "interfaceAmbienceVol"); }},
        {"masterMute", "AudioSettings", "MasterMute", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "masterMute"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "masterMute"); }},
        {"mediaAudioMute", "AudioSettings", "MediaAudioMute", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "mediaAudioMute"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "mediaAudioMute"); }},
        {"tableMusicMute", "AudioSettings", "TableMusicMute", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "tableMusicMute"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "tableMusicMute"); }},
        {"interfaceAudioMute", "AudioSettings", "InterfaceAudioMute", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "interfaceAudioMute"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "interfaceAudioMute"); }},
        {"interfaceAmbienceMute", "AudioSettings", "InterfaceAmbienceMute", false, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "interfaceAmbienceMute"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "interfaceAmbienceMute"); }},


        // [UISounds]
        {"scrollPrevSound", "UISounds", "ScrollPrevSound", std::string("snd/scroll_prev.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollPrevSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollPrevSound"); }},
        {"scrollNextSound", "UISounds", "ScrollNextSound", std::string("snd/scroll_next.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollNextSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollNextSound"); }},
        {"scrollFastPrevSound", "UISounds", "ScrollFastPrevSound", std::string("snd/scroll_fast_prev.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollFastPrevSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollFastPrevSound"); }},
        {"scrollFastNextSound", "UISounds", "ScrollFastNextSound", std::string("snd/scroll_fast_next.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollFastNextSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollFastNextSound"); }},
        {"scrollJumpPrevSound", "UISounds", "ScrollJumpPrevSound", std::string("snd/scroll_jump_prev.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollJumpPrevSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollJumpPrevSound"); }},
        {"scrollJumpNextSound", "UISounds", "ScrollJumpNextSound", std::string("snd/scroll_jump_next.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollJumpNextSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollJumpNextSound"); }},
        {"scrollRandomSound", "UISounds", "ScrollRandomSound", std::string("snd/scroll_random.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "scrollRandomSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "scrollRandomSound"); }},
        {"launchTableSound", "UISounds", "LaunchTableSound", std::string("snd/launch_table.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "launchTableSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "launchTableSound"); }},
        {"launchScreenshotSound", "UISounds", "LaunchScreenshotSound", std::string("snd/launch_screenshot.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "launchScreenshotSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "launchScreenshotSound"); }},
        {"configToggleSound", "UISounds", "ConfigToggleSound", std::string("snd/config_toggle.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "configToggleSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "configToggleSound"); }},
        {"configSaveSound", "UISounds", "ConfigSaveSound", std::string("snd/config_save.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "configSaveSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "configSaveSound"); }},
        {"screenshotTakeSound", "UISounds", "ScreenshotTakeSound", std::string("snd/screenshot_take.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "screenshotTakeSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "screenshotTakeSound"); }},
        {"screenshotQuitSound", "UISounds", "ScreenshotQuitSound", std::string("snd/screenshot_quit.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "screenshotQuitSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "screenshotQuitSound"); }},
        {"ambienceSound", "UISounds", "AmbienceSound", std::string("snd/interface_ambience.mp3"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "ambienceSound"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "ambienceSound"); }},
        
         // [Internal]
        {"logFile", "Internal", "LogFile", std::string("logs/debug.log"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "logFile"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "logFile"); }},
        {"vpsDbPath", "Internal", "VpsDbPath", std::string("data/vpsdb.json"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpsDbPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpsDbPath"); }},
        {"vpsDbUpdateFrequency", "Internal", "VpsDbUpdateFrequency", std::string("startup"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpsDbUpdateFrequency"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpsDbUpdateFrequency"); }},
         {"vpsDbLastUpdated", "Internal", "VpsDbLastUpdated", std::string("data/vpsdb_last_updated.txt"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpsDbLastUpdated"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpsDbLastUpdated"); }},
         {"indexPath", "Internal", "IndexPath", std::string("data/asapcabinetfe_index.json"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "indexPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "indexPath"); }},
        {"vpxtoolIndex", "Internal", "VpxtoolIndex", std::string("data/vpxtool_index.json"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxtoolIndex"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxtoolIndex"); }},
        {"screenshotWait", "Internal", "ScreenshotWait", 4, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "screenshotWait"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "screenshotWait"); }}
    };
}

void ConfigSchema::parseString(Settings& s, const std::string& val, const std::string& field) const {
    if (field == "VPXTablesPath") s.VPXTablesPath = val;
    else if (field == "VPinballXPath") s.VPinballXPath = val;
    else if (field == "vpxIniPath") s.vpxIniPath = val;
    else if (field == "vpxSubCmd") s.vpxSubCmd = val;
    else if (field == "vpxStartArgs") s.vpxStartArgs = val;
    else if (field == "vpxEndArgs") s.vpxEndArgs = val;
    else if (field == "defaultPlayfieldImage") s.defaultPlayfieldImage = val;
    else if (field == "defaultBackglassImage") s.defaultBackglassImage = val;
    else if (field == "defaultDmdImage") s.defaultDmdImage = val;
    else if (field == "defaultWheelImage") s.defaultWheelImage = val;
    else if (field == "defaultTopperImage") s.defaultTopperImage = val;
    else if (field == "defaultPlayfieldVideo") s.defaultPlayfieldVideo = val;
    else if (field == "defaultBackglassVideo") s.defaultBackglassVideo = val;
    else if (field == "defaultDmdVideo") s.defaultDmdVideo = val;
    else if (field == "defaultTopperVideo") s.defaultTopperVideo = val;
    else if (field == "customPlayfieldImage") s.customPlayfieldImage = val;
    else if (field == "customBackglassImage") s.customBackglassImage = val;
    else if (field == "customDmdImage") s.customDmdImage = val;
    else if (field == "customTopperImage") s.customTopperImage = val;
    else if (field == "customWheelImage") s.customWheelImage = val;
    else if (field == "customPlayfieldVideo") s.customPlayfieldVideo = val;
    else if (field == "customBackglassVideo") s.customBackglassVideo = val;
    else if (field == "customDmdVideo") s.customDmdVideo = val;
    else if (field == "customTopperVideo") s.customTopperVideo = val;
    else if (field == "tableMusic") s.tableMusic = val;
    else if (field == "customLaunchSound") s.customLaunchSound = val;
    else if (field == "fontPath") s.fontPath = val;
    else if (field == "titleSource") s.titleSource = val;
    else if (field == "titleSortBy") s.titleSortBy = val;
    else if (field == "scrollPrevSound") s.scrollPrevSound = val;
    else if (field == "scrollNextSound") s.scrollNextSound = val;
    else if (field == "scrollFastPrevSound") s.scrollFastPrevSound = val;
    else if (field == "scrollFastNextSound") s.scrollFastNextSound = val;
    else if (field == "scrollJumpPrevSound") s.scrollJumpPrevSound = val;
    else if (field == "scrollJumpNextSound") s.scrollJumpNextSound = val;
    else if (field == "scrollRandomSound") s.scrollRandomSound = val;
    else if (field == "launchTableSound") s.launchTableSound = val;
    else if (field == "launchScreenshotSound") s.launchScreenshotSound = val;
    else if (field == "configToggleSound") s.configToggleSound = val;
    else if (field == "configSaveSound") s.configSaveSound = val;
    else if (field == "screenshotTakeSound") s.screenshotTakeSound = val;
    else if (field == "screenshotQuitSound") s.screenshotQuitSound = val;
    else if (field == "ambienceSound") s.ambienceSound = val;
    else if (field == "logFile") s.logFile = val;
    else if (field == "videoBackend") s.videoBackend = val;
    else if (field == "vpsDbPath") s.vpsDbPath = val;
    else if (field == "vpsDbUpdateFrequency") s.vpsDbUpdateFrequency = val;
    else if (field == "vpsDbLastUpdated") s.vpsDbLastUpdated = val;
    else if (field == "indexPath") s.indexPath = val;
    else if (field == "vpxtoolIndex") s.vpxtoolIndex = val;
    else if (field == "wheelWindow") s.wheelWindow = val;
    else if (field == "titleWindow") s.titleWindow = val;
    else LOG_ERROR("ConfigSchema: Unknown string field: " << field);
}

void ConfigSchema::defaultString(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    parseString(s, std::get<std::string>(val), field);
}
   
void ConfigSchema::parseInt(Settings& s, const std::string& val, const std::string& field) const {
    int v = std::stoi(val);
    if (field == "playfieldWindowWidth") s.playfieldWindowWidth = v;
    else if (field == "playfieldWindowHeight") s.playfieldWindowHeight = v;
    else if (field == "playfieldX") s.playfieldX = v;
    else if (field == "playfieldY") s.playfieldY = v;
    else if (field == "backglassWindowWidth") s.backglassWindowWidth = v;
    else if (field == "backglassWindowHeight") s.backglassWindowHeight = v;
    else if (field == "backglassX") s.backglassX = v;
    else if (field == "backglassY") s.backglassY = v;
    else if (field == "dmdWindowWidth") s.dmdWindowWidth = v;
    else if (field == "dmdWindowHeight") s.dmdWindowHeight = v;
    else if (field == "dmdX") s.dmdX = v;
    else if (field == "dmdY") s.dmdY = v;
    else if (field == "topperWindowWidth") s.topperWindowWidth = v;
    else if (field == "topperWindowHeight") s.topperWindowHeight = v;
    else if (field == "topperWindowX") s.topperWindowX = v;
    else if (field == "topperWindowY") s.topperWindowY = v;
    else if (field == "wheelMediaHeight") s.wheelMediaHeight = v;
    else if (field == "wheelMediaWidth") s.wheelMediaWidth = v;
    else if (field == "wheelMediaX") s.wheelMediaX = v;
    else if (field == "wheelMediaY") s.wheelMediaY = v;
    else if (field == "playfieldMediaWidth") s.playfieldMediaWidth = v;
    else if (field == "playfieldMediaHeight") s.playfieldMediaHeight = v;
    else if (field == "playfieldMediaX") s.playfieldMediaX = v;
    else if (field == "playfieldMediaY") s.playfieldMediaY = v;
    else if (field == "playfieldRotation") s.playfieldRotation = v;
    else if (field == "backglassMediaWidth") s.backglassMediaWidth = v;
    else if (field == "backglassMediaHeight") s.backglassMediaHeight = v;
    else if (field == "backglassMediaX") s.backglassMediaX = v;
    else if (field == "backglassMediaY") s.backglassMediaY = v;
    else if (field == "backglassRotation") s.backglassRotation = v;
    else if (field == "dmdMediaWidth") s.dmdMediaWidth = v;
    else if (field == "dmdMediaHeight") s.dmdMediaHeight = v;
    else if (field == "dmdMediaX") s.dmdMediaX = v;
    else if (field == "dmdMediaY") s.dmdMediaY = v;
    else if (field == "dmdRotation") s.dmdRotation = v;
    else if (field == "topperMediaWidth") s.topperMediaWidth = v;
    else if (field == "topperMediaHeight") s.topperMediaHeight = v;
    else if (field == "topperMediaX") s.topperMediaX = v;
    else if (field == "topperMediaY") s.topperMediaY = v;
    else if (field == "topperRotation") s.topperRotation = v;
    else if (field == "fontSize") s.fontSize = v;
    else if (field == "titleX") s.titleX = v;
    else if (field == "titleY") s.titleY = v;
    else if (field == "screenshotWait") s.screenshotWait= v;
    else LOG_ERROR("ConfigSchema: Unknown int field: " << field);
}

void ConfigSchema::defaultInt(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    parseInt(s, std::to_string(std::get<int>(val)), field);
}

void ConfigSchema::parseFloat(Settings& s, const std::string& val, const std::string& field) const {
    float v = std::stof(val);
    if (field == "dpiScale") s.dpiScale = v;
    else if (field == "masterVol") s.masterVol = v;
    else if (field == "mediaAudioVol") s.mediaAudioVol = v;
    else if (field == "tableMusicVol") s.tableMusicVol = v;
    else if (field == "interfaceAudioVol") s.interfaceAudioVol = v;
    else if (field == "interfaceAmbienceVol") s.interfaceAmbienceVol = v;
    else if (field == "metadataPanelWidth") s.metadataPanelWidth = v;
    else if (field == "metadataPanelHeight") s.metadataPanelHeight = v;
    else if (field == "metadataPanelAlpha") s.metadataPanelAlpha = v;
    else if (field == "arrowHintWidth") s.arrowHintWidth = v;
    else if (field == "arrowHintHeight") s.arrowHintHeight = v;
    else if (field == "arrowThickness") s.arrowThickness = v;
    else if (field == "arrowAlpha") s.arrowAlpha = v;
    else if (field == "arrowGlow") s.arrowGlow = v;
    else if (field == "scrollbarWidth") s.scrollbarWidth = v;
    else if (field == "scrollbarHeight") s.scrollbarHeight = v;
    else if (field == "scrollbarLength") s.scrollbarLength = v;
    else LOG_ERROR("ConfigSchema: Unknown float field: " << field);
}

void ConfigSchema::defaultFloat(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    parseFloat(s, std::to_string(std::get<float>(val)), field);
}

void ConfigSchema::parseBool(Settings& s, const std::string& val, const std::string& field) const {
    bool v = (val == "true");
    if (field == "enableDpiScaling") s.enableDpiScaling = v;
    else if (field == "showBackglass") s.showBackglass = v;
    else if (field == "showDMD") s.showDMD = v;
    else if (field == "showWheel") s.showWheel = v;
    else if (field == "showTitle") s.showTitle = v;
    else if (field == "showTopper") s.showTopper = v;
    else if (field == "useVPinballXIni") s.useVPinballXIni = v;
    else if (field == "forceImagesOnly") s.forceImagesOnly = v;
    else if (field == "showMetadata") s.showMetadata = v;
    else if (field == "fetchVPSdb") s.fetchVPSdb = v;
    else if (field == "forceRebuildMetadata") s.forceRebuildMetadata = v;
    else if (field == "masterMute") s.masterMute = v;
    else if (field == "mediaAudioMute") s.mediaAudioMute = v;
    else if (field == "tableMusicMute") s.tableMusicMute = v;
    else if (field == "interfaceAudioMute") s.interfaceAudioMute = v;
    else if (field == "interfaceAmbienceMute") s.interfaceAmbienceMute = v;
    else if (field == "showArrowHint") s.showArrowHint = v;
    else if (field == "showScrollbar") s.showScrollbar = v;
    else LOG_ERROR("ConfigSchema: Unknown bool field: " << field);
}

void ConfigSchema::defaultBool(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    parseBool(s, std::get<bool>(val) ? "true" : "false", field);
}

void ConfigSchema::parseSDLColor(Settings& s, const std::string& val, const std::string& field) const {
    SDL_Color color;
    sscanf(val.c_str(), "%hhu,%hhu,%hhu,%hhu", &color.r, &color.g, &color.b, &color.a);
    if (field == "fontColor") s.fontColor = color;
    else if (field == "fontBgColor") s.fontBgColor = color;
    else if (field == "arrowGlowColor") s.arrowGlowColor = color;
    else if (field == "arrowColorTop") s.arrowColorTop = color;
    else if (field == "arrowColorBottom") s.arrowColorBottom = color;
    else if (field == "scrollbarColor") s.scrollbarColor = color;
    else if (field == "scrollbarThumbColor") s.scrollbarThumbColor = color;
    else LOG_ERROR("ConfigSchema: Unknown SDL_Color field: " << field);
}

void ConfigSchema::defaultSDLColor(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    const auto& color = std::get<SDL_Color>(val);
    std::ostringstream oss;
    oss << static_cast<int>(color.r) << "," << static_cast<int>(color.g) << ","
        << static_cast<int>(color.b) << "," << static_cast<int>(color.a);
    parseSDLColor(s, oss.str(), field);
}