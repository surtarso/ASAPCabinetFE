#include "config_file_handler.h"
#include "utils/logging.h"
#include <fstream>
#include <filesystem>

ConfigFileHandler::ConfigFileHandler(const std::string& configPath) : configPath_(configPath) {}

std::map<std::string, SettingsSection> ConfigFileHandler::readConfig(std::vector<std::string>& originalLines) {
    std::ifstream file(configPath_);
    std::map<std::string, SettingsSection> iniData;
    originalLines.clear();

    if (!file.is_open()) {
        LOG_INFO("ConfigFileHandler: Could not open " << configPath_ << ".");
        return iniData;
    }

    std::string line;
    while (std::getline(file, line)) {
        originalLines.push_back(line);
    }
    file.close();

    std::string currentSection;
    size_t lineIndex = 0;
    for (const auto& line : originalLines) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == ';') {
            lineIndex++;
            continue;
        }
        std::string trimmed = line.substr(start);
        if (trimmed[0] == '[' && trimmed.back() == ']') {
            currentSection = trimmed.substr(1, trimmed.size() - 2);
            iniData[currentSection] = SettingsSection();
        } else if (!currentSection.empty()) {
            size_t eq = trimmed.find('=');
            if (eq != std::string::npos) {
                std::string key = trimmed.substr(0, eq);
                std::string value = trimmed.substr(eq + 1);
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                if (key == "JumpNextLetter" && value == "Slash") value = "/";
                iniData[currentSection].keyValues.emplace_back(key, value);
                iniData[currentSection].keyToLineIndex[key] = lineIndex;
            }
        }
        lineIndex++;
    }

    //LOG_DEBUG("ConfigFileHandler: Successfully read config from " << configPath_);
    return iniData;
}

void ConfigFileHandler::writeConfig(const std::map<std::string, SettingsSection>& iniData) {
    // Read original lines to preserve comments and formatting
    std::vector<std::string> originalLines;
    readConfig(originalLines); // Re-read to ensure we have the latest file content

    // Create a map of section and key to line index for updates
    std::map<std::string, std::map<std::string, size_t>> sectionKeyToLineIndex;
    for (const auto& [section, configSection] : iniData) {
        for (const auto& [key, lineIndex] : configSection.keyToLineIndex) {
            sectionKeyToLineIndex[section][key] = lineIndex;
        }
    }

    // Update original lines with new values
    for (const auto& [section, configSection] : iniData) {
        for (const auto& [key, value] : configSection.keyValues) {
            if (sectionKeyToLineIndex[section].count(key)) {
                size_t lineIndex = sectionKeyToLineIndex[section][key];
                if (lineIndex < originalLines.size()) {
                    // Update the line, preserving leading/trailing whitespace
                    std::string& line = originalLines[lineIndex];
                    size_t eqPos = line.find('=');
                    if (eqPos != std::string::npos) {
                        std::string indent = line.substr(0, line.find_first_not_of(" \t"));
                        line = indent + key + "=" + value;
                    }
                }
            } else {
                // Key doesn't exist in original file, append to section
                // Find the last line of the section to append after
                size_t lastLineIndex = 0;
                bool sectionFound = false;
                for (size_t i = 0; i < originalLines.size(); ++i) {
                    std::string trimmed = originalLines[i];
                    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                    if (trimmed == "[" + section + "]") {
                        sectionFound = true;
                        lastLineIndex = i;
                    } else if (sectionFound && !trimmed.empty() && trimmed[0] == '[') {
                        break; // Next section starts
                    } else if (sectionFound) {
                        lastLineIndex = i;
                    }
                }
                if (sectionFound) {
                    originalLines.insert(originalLines.begin() + lastLineIndex + 1, key + "=" + value);
                } else {
                    // Section doesn't exist, append new section
                    originalLines.push_back("[" + section + "]");
                    originalLines.push_back(key + "=" + value);
                }
            }
        }
    }

    // Ensure the config directory exists
    std::filesystem::path configDir = std::filesystem::path(configPath_).parent_path();
    if (!configDir.empty() && !std::filesystem::exists(configDir)) {
        try {
            std::filesystem::create_directories(configDir);
            LOG_DEBUG("ConfigFileHandler: Created directory " << configDir);
        } catch (const std::exception& e) {
            LOG_ERROR("ConfigFileHandler: Failed to create directory " << configDir << ": " << e.what());
            return;
        }
    }

    // Write updated lines to file
    std::ofstream file(configPath_);
    if (!file.is_open()) {
        LOG_ERROR("ConfigFileHandler: Could not write " << configPath_ << ": Permission denied or invalid path");
        return;
    }

    for (const auto& line : originalLines) {
        file << line << "\n";
    }

    file.close();
    LOG_DEBUG("ConfigFileHandler: Successfully wrote config to " << configPath_);
}