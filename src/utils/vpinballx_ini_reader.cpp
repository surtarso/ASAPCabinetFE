#include "vpinballx_ini_reader.h"
#include "log/logging.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>

VPinballXIniReader::VPinballXIniReader(const std::string& iniPath) : iniPath_(iniPath) {}

// helper: check if string is an integer literal (optional sign + digits)
static inline bool isInteger(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    }
    return true;
}

// helper: safely convert string to int with validation
static inline bool tryParseInt(const std::string& s, int& out) {
    if (!isInteger(s)) return false;
    try {
        out = std::stoi(s);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<VPinballXIniSettings> VPinballXIniReader::readIniSettings() const {
    if (!std::filesystem::exists(iniPath_)) {
        LOG_DEBUG("INI file does not exist: " + iniPath_);
        return std::nullopt;
    }

    std::ifstream file(iniPath_);
    if (!file.is_open()) {
        LOG_DEBUG("Failed to open INI file: " + iniPath_);
        return std::nullopt;
    }

    VPinballXIniSettings settings;
    std::string line, currentSection;

    while (std::getline(file, line)) {
        // trim whitespace and skip comments
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == ';')
            continue;
        std::string trimmed = line.substr(start);

        // section header
        if (trimmed[0] == '[' && trimmed.back() == ']') {
            currentSection = trimmed.substr(1, trimmed.size() - 2);
            continue;
        }

        // relevant sections only
        if (currentSection != "Standalone" && currentSection != "Player"
            && currentSection != "Backglass" && currentSection != "ScoreView"
            && currentSection != "Topper" && currentSection != "Plugin.B2S"
            && currentSection != "Plugin.B2SLegacy")
            continue;

        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trimmed.substr(0, eq);
        std::string value = trimmed.substr(eq + 1);
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        int parsed = 0;
        auto assignIfInt = [&](auto& target, const std::string& name) {
            if (tryParseInt(value, parsed))
                target = parsed;
            else
                LOG_DEBUG("Invalid numeric value for " + name + ": " + value);
        };

        // 10.8.0 ini
        if (currentSection == "Player") {
            if (key == "PlayfieldWndX") assignIfInt(settings.playfieldX, key);
            else if (key == "PlayfieldWndY") assignIfInt(settings.playfieldY, key);
            else if (key == "PlayfieldWidth") assignIfInt(settings.playfieldWidth, key);
            else if (key == "PlayfieldHeight") assignIfInt(settings.playfieldHeight, key);
        }

        else if (currentSection == "Standalone") {
            if (key == "PUPPlayfieldWindowX") assignIfInt(settings.playfieldX, key);
            else if (key == "PUPPlayfieldWindowY") assignIfInt(settings.playfieldY, key);
            else if (key == "PUPPlayfieldWindowWidth") assignIfInt(settings.playfieldWidth, key);
            else if (key == "PUPPlayfieldWindowHeight") assignIfInt(settings.playfieldHeight, key);

            else if (key == "B2SBackglassX" || key == "PUPBackglassWindowX") {
                if (!settings.backglassX) assignIfInt(settings.backglassX, key);
            } else if (key == "B2SBackglassY" || key == "PUPBackglassWindowY") {
                if (!settings.backglassY) assignIfInt(settings.backglassY, key);
            } else if (key == "B2SBackglassWidth" || key == "PUPBackglassWindowWidth") {
                if (!settings.backglassWidth) assignIfInt(settings.backglassWidth, key);
            } else if (key == "B2SBackglassHeight" || key == "PUPBackglassWindowHeight") {
                if (!settings.backglassHeight) assignIfInt(settings.backglassHeight, key);
            }

            else if (key == "PinMAMEWindowX" || key == "FlexDMDWindowX" || key == "B2SDMDX" || key == "PUPDMDWindowX") {
                if (!settings.dmdX) assignIfInt(settings.dmdX, key);
            } else if (key == "PinMAMEWindowY" || key == "FlexDMDWindowY" || key == "B2SDMDY" || key == "PUPDMDWindowY") {
                if (!settings.dmdY) assignIfInt(settings.dmdY, key);
            } else if (key == "PinMAMEWindowWidth" || key == "FlexDMDWindowWidth" || key == "B2SDMDWidth" || key == "PUPDMDWindowWidth") {
                if (!settings.dmdWidth) assignIfInt(settings.dmdWidth, key);
            } else if (key == "PinMAMEWindowHeight" || key == "FlexDMDWindowHeight" || key == "B2SDMDHeight" || key == "PUPDMDWindowHeight") {
                if (!settings.dmdHeight) assignIfInt(settings.dmdHeight, key);
            }
        }

        // 10.8.1 ini
        else if (currentSection == "Backglass") {
            if (key == "BackglassWndX") assignIfInt(settings.backglassX, key);
            else if (key == "BackglassWndY") assignIfInt(settings.backglassY, key);
            else if (key == "BackglassWidth") assignIfInt(settings.backglassWidth, key);
            else if (key == "BackglassHeight") assignIfInt(settings.backglassHeight, key);
        }

        else if (currentSection == "ScoreView") {
            if (key == "ScoreViewWndX") assignIfInt(settings.dmdX, key);
            else if (key == "ScoreViewWndY") assignIfInt(settings.dmdY, key);
            else if (key == "ScoreViewWidth") assignIfInt(settings.dmdWidth, key);
            else if (key == "ScoreViewHeight") assignIfInt(settings.dmdHeight, key);
        }

        else if (currentSection == "Topper") {
            if (key == "TopperWndX") assignIfInt(settings.topperX, key);
            else if (key == "TopperWndY") assignIfInt(settings.topperY, key);
            else if (key == "TopperWidth") assignIfInt(settings.topperWidth, key);
            else if (key == "TopperHeight") assignIfInt(settings.topperHeight, key);
        }

        else if (currentSection == "Plugin.B2S" || currentSection == "Plugin.B2SLegacy") {
            if (key == "ScoreviewDMDX") assignIfInt(settings.dmdX, key);
            else if (key == "ScoreviewDMDY") assignIfInt(settings.dmdY, key);
            else if (key == "ScoreviewDMDWidth") assignIfInt(settings.dmdWidth, key);
            else if (key == "ScoreviewDMDHeight") assignIfInt(settings.dmdHeight, key);

            else if (key == "BackglassDMDX") assignIfInt(settings.backglassX, key);
            else if (key == "BackglassDMDY") assignIfInt(settings.backglassY, key);
            else if (key == "BackglassDMDWidth") assignIfInt(settings.backglassWidth, key);
            else if (key == "BackglassDMDHeight") assignIfInt(settings.backglassHeight, key);
        }
    }

    file.close();
    return settings;
}
