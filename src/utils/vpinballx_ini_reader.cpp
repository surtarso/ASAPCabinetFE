#include "vpinballx_ini_reader.h"
#include "log/logging.h"
#include <fstream>
#include <sstream>
#include <filesystem>

VPinballXIniReader::VPinballXIniReader(const std::string& iniPath) : iniPath_(iniPath) {}

std::optional<VPinballXIniSettings> VPinballXIniReader::readIniSettings() const {
    if (!std::filesystem::exists(iniPath_)) {
        LOG_DEBUG("INI file does not exist: " + std::string(iniPath_));
        return std::nullopt;
    }

    std::ifstream file(iniPath_);
    if (!file.is_open()) {
        LOG_DEBUG("Failed to open INI file: " + std::string(iniPath_));
        return std::nullopt;
    }

    VPinballXIniSettings settings;
    std::string line, currentSection;
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == ';') {
            continue;
        }
        std::string trimmed = line.substr(start);

        // Check for section
        if (trimmed[0] == '[' && trimmed.back() == ']') {
            currentSection = trimmed.substr(1, trimmed.size() - 2);
            continue;
        }

        // Parse key-value pairs in relevant sections
        if (currentSection == "Standalone" || currentSection == "Player"
            || currentSection == "Backglass" || currentSection == "ScoreView"
            || currentSection == "Topper") {
            size_t eq = trimmed.find('=');
            if (eq == std::string::npos) continue;

            std::string key = trimmed.substr(0, eq);
            std::string value = trimmed.substr(eq + 1);
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
              
            // 10.8.0 ini settings
            if (currentSection == "Player") {
            // Parse playfield settings
                if (key == "PlayfieldWndX") settings.playfieldX = std::stoi(value);
                else if (key == "PlayfieldWndY") settings.playfieldY = std::stoi(value);
                else if (key == "PlayfieldWidth") settings.playfieldWidth = std::stoi(value);
                else if (key == "PlayfieldHeight") settings.playfieldHeight = std::stoi(value);
            } else if (currentSection == "Standalone") {
                if (key == "PUPPlayfieldWindowX") settings.playfieldX = std::stoi(value);
                else if (key == "PUPPlayfieldWindowY") settings.playfieldY = std::stoi(value);
                else if (key == "PUPPlayfieldWindowWidth") settings.playfieldWidth = std::stoi(value);
                else if (key == "PUPPlayfieldWindowHeight") settings.playfieldHeight = std::stoi(value);

                // Parse backglass settings
                else if (key == "B2SBackglassX" || key == "PUPBackglassWindowX") {
                    if (!settings.backglassX) settings.backglassX = std::stoi(value);
                } else if (key == "B2SBackglassY" || key == "PUPBackglassWindowY") {
                    if (!settings.backglassY) settings.backglassY = std::stoi(value);
                } else if (key == "B2SBackglassWidth" || key == "PUPBackglassWindowWidth") {
                    if (!settings.backglassWidth) settings.backglassWidth = std::stoi(value);
                } else if (key == "B2SBackglassHeight" || key == "PUPBackglassWindowHeight") {
                    if (!settings.backglassHeight) settings.backglassHeight = std::stoi(value);
                }

                // Parse DMD settings
                else if (key == "PinMAMEWindowX" || key == "FlexDMDWindowX" || key == "B2SDMDX" || key == "PUPDMDWindowX") {
                    if (!settings.dmdX) settings.dmdX = std::stoi(value);
                } else if (key == "PinMAMEWindowY" || key == "FlexDMDWindowY" || key == "B2SDMDY" || key == "PUPDMDWindowY") {
                    if (!settings.dmdY) settings.dmdY = std::stoi(value);
                } else if (key == "PinMAMEWindowWidth" || key == "FlexDMDWindowWidth" || key == "B2SDMDWidth" || key == "PUPDMDWindowWidth") {
                    if (!settings.dmdWidth) settings.dmdWidth = std::stoi(value);
                } else if (key == "PinMAMEWindowHeight" || key == "FlexDMDWindowHeight" || key == "B2SDMDHeight" || key == "PUPDMDWindowHeight") {
                    if (!settings.dmdHeight) settings.dmdHeight = std::stoi(value);
                }
            }
            // 10.8.1 ini settings
            if (currentSection == "Player") {
                // Parse Playfield settings
                if (key == "PlayfieldWndX") settings.playfieldX = std::stoi(value);
                else if (key == "PlayfieldWndY") settings.playfieldY = std::stoi(value);
                else if (key == "PlayfieldWidth") settings.playfieldWidth = std::stoi(value);
                else if (key == "PlayfieldHeight") settings.playfieldHeight = std::stoi(value);
            } else if (currentSection == "Backglass") {
                // Parse backglass settings
                if (key == "BackglassWndX") settings.backglassX = std::stoi(value);
                else if (key == "BackglassWndY") settings.backglassY = std::stoi(value);
                else if (key == "BackglassWidth") settings.backglassWidth = std::stoi(value);
                else if (key == "BackglassHeight") settings.backglassHeight = std::stoi(value);
            } else if (currentSection == "ScoreView") {
                // Parse DMD settings
                if (key == "ScoreViewWndX") settings.dmdX = std::stoi(value);
                else if (key == "ScoreViewWndY") settings.dmdY = std::stoi(value);
                else if (key == "ScoreViewWidth") settings.dmdWidth = std::stoi(value);
                else if (key == "ScoreViewHeight") settings.dmdHeight = std::stoi(value);
            } else if (currentSection == "Topper") {
                // Parse DMD settings
                if (key == "TopperWndX") settings.topperX = std::stoi(value);
                else if (key == "TopperWndY") settings.topperY = std::stoi(value);
                else if (key == "TopperWidth") settings.topperWidth = std::stoi(value);
                else if (key == "TopperHeight") settings.topperHeight = std::stoi(value);
            }
        }
    }

    file.close();
    return settings;
}