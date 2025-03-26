#include "config/config_manager.h"
#include "table/asset_manager.h"
#include "table/table_manager.h"
#include "utils/logging.h"
#include <fstream>
#include <sstream>
#include <map>

ConfigManager::ConfigManager(const std::string& configPath) : configPath_(configPath) {
    loadConfig();
}

void ConfigManager::loadConfig() {
    parseIniFile(configPath_);
}

void ConfigManager::saveConfig() {
    writeIniFile(configPath_);
}

const Settings& ConfigManager::getSettings() const {
    return settings;
}

KeybindManager& ConfigManager::getKeybindManager() {
    return keybindManager_;
}

const KeybindManager& ConfigManager::getKeybindManager() const {
    return keybindManager_;
}

void ConfigManager::applyConfigChanges(SDL_Window* mainWindow, SDL_Window* playfieldWindow) {
    SDL_SetWindowFullscreen(mainWindow, settings.mainWindowWidth == 0 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_SetWindowSize(mainWindow, settings.mainWindowWidth, settings.mainWindowHeight);
    SDL_SetWindowFullscreen(playfieldWindow, settings.secondWindowWidth == 0 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_SetWindowSize(playfieldWindow, settings.secondWindowWidth, settings.secondWindowHeight);
}

void ConfigManager::notifyConfigChanged(AssetManager& assetManager, size_t& selectedTableIndex, std::vector<Table>& tables) {
    loadConfig();
    assetManager.loadTableAssets(selectedTableIndex, tables);
}

void ConfigManager::parseIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not open " << filename << ". Using defaults.");
        settings.vpxTablesPath = "/home/tarso/Games/vpinball/build/tables/";
        settings.vpxExecutableCmd = "/home/tarso/Games/vpinball/build/VPinballX_GL";
        settings.vpxSubCmd = "-Play";
        settings.tableChangeSound = "snd/table_change.mp3";
        settings.tableLoadSound = "snd/table_load.mp3";
        return;
    }

    std::map<std::string, std::map<std::string, std::string>> config;
    std::string currentSection;
    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == ';') continue;
        std::string trimmed = line.substr(start);
        if (trimmed[0] == '[' && trimmed.back() == ']') {
            currentSection = trimmed.substr(1, trimmed.size() - 2);
        } else {
            size_t eq = trimmed.find('=');
            if (eq != std::string::npos) {
                std::string key = trimmed.substr(0, eq);
                std::string value = trimmed.substr(eq + 1);
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                // Normalize "Slash" to "/" for consistency
                if (key == "JumpNextLetter" && value == "Slash") {
                    value = "/";
                }
                config[currentSection][key] = value;
            }
        }
    }
    file.close();

    // VPX settings
    settings.vpxTablesPath = config["VPX"]["TablesPath"].empty() ? "/home/tarso/Games/vpinball/build/tables/" : config["VPX"]["TablesPath"];
    settings.vpxExecutableCmd = config["VPX"]["ExecutableCmd"].empty() ? "/home/tarso/Games/vpinball/build/VPinballX_GL" : config["VPX"]["ExecutableCmd"];
    settings.vpxSubCmd = config["Internal"]["SubCmd"].empty() ? "-Play" : config["Internal"]["SubCmd"];
    settings.vpxStartArgs = config["VPX"]["StartArgs"];
    settings.vpxEndArgs = config["VPX"]["EndArgs"];

    // Default media paths
    std::string exeDir = filename.substr(0, filename.find_last_of('/') + 1);
    settings.defaultTableImage = exeDir + (config["Internal"]["DefaultTableImage"].empty() ? "img/default_table.png" : config["Internal"]["DefaultTableImage"]);
    settings.defaultBackglassImage = exeDir + (config["Internal"]["DefaultBackglassImage"].empty() ? "img/default_backglass.png" : config["Internal"]["DefaultBackglassImage"]);
    settings.defaultDmdImage = exeDir + (config["Internal"]["DefaultDmdImage"].empty() ? "img/default_dmd.png" : config["Internal"]["DefaultDmdImage"]);
    settings.defaultWheelImage = exeDir + (config["Internal"]["DefaultWheelImage"].empty() ? "img/default_wheel.png" : config["Internal"]["DefaultWheelImage"]);
    settings.defaultTableVideo = exeDir + (config["Internal"]["DefaultTableVideo"].empty() ? "img/default_table.mp4" : config["Internal"]["DefaultTableVideo"]);
    settings.defaultBackglassVideo = exeDir + (config["Internal"]["DefaultBackglassVideo"].empty() ? "img/default_backglass.mp4" : config["Internal"]["DefaultBackglassVideo"]);
    settings.defaultDmdVideo = exeDir + (config["Internal"]["DefaultDmdVideo"].empty() ? "img/default_dmd.mp4" : config["Internal"]["DefaultDmdVideo"]);

    // Custom media paths
    settings.customTableImage = config["CustomMedia"]["TableImage"].empty() ? "images/table.png" : config["CustomMedia"]["TableImage"];
    settings.customBackglassImage = config["CustomMedia"]["BackglassImage"].empty() ? "images/backglass.png" : config["CustomMedia"]["BackglassImage"];
    settings.customDmdImage = config["CustomMedia"]["DmdImage"].empty() ? "images/marquee.png" : config["CustomMedia"]["DmdImage"];
    settings.customWheelImage = config["CustomMedia"]["WheelImage"].empty() ? "images/wheel.png" : config["CustomMedia"]["WheelImage"];
    settings.customTableVideo = config["CustomMedia"]["TableVideo"].empty() ? "video/table.mp4" : config["CustomMedia"]["TableVideo"];
    settings.customBackglassVideo = config["CustomMedia"]["BackglassVideo"].empty() ? "video/backglass.mp4" : config["CustomMedia"]["BackglassVideo"];
    settings.customDmdVideo = config["CustomMedia"]["DmdVideo"].empty() ? "video/dmd.mp4" : config["CustomMedia"]["DmdVideo"];

    // Window settings
    settings.mainWindowMonitor = std::stoi(config["WindowSettings"]["MainMonitor"].empty() ? "1" : config["WindowSettings"]["MainMonitor"]);
    settings.mainWindowWidth = std::stoi(config["WindowSettings"]["MainWidth"].empty() ? "1080" : config["WindowSettings"]["MainWidth"]);
    settings.mainWindowHeight = std::stoi(config["WindowSettings"]["MainHeight"].empty() ? "1920" : config["WindowSettings"]["MainHeight"]);
    settings.wheelImageSize = std::stoi(config["MediaDimensions"]["WheelImageSize"].empty() ? "300" : config["MediaDimensions"]["WheelImageSize"]);
    settings.wheelImageMargin = std::stoi(config["MediaDimensions"]["WheelImageMargin"].empty() ? "24" : config["MediaDimensions"]["WheelImageMargin"]);
    settings.secondWindowMonitor = std::stoi(config["WindowSettings"]["SecondMonitor"].empty() ? "0" : config["WindowSettings"]["SecondMonitor"]);
    settings.secondWindowHeight = std::stoi(config["WindowSettings"]["SecondHeight"].empty() ? "1024" : config["WindowSettings"]["SecondHeight"]);
    settings.secondWindowWidth = std::stoi(config["WindowSettings"]["SecondWidth"].empty() ? "1024" : config["WindowSettings"]["SecondWidth"]);
    settings.backglassMediaWidth = std::stoi(config["MediaDimensions"]["BackglassWidth"].empty() ? "1024" : config["MediaDimensions"]["BackglassWidth"]);
    settings.backglassMediaHeight = std::stoi(config["MediaDimensions"]["BackglassHeight"].empty() ? "768" : config["MediaDimensions"]["BackglassHeight"]);
    settings.dmdMediaWidth = std::stoi(config["MediaDimensions"]["DmdWidth"].empty() ? "1024" : config["MediaDimensions"]["DmdWidth"]);
    settings.dmdMediaHeight = std::stoi(config["MediaDimensions"]["DmdHeight"].empty() ? "256" : config["MediaDimensions"]["DmdHeight"]);

    // Title display
    settings.fontPath = config["TitleDisplay"]["FontPath"].empty() ? "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" : config["TitleDisplay"]["FontPath"];
    std::string fontColorStr = config["TitleDisplay"]["FontColor"].empty() ? "255,255,255,255" : config["TitleDisplay"]["FontColor"];
    sscanf(fontColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings.fontColor.r, &settings.fontColor.g, &settings.fontColor.b, &settings.fontColor.a);
    std::string fontBgColorStr = config["TitleDisplay"]["FontBgColor"].empty() ? "0,0,0,128" : config["TitleDisplay"]["FontBgColor"];
    sscanf(fontBgColorStr.c_str(), "%hhu,%hhu,%hhu,%hhu", &settings.fontBgColor.r, &settings.fontBgColor.g, &settings.fontBgColor.b, &settings.fontBgColor.a);
    settings.fontSize = std::stoi(config["TitleDisplay"]["Size"].empty() ? "28" : config["TitleDisplay"]["Size"]);

    // Sound settings
    settings.tableChangeSound = config["Internal"]["TableChangeSound"].empty() ? "snd/table_change.mp3" : config["Internal"]["TableChangeSound"];
    settings.tableLoadSound = config["Internal"]["TableLoadSound"].empty() ? "snd/table_load.mp3" : config["Internal"]["TableLoadSound"];
    
    // Load keybindings
    keybindManager_.loadKeybinds(config["Keybinds"]);
}

