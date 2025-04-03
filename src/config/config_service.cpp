#include "config/config_service.h"
#include "utils/logging.h"
#include <fstream>
#include <sstream>

ConfigService::ConfigService(const std::string& configPath) 
    : configPath_(configPath), keybindManager_() {
    loadConfig();
}

bool ConfigService::isConfigValid() const {
    return std::filesystem::exists(settings_.vpxTablesPath) && 
           std::filesystem::exists(settings_.vpxExecutableCmd);
}

void ConfigService::loadConfig() {
    parseIniFile();
    LOG_DEBUG("Config loaded from " << configPath_);
}

void ConfigService::saveConfig(const std::map<std::string, SettingsSection>& iniData) {
    writeIniFile(iniData);
    iniData_ = iniData; // Update in-memory iniData_
    parseIniFile(); // Reload to sync settings_ and keybindManager_
    LOG_DEBUG("Config saved to " << configPath_);
}

void ConfigService::setIniData(const std::map<std::string, SettingsSection>& iniData) {
    iniData_ = iniData;
    parseIniFile(); // Sync settings_ and keybindManager_
}

void ConfigService::setDefaultSettings() {
    settings_.vpxTablesPath = "/home/tarso/Games/vpinball/build/tables/";
    settings_.vpxExecutableCmd = "/home/tarso/Games/vpinball/build/VPinballX_GL";
    settings_.vpxSubCmd = "-Play";
    std::string exeDir = configPath_.substr(0, configPath_.find_last_of('/') + 1);
    settings_.defaultTableImage = exeDir + "img/default_table.png";
    settings_.defaultBackglassImage = exeDir + "img/default_backglass.png";
    settings_.defaultDmdImage = exeDir + "img/default_dmd.png";
    settings_.defaultWheelImage = exeDir + "img/default_wheel.png";
    settings_.defaultTableVideo = exeDir + "img/default_table.mp4";
    settings_.defaultBackglassVideo = exeDir + "img/default_backglass.mp4";
    settings_.defaultDmdVideo = exeDir + "img/default_dmd.mp4";
    settings_.customTableImage = "images/table.png";
    settings_.customBackglassImage = "images/backglass.png";
    settings_.customDmdImage = "images/marquee.png";
    settings_.customWheelImage = "images/wheel.png";
    settings_.customTableVideo = "video/table.mp4";
    settings_.customBackglassVideo = "video/backglass.mp4";
    settings_.customDmdVideo = "video/dmd.mp4";
    settings_.mainWindowMonitor = 1;
    settings_.mainWindowWidth = 1080;
    settings_.mainWindowHeight = 1920;
    settings_.wheelImageSize = 300;
    settings_.wheelImageMargin = 24;
    settings_.secondWindowMonitor = 0;
    settings_.secondWindowWidth = 1024;
    settings_.secondWindowHeight = 1024;
    settings_.backglassMediaWidth = 1024;
    settings_.backglassMediaHeight = 768;
    settings_.dmdMediaWidth = 1024;
    settings_.dmdMediaHeight = 256;
    settings_.fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    settings_.fontColor = {255, 255, 255, 255};
    settings_.fontBgColor = {0, 0, 0, 128};
    settings_.fontSize = 28;
    settings_.showWheel = true;
    settings_.showTitle = true;
    settings_.configToggleSound = "snd/config_toggle.mp3";
    settings_.scrollPrevSound = "snd/scroll_prev.mp3";
    settings_.scrollNextSound = "snd/scroll_next.mp3";
    settings_.scrollFastPrevSound = "snd/scroll_fast_prev.mp3";
    settings_.scrollFastNextSound = "snd/scroll_fast_next.mp3";
    settings_.scrollJumpPrevSound = "snd/scroll_jump_prev.mp3";
    settings_.scrollJumpNextSound = "snd/scroll_jump_next.mp3";
    settings_.scrollRandomSound = "snd/scroll_random.mp3";
    settings_.launchTableSound = "snd/launch_table.mp3";
    settings_.launchScreenshotSound = "snd/launch_screenshot.mp3";
    settings_.configSaveSound = "snd/config_save.mp3";
    settings_.configCloseSound = "snd/config_close.mp3";
    settings_.quitSound = "snd/quit.mp3";
    settings_.screenshotTakeSound = "snd/screenshot_take.mp3";
    settings_.screenshotQuitSound = "snd/screenshot_quit.mp3";
    settings_.enableDpiScaling = true;
    settings_.dpiScale = 1.0f;
}

