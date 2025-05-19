#include "settings_parser.h"
#include "utils/logging.h"
#include <sstream>
#include <algorithm>

SettingsParser::SettingsParser(const std::string& configPath) : configPath_(configPath) {}

void SettingsParser::parse(const std::map<std::string, SettingsSection>& iniData, Settings& settings, KeybindManager& keybindManager) const {
    std::map<std::string, std::map<std::string, std::string>> config;
    for (const auto& [section, configSection] : iniData) {
        for (const auto& kv : configSection.keyValues) {
            config[section][kv.first] = kv.second;
        }
    }

    setDefaultSettings(settings);
    std::string exeDir = configPath_.substr(0, configPath_.find_last_of('/') + 1);
    auto resolvePath = [&](const std::string& value, const std::string& defaultPath) {
        if (value.empty()) return exeDir + defaultPath;
        if (value.find('/') == 0 || value.find('\\') == 0) return value;
        return exeDir + value;
    };

    settings.VPXTablesPath = config["VPX"]["VPXTablesPath"].empty() ? settings.VPXTablesPath : config["VPX"]["VPXTablesPath"];
    settings.VPinballXPath = config["VPX"]["VPinballXPath"].empty() ? settings.VPinballXPath : config["VPX"]["VPinballXPath"];
    settings.vpxSubCmd = config["Internal"]["SubCmd"].empty() ? settings.vpxSubCmd : config["Internal"]["SubCmd"];
    settings.vpxStartArgs = config["VPX"]["StartArgs"];
    settings.vpxEndArgs = config["VPX"]["EndArgs"];
    
    settings.defaultPlayfieldImage = resolvePath(config["DefaultMedia"]["DefaultPlayfieldImage"], "img/default_table.png");
    settings.defaultBackglassImage = resolvePath(config["DefaultMedia"]["DefaultBackglassImage"], "img/default_backglass.png");
    settings.defaultDmdImage = resolvePath(config["DefaultMedia"]["DefaultDmdImage"], "img/default_dmd.png");
    settings.defaultWheelImage = resolvePath(config["DefaultMedia"]["DefaultWheelImage"], "img/default_wheel.png");
    settings.defaultPlayfieldVideo = resolvePath(config["DefaultMedia"]["DefaultPlayfieldVideo"], "img/default_table.mp4");
    settings.defaultBackglassVideo = resolvePath(config["DefaultMedia"]["DefaultBackglassVideo"], "img/default_backglass.mp4");
    settings.defaultDmdVideo = resolvePath(config["DefaultMedia"]["DefaultDmdVideo"], "img/default_dmd.mp4");
    
    settings.customPlayfieldImage = config["CustomMedia"]["PlayfieldImage"].empty() ? settings.customPlayfieldImage : config["CustomMedia"]["PlayfieldImage"];
    settings.customBackglassImage = config["CustomMedia"]["BackglassImage"].empty() ? settings.customBackglassImage : config["CustomMedia"]["BackglassImage"];
    settings.customDmdImage = config["CustomMedia"]["DmdImage"].empty() ? settings.customDmdImage : config["CustomMedia"]["DmdImage"];
    settings.customWheelImage = config["CustomMedia"]["WheelImage"].empty() ? settings.customWheelImage : config["CustomMedia"]["WheelImage"];
    settings.customPlayfieldVideo = config["CustomMedia"]["PlayfieldVideo"].empty() ? settings.customPlayfieldVideo : config["CustomMedia"]["PlayfieldVideo"];
    settings.customBackglassVideo = config["CustomMedia"]["BackglassVideo"].empty() ? settings.customBackglassVideo : config["CustomMedia"]["BackglassVideo"];
    settings.customDmdVideo = config["CustomMedia"]["DmdVideo"].empty() ? settings.customDmdVideo : config["CustomMedia"]["DmdVideo"];
    
    settings.enableDpiScaling = config["DPISettings"]["EnableDpiScaling"].empty() ? true : (config["DPISettings"]["EnableDpiScaling"] == "true");
    settings.dpiScale = std::stof(config["DPISettings"]["DpiScale"].empty() ? "1.0" : config["DPISettings"]["DpiScale"]);
    
    settings.playfieldWindowWidth = std::stoi(config["WindowSettings"]["PlayfieldWidth"].empty() ? "1080" : config["WindowSettings"]["PlayfieldWidth"]);
    settings.playfieldWindowHeight = std::stoi(config["WindowSettings"]["PlayfieldHeight"].empty() ? "1920" : config["WindowSettings"]["PlayfieldHeight"]);
    settings.playfieldX = std::stoi(config["WindowSettings"]["PlayfieldX"].empty() ? "-1" : config["WindowSettings"]["PlayfieldX"]);
    settings.playfieldY = std::stoi(config["WindowSettings"]["PlayfieldY"].empty() ? "-1" : config["WindowSettings"]["PlayfieldY"]);
    
    settings.showBackglass = config["WindowSettings"]["ShowBackglass"].empty() ? true : (config["WindowSettings"]["ShowBackglass"] == "true");
    settings.backglassWindowWidth = std::stoi(config["WindowSettings"]["BackglassWidth"].empty() ? "1024" : config["WindowSettings"]["BackglassWidth"]);
    settings.backglassWindowHeight = std::stoi(config["WindowSettings"]["BackglassHeight"].empty() ? "768" : config["WindowSettings"]["BackglassHeight"]);
    settings.backglassX = std::stoi(config["WindowSettings"]["BackglassX"].empty() ? "-1" : config["WindowSettings"]["BackglassX"]);
    settings.backglassY = std::stoi(config["WindowSettings"]["BackglassY"].empty() ? "-1" : config["WindowSettings"]["BackglassY"]);
    
    settings.showDMD = config["WindowSettings"]["ShowDMD"].empty() ? true : (config["WindowSettings"]["ShowDMD"] == "true");
    settings.dmdWindowWidth = std::stoi(config["WindowSettings"]["DMDWidth"].empty() ? "1024" : config["WindowSettings"]["DMDWidth"]);
    settings.dmdWindowHeight = std::stoi(config["WindowSettings"]["DMDHeight"].empty() ? "256" : config["WindowSettings"]["DMDHeight"]);
    settings.dmdX = std::stoi(config["WindowSettings"]["DMDX"].empty() ? "-1" : config["WindowSettings"]["DMDX"]);
    settings.dmdY = std::stoi(config["WindowSettings"]["DMDY"].empty() ? "-1" : config["WindowSettings"]["DMDY"]);
    
    settings.wheelMediaHeight = std::stoi(config["MediaDimensions"]["WheelMediaHeight"].empty() ? "350" : config["MediaDimensions"]["WheelMediaHeight"]);
    settings.wheelMediaWidth = std::stoi(config["MediaDimensions"]["WheelMediaWidth"].empty() ? "350" : config["MediaDimensions"]["WheelMediaWidth"]);
    settings.wheelMediaX = std::stoi(config["MediaDimensions"]["WheelMediaX"].empty() ? "720" : config["MediaDimensions"]["WheelMediaX"]);
    settings.wheelMediaY = std::stoi(config["MediaDimensions"]["WheelMediaY"].empty() ? "1570" : config["MediaDimensions"]["WheelMediaY"]);
    
    settings.playfieldMediaWidth = std::stoi(config["MediaDimensions"]["PlayfieldMediaWidth"].empty() ? "1080" : config["MediaDimensions"]["PlayfieldMediaWidth"]);
    settings.playfieldMediaHeight = std::stoi(config["MediaDimensions"]["PlayfieldMediaHeight"].empty() ? "1920" : config["MediaDimensions"]["PlayfieldMediaHeight"]);
    settings.playfieldMediaX = std::stoi(config["MediaDimensions"]["PlayfieldMediaX"].empty() ? "0" : config["MediaDimensions"]["PlayfieldMediaX"]);
    settings.playfieldMediaY = std::stoi(config["MediaDimensions"]["PlayfieldMediaY"].empty() ? "0" : config["MediaDimensions"]["PlayfieldMediaY"]);
    settings.playfieldRotation = std::stoi(config["MediaDimensions"]["PlayfieldRotation"].empty() ? "0" : config["MediaDimensions"]["PlayfieldRotation"]);
    
    settings.backglassMediaWidth = std::stoi(config["MediaDimensions"]["BackglassMediaWidth"].empty() ? "1024" : config["MediaDimensions"]["BackglassMediaWidth"]);
    settings.backglassMediaHeight = std::stoi(config["MediaDimensions"]["BackglassMediaHeight"].empty() ? "768" : config["MediaDimensions"]["BackglassMediaHeight"]);
    settings.backglassMediaX = std::stoi(config["MediaDimensions"]["BackglassMediaX"].empty() ? "0" : config["MediaDimensions"]["BackglassMediaX"]);
    settings.backglassMediaY = std::stoi(config["MediaDimensions"]["BackglassMediaY"].empty() ? "0" : config["MediaDimensions"]["BackglassMediaY"]);
    settings.backglassRotation = std::stoi(config["MediaDimensions"]["BackglassRotation"].empty() ? "0" : config["MediaDimensions"]["BackglassRotation"]);
    
    settings.dmdMediaWidth = std::stoi(config["MediaDimensions"]["DMDMediaWidth"].empty() ? "1024" : config["MediaDimensions"]["DMDMediaWidth"]);
    settings.dmdMediaHeight = std::stoi(config["MediaDimensions"]["DMDMediaHeight"].empty() ? "256" : config["MediaDimensions"]["DMDMediaHeight"]);
    settings.dmdMediaX = std::stoi(config["MediaDimensions"]["DMDMediaX"].empty() ? "0" : config["MediaDimensions"]["DMDMediaX"]);
    settings.dmdMediaY = std::stoi(config["MediaDimensions"]["DMDMediaY"].empty() ? "0" : config["MediaDimensions"]["DMDMediaY"]);
    settings.dmdRotation = std::stoi(config["MediaDimensions"]["DMDRotation"].empty() ? "0" : config["MediaDimensions"]["DMDRotation"]);
    
    settings.titleSource = config["TitleDisplay"]["TitleSource"].empty() ? "filename" : config["TitleDisplay"]["TitleSource"];
    settings.showWheel = config["TitleDisplay"]["ShowWheel"].empty() ? true : (config["TitleDisplay"]["ShowWheel"] == "true");
    settings.showTitle = config["TitleDisplay"]["ShowTitle"].empty() ? true : (config["TitleDisplay"]["ShowTitle"] == "true");
    settings.fontPath = config["TitleDisplay"]["FontPath"].empty() ? settings.fontPath : config["TitleDisplay"]["FontPath"];
    std::string fontColorStr = config["TitleDisplay"]["FontColor"].empty() ? "255,255,255,255" : config["TitleDisplay"]["FontColor"];
    sscanf(fontColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings.fontColor.r, &settings.fontColor.g, &settings.fontColor.b, &settings.fontColor.a);
    std::string fontBgColorStr = config["TitleDisplay"]["FontBgColor"].empty() ? "0,0,0,128" : config["TitleDisplay"]["FontBgColor"];
    sscanf(fontBgColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings.fontBgColor.r, &settings.fontBgColor.g, &settings.fontBgColor.b, &settings.fontBgColor.a);
    settings.fontSize = std::stoi(config["TitleDisplay"]["FontSize"].empty() ? "28" : config["TitleDisplay"]["FontSize"]);
    if (settings.enableDpiScaling) {
        settings.fontSize = static_cast<int>(settings.fontSize * settings.dpiScale);
    }
    settings.titleX = std::stoi(config["TitleDisplay"]["TitleX"].empty() ? "30" : config["TitleDisplay"]["TitleX"]);
    settings.titleY = std::stoi(config["TitleDisplay"]["TitleY"].empty() ? "1850" : config["TitleDisplay"]["TitleY"]);
    
    settings.configToggleSound = config["UISounds"]["ConfigToggleSound"].empty() ? settings.configToggleSound : config["UISounds"]["ConfigToggleSound"];
    settings.scrollPrevSound = config["UISounds"]["ScrollPrevSound"].empty() ? settings.scrollPrevSound : config["UISounds"]["ScrollPrevSound"];
    settings.scrollNextSound = config["UISounds"]["ScrollNextSound"].empty() ? settings.scrollNextSound : config["UISounds"]["ScrollNextSound"];
    settings.scrollFastPrevSound = config["UISounds"]["ScrollFastPrevSound"].empty() ? settings.scrollFastPrevSound : config["UISounds"]["ScrollFastPrevSound"];
    settings.scrollFastNextSound = config["UISounds"]["ScrollFastNextSound"].empty() ? settings.scrollFastNextSound : config["UISounds"]["ScrollFastNextSound"];
    settings.scrollJumpPrevSound = config["UISounds"]["ScrollJumpPrevSound"].empty() ? settings.scrollJumpPrevSound : config["UISounds"]["ScrollJumpPrevSound"];
    settings.scrollJumpNextSound = config["UISounds"]["ScrollJumpNextSound"].empty() ? settings.scrollJumpNextSound : config["UISounds"]["ScrollJumpNextSound"];
    settings.scrollRandomSound = config["UISounds"]["ScrollRandomSound"].empty() ? settings.scrollRandomSound : config["UISounds"]["ScrollRandomSound"];
    settings.launchTableSound = config["UISounds"]["LaunchTableSound"].empty() ? settings.launchTableSound : config["UISounds"]["LaunchTableSound"];
    settings.launchScreenshotSound = config["UISounds"]["LaunchScreenshotSound"].empty() ? settings.launchScreenshotSound : config["UISounds"]["LaunchScreenshotSound"];
    settings.configSaveSound = config["UISounds"]["ConfigSaveSound"].empty() ? settings.configSaveSound : config["UISounds"]["ConfigSaveSound"];
    settings.screenshotTakeSound = config["UISounds"]["ScreenshotTakeSound"].empty() ? settings.screenshotTakeSound : config["UISounds"]["ScreenshotTakeSound"];
    settings.screenshotQuitSound = config["UISounds"]["ScreenshotQuitSound"].empty() ? settings.screenshotQuitSound : config["UISounds"]["ScreenshotQuitSound"];
    
    keybindManager.loadKeybinds(config["Keybinds"]);
    settings.logFile = config["Internal"]["LogFile"].empty() ? "logs/debug.log" : config["Internal"]["LogFile"];
}