void ConfigManager::writeIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not write " << filename);
        return;
    }

    file << "[VPX]\n";
    file << "TablesPath=" << settings.vpxTablesPath << "\n";
    file << "ExecutableCmd=" << settings.vpxExecutableCmd << "\n";
    file << "StartArgs=" << settings.vpxStartArgs << "\n";
    file << "EndArgs=" << settings.vpxEndArgs << "\n";
    file << "\n[Internal]\n";
    file << "SubCmd=" << settings.vpxSubCmd << "\n";
    file << "DefaultTableImage=" << settings.defaultTableImage.substr(settings.defaultTableImage.find("img/")) << "\n";
    file << "DefaultBackglassImage=" << settings.defaultBackglassImage.substr(settings.defaultBackglassImage.find("img/")) << "\n";
    file << "DefaultDmdImage=" << settings.defaultDmdImage.substr(settings.defaultBackglassImage.find("img/")) << "\n";
    file << "DefaultWheelImage=" << settings.defaultWheelImage.substr(settings.defaultWheelImage.find("img/")) << "\n";
    file << "DefaultTableVideo=" << settings.defaultTableVideo.substr(settings.defaultTableVideo.find("img/")) << "\n";
    file << "DefaultBackglassVideo=" << settings.defaultBackglassVideo.substr(settings.defaultBackglassVideo.find("img/")) << "\n";
    file << "DefaultDmdVideo=" << settings.defaultDmdVideo.substr(settings.defaultDmdVideo.find("img/")) << "\n";
    file << "TableChangeSound=" << settings.tableChangeSound << "\n";
    file << "TableLoadSound=" << settings.tableLoadSound << "\n";
    file << "\n[CustomMedia]\n";
    file << "TableImage=" << settings.customTableImage << "\n";
    file << "BackglassImage=" << settings.customBackglassImage << "\n";
    file << "DmdImage=" << settings.customDmdImage << "\n";
    file << "WheelImage=" << settings.customWheelImage << "\n";
    file << "TableVideo=" << settings.customTableVideo << "\n";
    file << "BackglassVideo=" << settings.customBackglassVideo << "\n";
    file << "DmdVideo=" << settings.customDmdVideo << "\n";
    file << "\n[WindowSettings]\n";
    file << "MainMonitor=" << settings.mainWindowMonitor << "\n";
    file << "MainWidth=" << settings.mainWindowWidth << "\n";
    file << "MainHeight=" << settings.mainWindowHeight << "\n";
    file << "SecondMonitor=" << settings.secondWindowMonitor << "\n";
    file << "SecondWidth=" << settings.secondWindowWidth << "\n";
    file << "SecondHeight=" << settings.secondWindowHeight << "\n";
    file << "\n[MediaDimensions]\n";
    file << "WheelImageSize=" << settings.wheelImageSize << "\n";
    file << "WheelImageMargin=" << settings.wheelImageMargin << "\n";
    file << "BackglassWidth=" << settings.backglassMediaWidth << "\n";
    file << "BackglassHeight=" << settings.backglassMediaHeight << "\n";
    file << "DmdWidth=" << settings.dmdMediaWidth << "\n";
    file << "DmdHeight=" << settings.dmdMediaHeight << "\n";
    file << "\n[TitleDisplay]\n";
    file << "FontPath=" << settings.fontPath << "\n";
    file << "FontColor=" << (int)settings.fontColor.r << "," << (int)settings.fontColor.g << "," << (int)settings.fontColor.b << "," << (int)settings.fontColor.a << "\n";
    file << "FontBgColor=" << (int)settings.fontBgColor.r << "," << (int)settings.fontBgColor.g << "," << (int)settings.fontBgColor.b << "," << (int)settings.fontBgColor.a << "\n";
    file << "Size=" << settings.fontSize << "\n";
    file << "\n";

    // Save keybindings
    keybindManager_.saveKeybinds(file);

    file.close();
}