void ConfigService::parseIniFile() {
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        LOG_DEBUG("Could not open " << configPath_ << ". Using defaults.");
        setDefaultSettings();
        return;
    }

    originalLines_.clear();
    std::string line;
    while (std::getline(file, line)) {
        originalLines_.push_back(line);
    }
    file.close();

    std::map<std::string, std::map<std::string, std::string>> config;
    std::string currentSection;
    size_t lineIndex = 0;
    iniData_.clear();
    for (const auto& line : originalLines_) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == ';') {
            lineIndex++;
            continue;
        }
        std::string trimmed = line.substr(start);
        if (trimmed[0] == '[' && trimmed.back() == ']') {
            currentSection = trimmed.substr(1, trimmed.size() - 2);
            iniData_[currentSection] = SettingsSection();
        } else if (!currentSection.empty()) {
            size_t eq = trimmed.find('=');
            if (eq != std::string::npos) {
                std::string key = trimmed.substr(0, eq);
                std::string value = trimmed.substr(eq + 1);
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                if (key == "JumpNextLetter" && value == "Slash") value = "/";
                config[currentSection][key] = value;
                iniData_[currentSection].keyValues.emplace_back(key, value);
                iniData_[currentSection].keyToLineIndex[key] = lineIndex;
            }
        }
        lineIndex++;
    }

    setDefaultSettings();
    std::string exeDir = configPath_.substr(0, configPath_.find_last_of('/') + 1);
    settings_.vpxTablesPath = config["VPX"]["TablesPath"].empty() ? settings_.vpxTablesPath : config["VPX"]["TablesPath"];
    settings_.vpxExecutableCmd = config["VPX"]["ExecutableCmd"].empty() ? settings_.vpxExecutableCmd : config["VPX"]["ExecutableCmd"];
    settings_.vpxSubCmd = config["Internal"]["SubCmd"].empty() ? settings_.vpxSubCmd : config["Internal"]["SubCmd"];
    settings_.vpxStartArgs = config["VPX"]["StartArgs"];
    settings_.vpxEndArgs = config["VPX"]["EndArgs"];
    settings_.defaultTableImage = exeDir + (config["DefaultMedia"]["DefaultTableImage"].empty() ? "img/default_table.png" : config["DefaultMedia"]["DefaultTableImage"]);
    settings_.defaultBackglassImage = exeDir + (config["DefaultMedia"]["DefaultBackglassImage"].empty() ? "img/default_backglass.png" : config["DefaultMedia"]["DefaultBackglassImage"]);
    settings_.defaultDmdImage = exeDir + (config["DefaultMedia"]["DefaultDmdImage"].empty() ? "img/default_dmd.png" : config["DefaultMedia"]["DefaultDmdImage"]);
    settings_.defaultWheelImage = exeDir + (config["DefaultMedia"]["DefaultWheelImage"].empty() ? "img/default_wheel.png" : config["DefaultMedia"]["DefaultWheelImage"]);
    settings_.defaultTableVideo = exeDir + (config["DefaultMedia"]["DefaultTableVideo"].empty() ? "img/default_table.mp4" : config["DefaultMedia"]["DefaultTableVideo"]);
    settings_.defaultBackglassVideo = exeDir + (config["DefaultMedia"]["DefaultBackglassVideo"].empty() ? "img/default_backglass.mp4" : config["DefaultMedia"]["DefaultBackglassVideo"]);
    settings_.defaultDmdVideo = exeDir + (config["DefaultMedia"]["DefaultDmdVideo"].empty() ? "img/default_dmd.mp4" : config["DefaultMedia"]["DefaultDmdVideo"]);
    settings_.customTableImage = config["CustomMedia"]["TableImage"].empty() ? settings_.customTableImage : config["CustomMedia"]["TableImage"];
    settings_.customBackglassImage = config["CustomMedia"]["BackglassImage"].empty() ? settings_.customBackglassImage : config["CustomMedia"]["BackglassImage"];
    settings_.customDmdImage = config["CustomMedia"]["DmdImage"].empty() ? settings_.customDmdImage : config["CustomMedia"]["DmdImage"];
    settings_.customWheelImage = config["CustomMedia"]["WheelImage"].empty() ? settings_.customWheelImage : config["CustomMedia"]["WheelImage"];
    settings_.customTableVideo = config["CustomMedia"]["TableVideo"].empty() ? settings_.customTableVideo : config["CustomMedia"]["TableVideo"];
    settings_.customBackglassVideo = config["CustomMedia"]["BackglassVideo"].empty() ? settings_.customBackglassVideo : config["CustomMedia"]["BackglassVideo"];
    settings_.customDmdVideo = config["CustomMedia"]["DmdVideo"].empty() ? settings_.customDmdVideo : config["CustomMedia"]["DmdVideo"];
    settings_.mainWindowMonitor = std::stoi(config["WindowSettings"]["MainMonitor"].empty() ? "1" : config["WindowSettings"]["MainMonitor"]);
    settings_.mainWindowWidth = std::stoi(config["WindowSettings"]["MainWidth"].empty() ? "1080" : config["WindowSettings"]["MainWidth"]);
    settings_.mainWindowHeight = std::stoi(config["WindowSettings"]["MainHeight"].empty() ? "1920" : config["WindowSettings"]["MainHeight"]);
    settings_.wheelImageSize = std::stoi(config["MediaDimensions"]["WheelImageSize"].empty() ? "300" : config["MediaDimensions"]["WheelImageSize"]);
    settings_.wheelImageMargin = std::stoi(config["MediaDimensions"]["WheelImageMargin"].empty() ? "24" : config["MediaDimensions"]["WheelImageMargin"]);
    settings_.secondWindowMonitor = std::stoi(config["WindowSettings"]["SecondMonitor"].empty() ? "0" : config["WindowSettings"]["SecondMonitor"]);
    settings_.secondWindowWidth = std::stoi(config["WindowSettings"]["SecondWidth"].empty() ? "1024" : config["WindowSettings"]["SecondWidth"]);
    settings_.secondWindowHeight = std::stoi(config["WindowSettings"]["SecondHeight"].empty() ? "1024" : config["WindowSettings"]["SecondHeight"]);
    settings_.backglassMediaWidth = std::stoi(config["MediaDimensions"]["BackglassWidth"].empty() ? "1024" : config["MediaDimensions"]["BackglassWidth"]);
    settings_.backglassMediaHeight = std::stoi(config["MediaDimensions"]["BackglassHeight"].empty() ? "768" : config["MediaDimensions"]["BackglassHeight"]);
    settings_.dmdMediaWidth = std::stoi(config["MediaDimensions"]["DmdWidth"].empty() ? "1024" : config["MediaDimensions"]["DmdWidth"]);
    settings_.dmdMediaHeight = std::stoi(config["MediaDimensions"]["DmdHeight"].empty() ? "256" : config["MediaDimensions"]["DmdHeight"]);
    settings_.fontPath = config["TitleDisplay"]["FontPath"].empty() ? settings_.fontPath : config["TitleDisplay"]["FontPath"];
    std::string fontColorStr = config["TitleDisplay"]["FontColor"].empty() ? "255,255,255,255" : config["TitleDisplay"]["FontColor"];
    sscanf(fontColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings_.fontColor.r, &settings_.fontColor.g, &settings_.fontColor.b, &settings_.fontColor.a);
    std::string fontBgColorStr = config["TitleDisplay"]["FontBgColor"].empty() ? "0,0,0,128" : config["TitleDisplay"]["FontBgColor"];
    sscanf(fontBgColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings_.fontBgColor.r, &settings_.fontBgColor.g, &settings_.fontBgColor.b, &settings_.fontBgColor.a);
    settings_.fontSize = std::stoi(config["TitleDisplay"]["FontSize"].empty() ? "28" : config["TitleDisplay"]["FontSize"]);
    
    // Parse show/hide settings
    settings_.showWheel = config["TitleDisplay"]["ShowWheel"].empty() ? true : 
        (config["TitleDisplay"]["ShowWheel"] == "true");
    settings_.showTitle = config["TitleDisplay"]["ShowTitle"].empty() ? true : 
        (config["TitleDisplay"]["ShowTitle"] == "true");

    if (settings_.enableDpiScaling) {
        settings_.fontSize = static_cast<int>(settings_.fontSize * settings_.dpiScale);
    }
    settings_.configToggleSound = config["UISounds"]["ConfigToggleSound"].empty() ? settings_.configToggleSound : config["UISounds"]["ConfigToggleSound"];
    settings_.scrollPrevSound = config["UISounds"]["ScrollPrevSound"].empty() ? settings_.scrollPrevSound : config["UISounds"]["ScrollPrevSound"];
    settings_.scrollNextSound = config["UISounds"]["ScrollNextSound"].empty() ? settings_.scrollNextSound : config["UISounds"]["ScrollNextSound"];
    settings_.scrollFastPrevSound = config["UISounds"]["ScrollFastPrevSound"].empty() ? settings_.scrollFastPrevSound : config["UISounds"]["ScrollFastPrevSound"];
    settings_.scrollFastNextSound = config["UISounds"]["ScrollFastNextSound"].empty() ? settings_.scrollFastNextSound : config["UISounds"]["ScrollFastNextSound"];
    settings_.scrollJumpPrevSound = config["UISounds"]["ScrollJumpPrevSound"].empty() ? settings_.scrollJumpPrevSound : config["UISounds"]["ScrollJumpPrevSound"];
    settings_.scrollJumpNextSound = config["UISounds"]["ScrollJumpNextSound"].empty() ? settings_.scrollJumpNextSound : config["UISounds"]["ScrollJumpNextSound"];
    settings_.scrollRandomSound = config["UISounds"]["ScrollRandomSound"].empty() ? settings_.scrollRandomSound : config["UISounds"]["ScrollRandomSound"];
    settings_.launchTableSound = config["UISounds"]["LaunchTableSound"].empty() ? settings_.launchTableSound : config["UISounds"]["LaunchTableSound"];
    settings_.launchScreenshotSound = config["UISounds"]["LaunchScreenshotSound"].empty() ? settings_.launchScreenshotSound : config["UISounds"]["LaunchScreenshotSound"];
    settings_.configSaveSound = config["UISounds"]["ConfigSaveSound"].empty() ? settings_.configSaveSound : config["UISounds"]["ConfigSaveSound"];
    settings_.configCloseSound = config["UISounds"]["ConfigCloseSound"].empty() ? settings_.configCloseSound : config["UISounds"]["ConfigCloseSound"];
    settings_.quitSound = config["UISounds"]["QuitSound"].empty() ? settings_.quitSound : config["UISounds"]["QuitSound"];
    settings_.screenshotTakeSound = config["UISounds"]["ScreenshotTakeSound"].empty() ? settings_.screenshotTakeSound : config["UISounds"]["ScreenshotTakeSound"];
    settings_.screenshotQuitSound = config["UISounds"]["ScreenshotQuitSound"].empty()? settings_.screenshotQuitSound : config["UISounds"]["ScreenshotQuitSound"];
    settings_.enableDpiScaling = config["DPISettings"]["EnableDpiScaling"].empty() ? true : 
        (config["DPISettings"]["EnableDpiScaling"] == "true");
    settings_.dpiScale = std::stof(config["DPISettings"]["DpiScale"].empty() ? "1.0" : config["DPISettings"]["DpiScale"]);
    keybindManager_.loadKeybinds(config["Keybinds"]);
}

void ConfigService::writeIniFile(const std::map<std::string, SettingsSection>& iniData) {
    std::ofstream file(configPath_);
    if (!file.is_open()) {
        LOG_DEBUG("Could not write " << configPath_);
        return;
    }

    for (const auto& [section, configSection] : iniData) {
        file << "[" << section << "]\n";
        for (const auto& kv : configSection.keyValues) {
            file << kv.first << "=" << kv.second << "\n";
        }
        file << "\n";
    }
    file.close();
}