void SettingsParser::setDefaultSettings(Settings& settings) const {
    settings.VPXTablesPath = "/home/$USER/VPX_Tables/";
    settings.VPinballXPath = "/home/$USER/VPinballX_GL";
    settings.vpxSubCmd = "-Play";
    settings.vpxStartArgs = "";
    settings.vpxEndArgs = "";

    settings.defaultPlayfieldImage = "img/default_table.png";
    settings.defaultBackglassImage = "img/default_backglass.png";
    settings.defaultDmdImage = "img/default_dmd.png";
    settings.defaultWheelImage = "img/default_wheel.png";
    settings.defaultPlayfieldVideo = "img/default_table.mp4";
    settings.defaultBackglassVideo = "img/default_backglass.mp4";
    settings.defaultDmdVideo = "img/default_dmd.mp4";

    settings.customPlayfieldImage = "images/table.png";
    settings.customBackglassImage = "images/backglass.png";
    settings.customDmdImage = "images/marquee.png";
    settings.customWheelImage = "images/wheel.png";
    settings.customPlayfieldVideo = "video/table.mp4";
    settings.customBackglassVideo = "video/backglass.mp4";
    settings.customDmdVideo = "video/dmd.mp4";

    settings.playfieldWindowWidth = 1080;
    settings.playfieldWindowHeight = 1920;
    settings.playfieldX = -1;
    settings.playfieldY = -1;

    settings.showBackglass = true;
    settings.backglassWindowWidth = 1024;
    settings.backglassWindowHeight = 768;
    settings.backglassX = -1;
    settings.backglassY = -1;

    settings.showDMD = true;
    settings.dmdWindowWidth = 1024;
    settings.dmdWindowHeight = 256;
    settings.dmdX = -1;
    settings.dmdY = -1;

    settings.wheelMediaHeight = 350;
    settings.wheelMediaWidth = 350;
    settings.wheelMediaX = 720;
    settings.wheelMediaY = 1570;

    settings.playfieldMediaWidth = 1080;
    settings.playfieldMediaHeight = 1920;
    settings.playfieldMediaX = 0;
    settings.playfieldMediaY = 0;
    settings.playfieldRotation = 0;

    settings.backglassMediaWidth = 1024;
    settings.backglassMediaHeight = 768;
    settings.backglassMediaX = 0;
    settings.backglassMediaY = 0;
    settings.backglassRotation = 0;

    settings.dmdMediaWidth = 1024;
    settings.dmdMediaHeight = 256;
    settings.dmdMediaX = 0;
    settings.dmdMediaY = 0;
    settings.dmdRotation = 0;

    settings.fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    settings.fontColor = {255, 255, 255, 255};
    settings.fontBgColor = {0, 0, 0, 128};
    settings.fontSize = 28;
    settings.titleSource = "filename";
    settings.showWheel = true;
    settings.showTitle = true;
    settings.titleX = 30;
    settings.titleY = 1850;

    settings.scrollPrevSound = "snd/scroll_prev.mp3";
    settings.scrollNextSound = "snd/scroll_next.mp3";
    settings.scrollFastPrevSound = "snd/scroll_fast_prev.mp3";
    settings.scrollFastNextSound = "snd/scroll_fast_next.mp3";
    settings.scrollJumpPrevSound = "snd/scroll_jump_prev.mp3";
    settings.scrollJumpNextSound = "snd/scroll_jump_next.mp3";
    settings.scrollRandomSound = "snd/scroll_random.mp3";
    settings.launchTableSound = "snd/launch_table.mp3";
    settings.launchScreenshotSound = "snd/launch_screenshot.mp3";
    settings.configSaveSound = "snd/config_save.mp3";
    settings.configToggleSound = "snd/config_toggle.mp3";
    settings.screenshotTakeSound = "snd/screenshot_take.mp3";
    settings.screenshotQuitSound = "snd/screenshot_quit.mp3";

    settings.enableDpiScaling = true;
    settings.dpiScale = 1.0f;
    settings.logFile = "logs/debug.log";
}