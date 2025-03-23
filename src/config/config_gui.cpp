// Tarso Galvão Mar/2025

#include "config/config_gui.h"
#include "config/tooltips.h"
#include "config/config_loader.h"
#include "input/input_manager.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>

// Constructor
IniEditor::IniEditor(const std::string& filename, bool& showFlag) 
    : iniFilename(filename), showFlag(showFlag) {
    loadIniFile(filename);
    initExplanations();
    if (!sections.empty()) {
        currentSection = sections[0];
    }
}

// Destructor
IniEditor::~IniEditor() {
    // No dynamic allocations to clean up
}

void IniEditor::loadIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open " << filename << std::endl;
        return;
    }

    // Read all lines into originalLines
    std::string line;
    while (std::getline(file, line)) {
        originalLines.push_back(line);
    }
    file.close();

    // Parse the lines to populate iniData, sections, and line mappings
    std::string currentSectionName;
    size_t lineIndex = 0;
    for (const auto& line : originalLines) {
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            lineIndex++;
            continue;
        }
        std::string trimmedLine = line.substr(start);

        // Skip empty lines or comments (but they’re preserved in originalLines)
        if (trimmedLine.empty() || trimmedLine[0] == ';') {
            lineIndex++;
            continue;
        }

        // Check for section header
        if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
            currentSectionName = trimmedLine.substr(1, trimmedLine.size() - 2);
            sections.push_back(currentSectionName);
            iniData[currentSectionName] = ConfigSection();
        } else if (!currentSectionName.empty()) {
            // Parse key=value pairs
            size_t pos = trimmedLine.find('=');
            if (pos != std::string::npos) {
                std::string key = trimmedLine.substr(0, pos);
                std::string value = trimmedLine.substr(pos + 1);
                // Trim trailing whitespace from key
                size_t endKey = key.find_last_not_of(" \t");
                if (endKey != std::string::npos)
                    key = key.substr(0, endKey + 1);
                // Trim leading whitespace from value
                size_t startValue = value.find_first_not_of(" \t");
                if (startValue != std::string::npos)
                    value = value.substr(startValue);
                // Append to vector to preserve order
                iniData[currentSectionName].keyValues.emplace_back(key, value);
                // Record the line index for this key
                iniData[currentSectionName].keyToLineIndex[key] = lineIndex;
                lineToKey[lineIndex] = {currentSectionName, key};
            }
        }
        lineIndex++;
    }
    // Post-load debug
    // std::cout << "Total sections loaded: " << sections.size() << std::endl;
    // for (const auto& sec : sections) {
    //     std::cout << "Section: " << sec << ", Keys: " << iniData[sec].keyValues.size() << std::endl;
    // }
}

void IniEditor::saveIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not write " << filename << std::endl;
        return;
    }

    for (size_t i = 0; i < originalLines.size(); ++i) {
        if (lineToKey.find(i) != lineToKey.end()) {
            auto [section, key] = lineToKey[i];
            for (const auto& kv : iniData[section].keyValues) {
                if (kv.first == key && iniData[section].keyToLineIndex[key] == i) {
                    file << key << " = " << kv.second << "\n";
                    break;
                }
            }
        } else {
            file << originalLines[i] << "\n";
        }
    }
    file.close();
}

void IniEditor::initExplanations() {
    explanations = Tooltips::getTooltips();  // Qualified with Tooltips::
}

void IniEditor::drawGUI() {
    // Center the window on the screen
    float windowWidth = 800.0f;
    float windowHeight = 500.0f;
    ImGui::SetNextWindowPos(ImVec2((MAIN_WINDOW_WIDTH - windowWidth) / 2.0f, (MAIN_WINDOW_HEIGHT - windowHeight) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Once); // Keep initial size, allow resizing
    ImGui::Begin("ASAPCabinetFE Configuration", &showFlag, ImGuiWindowFlags_NoTitleBar); // No title bar
    ImGui::SetWindowFocus();

    // Left column: Section listbox, stretch to available height
    ImGui::BeginChild("SectionsPane", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
    if (ImGui::BeginListBox("##Sections", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y))) { // Full available height
        for (const auto& section : sections) {
            bool is_selected = (currentSection == section);
            if (ImGui::Selectable(section.c_str(), is_selected)) {
                currentSection = section;
                // std::cout << "User selected section: " << section << std::endl;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
    ImGui::EndChild();

    // Right column: Key-value pairs
    ImGui::SameLine();
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
    if (iniData.find(currentSection) != iniData.end()) {
        for (auto& kv : iniData[currentSection].keyValues) {
            ImGui::Text("%s", kv.first.c_str());
            ImGui::SameLine(150);
            if (explanations.find(kv.first) != explanations.end()) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                    ImGui::TextWrapped("%s", explanations[kv.first].c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::SameLine(200);
            char buf[256];
            std::strncpy(buf, kv.second.c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText(("##" + kv.first).c_str(), buf, sizeof(buf))) {
                kv.second = std::string(buf);
            }
        }
    } else {
        ImGui::Text("No section data available.");
    }
    ImGui::EndChild();

    // Buttons at the bottom
    if (ImGui::Button("Save")) {
        saveIniFile(iniFilename);
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        showFlag = false;
    }

    ImGui::End();
}

void IniEditor::handleEvent(const SDL_Event& event) {
    InputManager input; // Use InputManager
    if (input.isConfigSave(event)) {
        saveIniFile(iniFilename);
        std::cout << "Config saved to " << iniFilename << std::endl;
    }
    if (input.isConfigClose(event)) {
        showFlag = false;
    }
}