#include "config_service.h"
#include "utils/logging.h"
#include <sstream>
#include <algorithm>

ConfigService::ConfigService(const std::string& configPath) 
    : configPath_(configPath), keybindManager_(), fileHandler_(configPath), parser_(configPath) {
    loadConfig();
}

bool ConfigService::isConfigValid() const {
    return std::filesystem::exists(settings_.VPXTablesPath) && 
           std::filesystem::exists(settings_.VPinballXPath);
}

void ConfigService::loadConfig() {
    iniData_ = fileHandler_.readConfig(originalLines_);
    if (iniData_.empty()) {
        LOG_INFO("ConfigService: Could not open " << configPath_ << ". Using defaults and creating config.ini.");
        setDefaultSettings();
        initializeIniData();
        fileHandler_.writeConfig(iniData_);
    }
    parser_.parse(iniData_, settings_, keybindManager_);
    LOG_INFO("ConfigService: Config loaded from " << configPath_);
}

void ConfigService::saveConfig(const std::map<std::string, SettingsSection>& iniData) {
    fileHandler_.writeConfig(iniData);
    iniData_ = iniData;
    parser_.parse(iniData_, settings_, keybindManager_);
    LOG_DEBUG("ConfigService: Config saved to " << configPath_);
}

void ConfigService::setIniData(const std::map<std::string, SettingsSection>& iniData) {
    iniData_ = iniData;
    parser_.parse(iniData_, settings_, keybindManager_);
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
    parser_.parse({}, settings_, keybindManager_);
}

void ConfigService::initializeIniData() {
    iniData_.clear();

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

    auto& internal = iniData_["Internal"];
    internal.keyValues = {
        {"SubCmd", "-Play"},
        {"LogFile", "logs/debug.log"}
    };
    for (size_t i = 0; i < internal.keyValues.size(); ++i) {
        internal.keyToLineIndex[internal.keyValues[i].first] = i;
    }

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

    auto& dpiSettings = iniData_["DPISettings"];
    dpiSettings.keyValues = {
        {"EnableDpiScaling", "true"},
        {"DpiScale", "1.0"}
    };
    for (size_t i = 0; i < dpiSettings.keyValues.size(); ++i) {
        dpiSettings.keyToLineIndex[dpiSettings.keyValues[i].first] = i;
    }

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