#include "config/config_service.h"
#include "utils/logging.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

ConfigService::ConfigService(const std::string& configPath) 
    : configPath_(configPath), keybindManager_() {
    loadConfig();
}

bool ConfigService::isConfigValid() const {
    return std::filesystem::exists(settings_.VPXTablesPath) && 
           std::filesystem::exists(settings_.VPinballXPath);
}

void ConfigService::loadConfig() {
    parseIniFile();
    LOG_INFO("ConfigService: Config loaded from " << configPath_);
}

void ConfigService::saveConfig(const std::map<std::string, SettingsSection>& iniData) {
    writeIniFile(iniData);
    iniData_ = iniData;
    parseIniFile();
    LOG_DEBUG("ConfigService: Config saved to " << configPath_);
}

void ConfigService::setIniData(const std::map<std::string, SettingsSection>& iniData) {
    iniData_ = iniData;
    parseIniFile();
}

void ConfigService::updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY) {
    auto& windowSettings = iniData_["WindowSettings"];
    auto updateKeyValue = [&](const std::string& key, const std::string& value) {
        auto& kv = windowSettings.keyValues;
        auto it = std::find_if(kv.begin(), kv.end(),
                               [&key](const auto& pair) { return pair.first == key; });
        if (it != kv.end()) {
            it->second = value;
        } else {
            kv.emplace_back(key, value);
            windowSettings.keyToLineIndex[key] = originalLines_.size();
        }
    };

    updateKeyValue("PlayfieldX", std::to_string(playfieldX));
    updateKeyValue("PlayfieldY", std::to_string(playfieldY));
    updateKeyValue("BackglassX", std::to_string(backglassX));
    updateKeyValue("BackglassY", std::to_string(backglassY));
    updateKeyValue("DMDX", std::to_string(dmdX));
    updateKeyValue("DMDY", std::to_string(dmdY));

    saveConfig(iniData_);
    LOG_INFO("ConfigService: Window positions saved: P(" << playfieldX << "," << playfieldY << "), B("
                 << backglassX << "," << backglassY << "), D(" << dmdX << "," << dmdY << ")");
}

void ConfigService::setDefaultSettings() {
    settings_.VPXTablesPath = "/home/$USER/VPX_Tables/";
    settings_.VPinballXPath = "/home/$USER/VPinballX_GL";
    settings_.vpxSubCmd = "-Play";
    settings_.vpxStartArgs = "";
    settings_.vpxEndArgs = "";

    // Use relative paths, exeDir will be prepended at runtime
    settings_.defaultPlayfieldImage = "img/default_table.png";
    settings_.defaultBackglassImage = "img/default_backglass.png";
    settings_.defaultDmdImage = "img/default_dmd.png";
    settings_.defaultWheelImage = "img/default_wheel.png";
    settings_.defaultPlayfieldVideo = "img/default_table.mp4";
    settings_.defaultBackglassVideo = "img/default_backglass.mp4";
    settings_.defaultDmdVideo = "img/default_dmd.mp4";

    settings_.customPlayfieldImage = "images/table.png";
    settings_.customBackglassImage = "images/backglass.png";
    settings_.customDmdImage = "images/marquee.png";
    settings_.customWheelImage = "images/wheel.png";
    settings_.customPlayfieldVideo = "video/table.mp4";
    settings_.customBackglassVideo = "video/backglass.mp4";
    settings_.customDmdVideo = "video/dmd.mp4";

    //settings_.playfieldWindowMonitor = 1;
    settings_.playfieldWindowWidth = 1080;
    settings_.playfieldWindowHeight = 1920;
    settings_.playfieldX = -1;
    settings_.playfieldY = -1;

    settings_.showBackglass = true;
    //settings_.backglassWindowMonitor = 0;
    settings_.backglassWindowWidth = 1024;
    settings_.backglassWindowHeight = 768;
    settings_.backglassX = -1;
    settings_.backglassY = -1;

    settings_.showDMD = true;
    //settings_.dmdWindowMonitor = 0;
    settings_.dmdWindowWidth = 1024;
    settings_.dmdWindowHeight = 256;
    settings_.dmdX = -1;
    settings_.dmdY = -1;

    settings_.wheelMediaHeight = 350;
    settings_.wheelMediaWidth = 350;
    settings_.wheelMediaX = 720;
    settings_.wheelMediaY = 1570;

    settings_.playfieldMediaWidth = 1080;
    settings_.playfieldMediaHeight = 1920;
    settings_.playfieldMediaX = 0;
    settings_.playfieldMediaY = 0;
    settings_.playfieldRotation = 0;

    settings_.backglassMediaWidth = 1024;
    settings_.backglassMediaHeight = 768;
    settings_.backglassMediaX = 0;
    settings_.backglassMediaY = 0;
    settings_.backglassRotation = 0;

    settings_.dmdMediaWidth = 1024;
    settings_.dmdMediaHeight = 256;
    settings_.dmdMediaX = 0;
    settings_.dmdMediaY = 0;
    settings_.dmdRotation = 0;

    settings_.fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    settings_.fontColor = {255, 255, 255, 255};
    settings_.fontBgColor = {0, 0, 0, 128};
    settings_.fontSize = 28;
    settings_.titleSource = "filename";
    settings_.showWheel = true;
    settings_.showTitle = true;
    settings_.titleX = 30;
    settings_.titleY = 1850;

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
    settings_.configToggleSound = "snd/config_toggle.mp3";
    settings_.screenshotTakeSound = "snd/screenshot_take.mp3";
    settings_.screenshotQuitSound = "snd/screenshot_quit.mp3";

    settings_.enableDpiScaling = true;
    settings_.dpiScale = 1.0f;
    settings_.logFile = "logs/debug.log";
}

