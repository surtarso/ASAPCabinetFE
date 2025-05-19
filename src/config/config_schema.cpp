#include "config_schema.h"
#include "utils/logging.h"
#include <sstream>

ConfigSchema::ConfigSchema() {
    variables_ = {
        // VPX settings
        {"VPXTablesPath", "VPX", "VPXTablesPath", std::string("/home/$USER/VPX_Tables/"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "VPXTablesPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "VPXTablesPath"); }},
        {"VPinballXPath", "VPX", "VPinballXPath", std::string("/home/$USER/VPinballX_GL"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "VPinballXPath"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "VPinballXPath"); }},
        {"vpxSubCmd", "Internal", "SubCmd", std::string("-Play"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxSubCmd"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxSubCmd"); }},
        {"vpxStartArgs", "VPX", "StartArgs", std::string(""), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxStartArgs"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxStartArgs"); }},
        {"vpxEndArgs", "VPX", "EndArgs", std::string(""), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "vpxEndArgs"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "vpxEndArgs"); }},

        // DPI settings
        {"dpiScale", "DPISettings", "DpiScale", 1.0f, Type::Float, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseFloat(s, val, "dpiScale"); },
         [this](Settings& s, const auto& val) { defaultFloat(s, val, "dpiScale"); }},
        {"enableDpiScaling", "DPISettings", "EnableDpiScaling", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "enableDpiScaling"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "enableDpiScaling"); }},

        // Default media paths
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
        {"defaultPlayfieldVideo", "DefaultMedia", "DefaultPlayfieldVideo", std::string("img/default_table.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPlayfieldVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPlayfieldVideo"); }},
        {"defaultBackglassVideo", "DefaultMedia", "DefaultBackglassVideo", std::string("img/default_backglass.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultBackglassVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultBackglassVideo"); }},
        {"defaultDmdVideo", "DefaultMedia", "DefaultDmdVideo", std::string("img/default_dmd.mp4"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultDmdVideo"); }},
        {"defaultPuPPlayfieldImage", "DefaultMedia", "DefaultPuPPlayfieldImage", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPPlayfieldImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPPlayfieldImage"); }},
        {"defaultPuPBackglassImage", "DefaultMedia", "DefaultPuPBackglassImage", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPBackglassImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPBackglassImage"); }},
        {"defaultPuPDmdImage", "DefaultMedia", "DefaultPuPDmdImage", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPDmdImage"); }},
        {"defaultPuPFullDmdImage", "DefaultMedia", "DefaultPuPFullDmdImage", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPFullDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPFullDmdImage"); }},
        {"defaultPupTopperImage", "DefaultMedia", "DefaultPupTopperImage", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPupTopperImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPupTopperImage"); }},
        {"defaultPuPPlayfieldVideo", "DefaultMedia", "DefaultPuPPlayfieldVideo", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPPlayfieldVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPPlayfieldVideo"); }},
        {"defaultPuPBackglassVideo", "DefaultMedia", "DefaultPuPBackglassVideo", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPBackglassVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPBackglassVideo"); }},
        {"defaultPuPDmdVideo", "DefaultMedia", "DefaultPuPDmdVideo", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPDmdVideo"); }},
        {"defaultPuPFullDmdVideo", "DefaultMedia", "DefaultPuPFullDmdVideo", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPFullDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPFullDmdVideo"); }},
        {"defaultPuPTopperVideo", "DefaultMedia", "DefaultPuPTopperVideo", std::string("NYI"), Type::String, true, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "defaultPuPTopperVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "defaultPuPTopperVideo"); }},

        // Custom media paths
        {"customPlayfieldImage", "CustomMedia", "PlayfieldImage", std::string("images/table.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customPlayfieldImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customPlayfieldImage"); }},
        {"customBackglassImage", "CustomMedia", "BackglassImage", std::string("images/backglass.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customBackglassImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customBackglassImage"); }},
        {"customDmdImage", "CustomMedia", "DmdImage", std::string("images/marquee.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customDmdImage"); }},
        {"customWheelImage", "CustomMedia", "WheelImage", std::string("images/wheel.png"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customWheelImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customWheelImage"); }},
        {"customPlayfieldVideo", "CustomMedia", "PlayfieldVideo", std::string("video/table.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customPlayfieldVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customPlayfieldVideo"); }},
        {"customBackglassVideo", "CustomMedia", "BackglassVideo", std::string("video/backglass.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customBackglassVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customBackglassVideo"); }},
        {"customDmdVideo", "CustomMedia", "DmdVideo", std::string("video/dmd.mp4"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "customDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "customDmdVideo"); }},
        {"puPTopperImage", "CustomMedia", "PuPTopperImage", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPTopperImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPTopperImage"); }},
        {"puPPlayfieldImage", "CustomMedia", "PuPPlayfieldImage", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPPlayfieldImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPPlayfieldImage"); }},
        {"puPBackglassImage", "CustomMedia", "PuPBackglassImage", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPBackglassImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPBackglassImage"); }},
        {"puPDmdImage", "CustomMedia", "PuPDmdImage", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPDmdImage"); }},
        {"puPFullDmdImage", "CustomMedia", "PuPFullDmdImage", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPFullDmdImage"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPFullDmdImage"); }},
        {"puPTopperVideo", "CustomMedia", "PuPTopperVideo", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPTopperVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPTopperVideo"); }},
        {"puPPlayfieldVideo", "CustomMedia", "PuPPlayfieldVideo", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPPlayfieldVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPPlayfieldVideo"); }},
        {"puPBackglassVideo", "CustomMedia", "PuPBackglassVideo", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPBackglassVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPBackglassVideo"); }},
        {"puPDmdVideo", "CustomMedia", "PuPDmdVideo", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPDmdVideo"); }},
        {"puPFullDmdVideo", "CustomMedia", "PuPFullDmdVideo", std::string("NYI"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "puPFullDmdVideo"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "puPFullDmdVideo"); }},

        // Window settings
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

        // Media sizes/positions
        {"wheelMediaHeight", "MediaDimensions", "WheelMediaHeight", 350, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaHeight"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaHeight"); }},
        {"wheelMediaWidth", "MediaDimensions", "WheelMediaWidth", 350, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaWidth"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaWidth"); }},
        {"wheelMediaX", "MediaDimensions", "WheelMediaX", 720, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaX"); }},
        {"wheelMediaY", "MediaDimensions", "WheelMediaY", 1570, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "wheelMediaY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "wheelMediaY"); }},
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

        // Title display
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
        {"titleSource", "TitleDisplay", "TitleSource", std::string("filename"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "titleSource"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "titleSource"); }},
        {"showWheel", "TitleDisplay", "ShowWheel", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showWheel"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showWheel"); }},
        {"showTitle", "TitleDisplay", "ShowTitle", true, Type::Bool, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseBool(s, val, "showTitle"); },
         [this](Settings& s, const auto& val) { defaultBool(s, val, "showTitle"); }},
        {"titleX", "TitleDisplay", "TitleX", 30, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "titleX"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "titleX"); }},
        {"titleY", "TitleDisplay", "TitleY", 1850, Type::Int, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseInt(s, val, "titleY"); },
         [this](Settings& s, const auto& val) { defaultInt(s, val, "titleY"); }},

        // Sound settings
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

        // Logging
        {"logFile", "Internal", "LogFile", std::string("logs/debug.log"), Type::String, false, PostProcess::None,
         [this](Settings& s, const std::string& val) { parseString(s, val, "logFile"); },
         [this](Settings& s, const auto& val) { defaultString(s, val, "logFile"); }}
    };
}

void ConfigSchema::parseString(Settings& s, const std::string& val, const std::string& field) const {
    if (field == "VPXTablesPath") s.VPXTablesPath = val;
    else if (field == "VPinballXPath") s.VPinballXPath = val;
    else if (field == "vpxSubCmd") s.vpxSubCmd = val;
    else if (field == "vpxStartArgs") s.vpxStartArgs = val;
    else if (field == "vpxEndArgs") s.vpxEndArgs = val;
    else if (field == "defaultPlayfieldImage") s.defaultPlayfieldImage = val;
    else if (field == "defaultBackglassImage") s.defaultBackglassImage = val;
    else if (field == "defaultDmdImage") s.defaultDmdImage = val;
    else if (field == "defaultWheelImage") s.defaultWheelImage = val;
    else if (field == "defaultPlayfieldVideo") s.defaultPlayfieldVideo = val;
    else if (field == "defaultBackglassVideo") s.defaultBackglassVideo = val;
    else if (field == "defaultDmdVideo") s.defaultDmdVideo = val;
    else if (field == "defaultPuPPlayfieldImage") s.defaultPuPPlayfieldImage = val;
    else if (field == "defaultPuPBackglassImage") s.defaultPuPBackglassImage = val;
    else if (field == "defaultPuPDmdImage") s.defaultPuPDmdImage = val;
    else if (field == "defaultPuPFullDmdImage") s.defaultPuPFullDmdImage = val;
    else if (field == "defaultPupTopperImage") s.defaultPupTopperImage = val;
    else if (field == "defaultPuPPlayfieldVideo") s.defaultPuPPlayfieldVideo = val;
    else if (field == "defaultPuPBackglassVideo") s.defaultPuPBackglassVideo = val;
    else if (field == "defaultPuPDmdVideo") s.defaultPuPDmdVideo = val;
    else if (field == "defaultPuPFullDmdVideo") s.defaultPuPFullDmdVideo = val;
    else if (field == "defaultPuPTopperVideo") s.defaultPuPTopperVideo = val;
    else if (field == "customPlayfieldImage") s.customPlayfieldImage = val;
    else if (field == "customBackglassImage") s.customBackglassImage = val;
    else if (field == "customDmdImage") s.customDmdImage = val;
    else if (field == "customWheelImage") s.customWheelImage = val;
    else if (field == "customPlayfieldVideo") s.customPlayfieldVideo = val;
    else if (field == "customBackglassVideo") s.customBackglassVideo = val;
    else if (field == "customDmdVideo") s.customDmdVideo = val;
    else if (field == "puPTopperImage") s.puPTopperImage = val;
    else if (field == "puPPlayfieldImage") s.puPPlayfieldImage = val;
    else if (field == "puPBackglassImage") s.puPBackglassImage = val;
    else if (field == "puPDmdImage") s.puPDmdImage = val;
    else if (field == "puPFullDmdImage") s.puPFullDmdImage = val;
    else if (field == "puPTopperVideo") s.puPTopperVideo = val;
    else if (field == "puPPlayfieldVideo") s.puPPlayfieldVideo = val;
    else if (field == "puPBackglassVideo") s.puPBackglassVideo = val;
    else if (field == "puPDmdVideo") s.puPDmdVideo = val;
    else if (field == "puPFullDmdVideo") s.puPFullDmdVideo = val;
    else if (field == "fontPath") s.fontPath = val;
    else if (field == "titleSource") s.titleSource = val;
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
    else if (field == "logFile") s.logFile = val;
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
    else if (field == "fontSize") s.fontSize = v;
    else if (field == "titleX") s.titleX = v;
    else if (field == "titleY") s.titleY = v;
    else LOG_ERROR("ConfigSchema: Unknown int field: " << field);
}

void ConfigSchema::defaultInt(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    parseInt(s, std::to_string(std::get<int>(val)), field);
}

void ConfigSchema::parseFloat(Settings& s, const std::string& val, const std::string& field) const {
    float v = std::stof(val);
    if (field == "dpiScale") s.dpiScale = v;
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
    else LOG_ERROR("ConfigSchema: Unknown SDL_Color field: " << field);
}

void ConfigSchema::defaultSDLColor(Settings& s, const std::variant<std::string, int, float, bool, SDL_Color>& val, const std::string& field) const {
    const auto& color = std::get<SDL_Color>(val);
    std::ostringstream oss;
    oss << static_cast<int>(color.r) << "," << static_cast<int>(color.g) << ","
        << static_cast<int>(color.b) << "," << static_cast<int>(color.a);
    parseSDLColor(s, oss.str(), field);
}