void ConfigService::initializeIniData() {
    iniData_.clear();

    // VPX
    auto& vpx = iniData_["VPX"];
    vpx.keyValues = {
        {"VPXTablesPath", "/home/$USER/VPX_Tables/"},
        {"VPinballXPath", "/home/$USER/VPinballX_GL"},
        {"StartArgs", ""},
        {"EndArgs", ""}
    };
    for (size_t i = 0; i < vpx.keyValues.size(); ++i) {
        vpx.keyToLineIndex[vpx.keyValues[i].first] = i;
    }

    // Internal
    auto& internal = iniData_["Internal"];
    internal.keyValues = {
        {"SubCmd", "-Play"},
        {"LogFile", "logs/debug.log"}
    };
    for (size_t i = 0; i < internal.keyValues.size(); ++i) {
        internal.keyToLineIndex[internal.keyValues[i].first] = i;
    }

    // DefaultMedia
    auto& defaultMedia = iniData_["DefaultMedia"];
    defaultMedia.keyValues = {
        {"DefaultPlayfieldImage", "img/default_table.png"},
        {"DefaultPuPPlayfieldImage", "NYI"},
        {"DefaultBackglassImage", "img/default_backglass.png"},
        {"DefaultPuPBackglassImage", "NYI"},
        {"DefaultDmdImage", "img/default_dmd.png"},
        {"DefaultPuPDmdImage", "NYI"},
        {"DefaultPuPFullDmdImage", "NYI"},
        {"DefaultPupTopperImage", "NYI"},
        {"DefaultWheelImage", "img/default_wheel.png"},
        {"DefaultPlayfieldVideo", "img/default_table.mp4"},
        {"DefaultPuPPlayfieldVideo", "NYI"},
        {"DefaultBackglassVideo", "img/default_backglass.mp4"},
        {"DefaultPuPBackglassVideo", "NYI"},
        {"DefaultDmdVideo", "img/default_dmd.mp4"},
        {"DefaultPuPDmdVideo", "NYI"},
        {"DefaultPuPFullDmdVideo", "NYI"},
        {"DefaultPuPTopperVideo", "NYI"}
    };
    for (size_t i = 0; i < defaultMedia.keyValues.size(); ++i) {
        defaultMedia.keyToLineIndex[defaultMedia.keyValues[i].first] = i;
    }

    // CustomMedia
    auto& customMedia = iniData_["CustomMedia"];
    customMedia.keyValues = {
        {"WheelImage", "images/wheel.png"},
        {"PuPTopperImage", "NYI"},
        {"PlayfieldImage", "images/table.png"},
        {"PuPPlayfieldImage", "NYI"},
        {"BackglassImage", "images/backglass.png"},
        {"PuPBackglassImage", "NYI"},
        {"DmdImage", "images/marquee.png"},
        {"PuPDmdImage", "NYI"},
        {"PuPFullDmdImage", "NYI"},
        {"PuPTopperVideo", "NYI"},
        {"PlayfieldVideo", "video/table.mp4"},
        {"PuPPlayfieldVideo", "NYI"},
        {"BackglassVideo", "video/backglass.mp4"},
        {"PuPBackglassVideo", "NYI"},
        {"DmdVideo", "video/dmd.mp4"},
        {"PuPDmdVideo", "NYI"},
        {"PuPFullDmdVideo", "NYI"}
    };
    for (size_t i = 0; i < customMedia.keyValues.size(); ++i) {
        customMedia.keyToLineIndex[customMedia.keyValues[i].first] = i;
    }

    // DPISettings
    auto& dpiSettings = iniData_["DPISettings"];
    dpiSettings.keyValues = {
        {"EnableDpiScaling", "true"},
        {"DpiScale", "1.0"}
    };
    for (size_t i = 0; i < dpiSettings.keyValues.size(); ++i) {
        dpiSettings.keyToLineIndex[dpiSettings.keyValues[i].first] = i;
    }

    // WindowSettings
    auto& windowSettings = iniData_["WindowSettings"];
    windowSettings.keyValues = {
        {"PlayfieldMonitor", "1"},
        {"PlayfieldWidth", "1080"},
        {"PlayfieldHeight", "1920"},
        {"PlayfieldX", "-1"},
        {"PlayfieldY", "-1"},
        {"ShowBackglass", "true"},
        {"BackglassMonitor", "0"},
        {"BackglassWidth", "1024"},
        {"BackglassHeight", "768"},
        {"BackglassX", "-1"},
        {"BackglassY", "-1"},
        {"ShowDMD", "true"},
        {"DMDMonitor", "0"},
        {"DMDWidth", "1024"},
        {"DMDHeight", "256"},
        {"DMDX", "-1"},
        {"DMDY", "-1"}
    };
    for (size_t i = 0; i < windowSettings.keyValues.size(); ++i) {
        windowSettings.keyToLineIndex[windowSettings.keyValues[i].first] = i;
    }

    // MediaDimensions
    auto& mediaDimensions = iniData_["MediaDimensions"];
    mediaDimensions.keyValues = {
        {"WheelMediaHeight", "350"},
        {"WheelMediaWidth", "350"},
        {"WheelMediaX", "720"},
        {"WheelMediaY", "1570"},
        {"PlayfieldMediaWidth", "1080"},
        {"PlayfieldMediaHeight", "1920"},
        {"PlayfieldMediaX", "0"},
        {"PlayfieldMediaY", "0"},
        {"PlayfieldRotation", "0"},
        {"BackglassMediaWidth", "1024"},
        {"BackglassMediaHeight", "768"},
        {"BackglassMediaX", "0"},
        {"BackglassMediaY", "0"},
        {"BackglassRotation", "0"},
        {"DMDMediaWidth", "1024"},
        {"DMDMediaHeight", "256"},
        {"DMDMediaX", "0"},
        {"DMDMediaY", "0"},
        {"DMDRotation", "0"}
    };
    for (size_t i = 0; i < mediaDimensions.keyValues.size(); ++i) {
        mediaDimensions.keyToLineIndex[mediaDimensions.keyValues[i].first] = i;
    }

    // TitleDisplay
    auto& titleDisplay = iniData_["TitleDisplay"];
    titleDisplay.keyValues = {
        {"TitleSource", "filename"},
        {"ShowWheel", "true"},
        {"ShowTitle", "true"},
        {"FontPath", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"},
        {"FontColor", "255,255,255,255"},
        {"FontBgColor", "0,0,0,128"},
        {"FontSize", "28"},
        {"TitleX", "30"},
        {"TitleY", "1850"}
    };
    for (size_t i = 0; i < titleDisplay.keyValues.size(); ++i) {
        titleDisplay.keyToLineIndex[titleDisplay.keyValues[i].first] = i;
    }

    // Keybinds
    auto& keybinds = iniData_["Keybinds"];
    keybinds.keyValues = {
        {"PreviousTable", "Left Shift"},
        {"NextTable", "Right Shift"},
        {"FastPrevTable", "Left Ctrl"},
        {"FastNextTable", "Right Ctrl"},
        {"JumpNextLetter", "Slash"},
        {"JumpPrevLetter", "Z"},
        {"RandomTable", "R"},
        {"LaunchTable", "Return"},
        {"ToggleConfig", "C"},
        {"Quit", "Q"},
        {"ConfigSave", "Space"},
        {"ConfigClose", "Q"},
        {"ScreenshotMode", "S"},
        {"ScreenshotKey", "S"},
        {"ScreenshotQuit", "Q"}
    };
    for (size_t i = 0; i < keybinds.keyValues.size(); ++i) {
        keybinds.keyToLineIndex[keybinds.keyValues[i].first] = i;
    }

    // UISounds
    auto& uiSounds = iniData_["UISounds"];
    uiSounds.keyValues = {
        {"ScrollPrevSound", "snd/scroll_prev.mp3"},
        {"ScrollNextSound", "snd/scroll_next.mp3"},
        {"ScrollFastPrevSound", "snd/scroll_fast_prev.mp3"},
        {"ScrollFastNextSound", "snd/scroll_fast_next.mp3"},
        {"ScrollJumpPrevSound", "snd/scroll_jump_prev.mp3"},
        {"ScrollJumpNextSound", "snd/scroll_jump_next.mp3"},
        {"ScrollRandomSound", "snd/scroll_random.mp3"},
        {"LaunchTableSound", "snd/launch_table.mp3"},
        {"LaunchScreenshotSound", "snd/launch_screenshot.mp3"},
        {"ConfigSaveSound", "snd/config_save.mp3"},
        {"ConfigToggleSound", "snd/config_toggle.mp3"},
        {"ScreenshotTakeSound", "snd/screenshot_take.mp3"},
        {"ScreenshotQuitSound", "snd/screenshot_quit.mp3"}
    };
    for (size_t i = 0; i < uiSounds.keyValues.size(); ++i) {
        uiSounds.keyToLineIndex[uiSounds.keyValues[i].first] = i;
    }
}

void ConfigService::parseIniFile() {
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        LOG_INFO("ConfigService: Could not open " << configPath_ << ". Using defaults and creating config.ini.");
        setDefaultSettings();
        initializeIniData();
        writeIniFile(iniData_);
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
    auto resolvePath = [&](const std::string& value, const std::string& defaultPath) {
        if (value.empty()) return exeDir + defaultPath;
        // If path is absolute (starts with '/' or '\'), use as-is
        if (value.find('/') == 0 || value.find('\\') == 0) return value;
        // Otherwise, prepend exeDir for relative paths
        return exeDir + value;
    };

    settings_.VPXTablesPath = config["VPX"]["VPXTablesPath"].empty() ? settings_.VPXTablesPath : config["VPX"]["VPXTablesPath"];
    settings_.VPinballXPath = config["VPX"]["VPinballXPath"].empty() ? settings_.VPinballXPath : config["VPX"]["VPinballXPath"];
    settings_.vpxSubCmd = config["Internal"]["SubCmd"].empty() ? settings_.vpxSubCmd : config["Internal"]["SubCmd"];
    settings_.vpxStartArgs = config["VPX"]["StartArgs"];
    settings_.vpxEndArgs = config["VPX"]["EndArgs"];
    
    settings_.defaultPlayfieldImage = resolvePath(config["DefaultMedia"]["DefaultPlayfieldImage"], "img/default_table.png");
    settings_.defaultBackglassImage = resolvePath(config["DefaultMedia"]["DefaultBackglassImage"], "img/default_backglass.png");
    settings_.defaultDmdImage = resolvePath(config["DefaultMedia"]["DefaultDmdImage"], "img/default_dmd.png");
    settings_.defaultWheelImage = resolvePath(config["DefaultMedia"]["DefaultWheelImage"], "img/default_wheel.png");
    settings_.defaultPlayfieldVideo = resolvePath(config["DefaultMedia"]["DefaultPlayfieldVideo"], "img/default_table.mp4");
    settings_.defaultBackglassVideo = resolvePath(config["DefaultMedia"]["DefaultBackglassVideo"], "img/default_backglass.mp4");
    settings_.defaultDmdVideo = resolvePath(config["DefaultMedia"]["DefaultDmdVideo"], "img/default_dmd.mp4");
    
    settings_.customPlayfieldImage = config["CustomMedia"]["PlayfieldImage"].empty() ? settings_.customPlayfieldImage : config["CustomMedia"]["PlayfieldImage"];
    settings_.customBackglassImage = config["CustomMedia"]["BackglassImage"].empty() ? settings_.customBackglassImage : config["CustomMedia"]["BackglassImage"];
    settings_.customDmdImage = config["CustomMedia"]["DmdImage"].empty() ? settings_.customDmdImage : config["CustomMedia"]["DmdImage"];
    settings_.customWheelImage = config["CustomMedia"]["WheelImage"].empty() ? settings_.customWheelImage : config["CustomMedia"]["WheelImage"];
    settings_.customPlayfieldVideo = config["CustomMedia"]["PlayfieldVideo"].empty() ? settings_.customPlayfieldVideo : config["CustomMedia"]["PlayfieldVideo"];
    settings_.customBackglassVideo = config["CustomMedia"]["BackglassVideo"].empty() ? settings_.customBackglassVideo : config["CustomMedia"]["BackglassVideo"];
    settings_.customDmdVideo = config["CustomMedia"]["DmdVideo"].empty() ? settings_.customDmdVideo : config["CustomMedia"]["DmdVideo"];
    
    settings_.enableDpiScaling = config["DPISettings"]["EnableDpiScaling"].empty() ? true : (config["DPISettings"]["EnableDpiScaling"] == "true");
    settings_.dpiScale = std::stof(config["DPISettings"]["DpiScale"].empty() ? "1.0" : config["DPISettings"]["DpiScale"]);
    
    //settings_.playfieldWindowMonitor = std::stoi(config["WindowSettings"]["PlayfieldMonitor"].empty() ? "1" : config["WindowSettings"]["PlayfieldMonitor"]);
    settings_.playfieldWindowWidth = std::stoi(config["WindowSettings"]["PlayfieldWidth"].empty() ? "1080" : config["WindowSettings"]["PlayfieldWidth"]);
    settings_.playfieldWindowHeight = std::stoi(config["WindowSettings"]["PlayfieldHeight"].empty() ? "1920" : config["WindowSettings"]["PlayfieldHeight"]);
    settings_.playfieldX = std::stoi(config["WindowSettings"]["PlayfieldX"].empty() ? "-1" : config["WindowSettings"]["PlayfieldX"]);
    settings_.playfieldY = std::stoi(config["WindowSettings"]["PlayfieldY"].empty() ? "-1" : config["WindowSettings"]["PlayfieldY"]);
    
    settings_.showBackglass = config["WindowSettings"]["ShowBackglass"].empty() ? true : (config["WindowSettings"]["ShowBackglass"] == "true");
    //settings_.backglassWindowMonitor = std::stoi(config["WindowSettings"]["BackglassMonitor"].empty() ? "0" : config["WindowSettings"]["BackglassMonitor"]);
    settings_.backglassWindowWidth = std::stoi(config["WindowSettings"]["BackglassWidth"].empty() ? "1024" : config["WindowSettings"]["BackglassWidth"]);
    settings_.backglassWindowHeight = std::stoi(config["WindowSettings"]["BackglassHeight"].empty() ? "768" : config["WindowSettings"]["BackglassHeight"]);
    settings_.backglassX = std::stoi(config["WindowSettings"]["BackglassX"].empty() ? "-1" : config["WindowSettings"]["BackglassX"]);
    settings_.backglassY = std::stoi(config["WindowSettings"]["BackglassY"].empty() ? "-1" : config["WindowSettings"]["BackglassY"]);
    
    settings_.showDMD = config["WindowSettings"]["ShowDMD"].empty() ? true : (config["WindowSettings"]["ShowDMD"] == "true");
    //settings_.dmdWindowMonitor = std::stoi(config["WindowSettings"]["DMDMonitor"].empty() ? "0" : config["WindowSettings"]["DMDMonitor"]);
    settings_.dmdWindowWidth = std::stoi(config["WindowSettings"]["DMDWidth"].empty() ? "1024" : config["WindowSettings"]["DMDWidth"]);
    settings_.dmdWindowHeight = std::stoi(config["WindowSettings"]["DMDHeight"].empty() ? "256" : config["WindowSettings"]["DMDHeight"]);
    settings_.dmdX = std::stoi(config["WindowSettings"]["DMDX"].empty() ? "-1" : config["WindowSettings"]["DMDX"]);
    settings_.dmdY = std::stoi(config["WindowSettings"]["DMDY"].empty() ? "-1" : config["WindowSettings"]["DMDY"]);
    
    settings_.wheelMediaHeight = std::stoi(config["MediaDimensions"]["WheelMediaHeight"].empty() ? "350" : config["MediaDimensions"]["WheelMediaHeight"]);
    settings_.wheelMediaWidth = std::stoi(config["MediaDimensions"]["WheelMediaWidth"].empty() ? "350" : config["MediaDimensions"]["WheelMediaWidth"]);
    settings_.wheelMediaX = std::stoi(config["MediaDimensions"]["WheelMediaX"].empty() ? "720" : config["MediaDimensions"]["WheelMediaX"]);
    settings_.wheelMediaY = std::stoi(config["MediaDimensions"]["WheelMediaY"].empty() ? "1570" : config["MediaDimensions"]["WheelMediaY"]);
    
    settings_.playfieldMediaWidth = std::stoi(config["MediaDimensions"]["PlayfieldMediaWidth"].empty() ? "1080" : config["MediaDimensions"]["PlayfieldMediaWidth"]);
    settings_.playfieldMediaHeight = std::stoi(config["MediaDimensions"]["PlayfieldMediaHeight"].empty() ? "1920" : config["MediaDimensions"]["PlayfieldMediaHeight"]);
    settings_.playfieldMediaX = std::stoi(config["MediaDimensions"]["PlayfieldMediaX"].empty() ? "0" : config["MediaDimensions"]["PlayfieldMediaX"]);
    settings_.playfieldMediaY = std::stoi(config["MediaDimensions"]["PlayfieldMediaY"].empty() ? "0" : config["MediaDimensions"]["PlayfieldMediaY"]);
    settings_.playfieldRotation = std::stoi(config["MediaDimensions"]["PlayfieldRotation"].empty() ? "0" : config["MediaDimensions"]["PlayfieldRotation"]);
    
    settings_.backglassMediaWidth = std::stoi(config["MediaDimensions"]["BackglassMediaWidth"].empty() ? "1024" : config["MediaDimensions"]["BackglassMediaWidth"]);
    settings_.backglassMediaHeight = std::stoi(config["MediaDimensions"]["BackglassMediaHeight"].empty() ? "768" : config["MediaDimensions"]["BackglassMediaHeight"]);
    settings_.backglassMediaX = std::stoi(config["MediaDimensions"]["BackglassMediaX"].empty() ? "0" : config["MediaDimensions"]["BackglassMediaX"]);
    settings_.backglassMediaY = std::stoi(config["MediaDimensions"]["BackglassMediaY"].empty() ? "0" : config["MediaDimensions"]["BackglassMediaY"]);
    settings_.backglassRotation = std::stoi(config["MediaDimensions"]["BackglassRotation"].empty() ? "0" : config["MediaDimensions"]["BackglassRotation"]);
    
    settings_.dmdMediaWidth = std::stoi(config["MediaDimensions"]["DMDMediaWidth"].empty() ? "1024" : config["MediaDimensions"]["DMDMediaWidth"]);
    settings_.dmdMediaHeight = std::stoi(config["MediaDimensions"]["DMDMediaHeight"].empty() ? "256" : config["MediaDimensions"]["DMDMediaHeight"]);
    settings_.dmdMediaX = std::stoi(config["MediaDimensions"]["DMDMediaX"].empty() ? "0" : config["MediaDimensions"]["DMDMediaX"]);
    settings_.dmdMediaY = std::stoi(config["MediaDimensions"]["DMDMediaY"].empty() ? "0" : config["MediaDimensions"]["DMDMediaY"]);
    settings_.dmdRotation = std::stoi(config["MediaDimensions"]["DMDRotation"].empty() ? "0" : config["MediaDimensions"]["DMDRotation"]);
    
    settings_.titleSource = config["TitleDisplay"]["TitleSource"].empty() ? "filename" : config["TitleDisplay"]["TitleSource"];
    settings_.showWheel = config["TitleDisplay"]["ShowWheel"].empty() ? true : (config["TitleDisplay"]["ShowWheel"] == "true");
    settings_.showTitle = config["TitleDisplay"]["ShowTitle"].empty() ? true : (config["TitleDisplay"]["ShowTitle"] == "true");
    settings_.fontPath = config["TitleDisplay"]["FontPath"].empty() ? settings_.fontPath : config["TitleDisplay"]["FontPath"];
    std::string fontColorStr = config["TitleDisplay"]["FontColor"].empty() ? "255,255,255,255" : config["TitleDisplay"]["FontColor"];
    sscanf(fontColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings_.fontColor.r, &settings_.fontColor.g, &settings_.fontColor.b, &settings_.fontColor.a);
    std::string fontBgColorStr = config["TitleDisplay"]["FontBgColor"].empty() ? "0,0,0,128" : config["TitleDisplay"]["FontBgColor"];
    sscanf(fontBgColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings_.fontBgColor.r, &settings_.fontBgColor.g, &settings_.fontBgColor.b, &settings_.fontBgColor.a);
    settings_.fontSize = std::stoi(config["TitleDisplay"]["FontSize"].empty() ? "28" : config["TitleDisplay"]["FontSize"]);
    if (settings_.enableDpiScaling) {
        settings_.fontSize = static_cast<int>(settings_.fontSize * settings_.dpiScale);
    }
    settings_.titleX = std::stoi(config["TitleDisplay"]["TitleX"].empty() ? "30" : config["TitleDisplay"]["TitleX"]);
    settings_.titleY = std::stoi(config["TitleDisplay"]["TitleY"].empty() ? "1850" : config["TitleDisplay"]["TitleY"]);
    
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
    settings_.screenshotTakeSound = config["UISounds"]["ScreenshotTakeSound"].empty() ? settings_.screenshotTakeSound : config["UISounds"]["ScreenshotTakeSound"];
    settings_.screenshotQuitSound = config["UISounds"]["ScreenshotQuitSound"].empty() ? settings_.screenshotQuitSound : config["UISounds"]["ScreenshotQuitSound"];
    
    keybindManager_.loadKeybinds(config["Keybinds"]);
    settings_.logFile = config["Internal"]["LogFile"].empty() ? "logs/debug.log" : config["Internal"]["LogFile"];
}

void ConfigService::writeIniFile(const std::map<std::string, SettingsSection>& iniData) {
    std::filesystem::path configDir = std::filesystem::path(configPath_).parent_path();
    if (!configDir.empty() && !std::filesystem::exists(configDir)) {
        try {
            std::filesystem::create_directories(configDir);
            LOG_DEBUG("ConfigService: Created directory " << configDir);
        } catch (const std::exception& e) {
            LOG_ERROR("ConfigService: Failed to create directory " << configDir << ": " << e.what());
            return;
        }
    }

    std::ofstream file(configPath_);
    if (!file.is_open()) {
        LOG_ERROR("ConfigService: Could not write " << configPath_ << ": Permission denied or invalid path");
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
    LOG_DEBUG("ConfigService: Successfully wrote config to " << configPath_);
}