// Tarso Galvão Mar/2025

#include "config/config_gui.h"
#include "config/config_loader.h"
#include "input/input_manager.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
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
    explanations["TablesPath"] = "Specifies the absolute path to the folder containing VPX table files.\n\nIt must be a full path.\n(e.g., /home/user/tables/).\n\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["ExecutableCmd"] = "Defines the absolute path to the VPinballX executable.\n\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["StartArgs"] = "Optional command-line arguments to prepend to the executable.\n\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["EndArgs"] = "Optional arguments to append after the table file in the command.\n\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["TableImage"] = "Relative path to the table's preview image.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["BackglassImage"] = "Relative path to the backglass image.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["WheelImage"] = "Relative path to the wheel image for the table.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["DmdImage"] = "Relative path to the DMD or marquee image.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["TableVideo"] = "Relative path to the table preview video.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["BackglassVideo"] = "Relative path to the backglass video.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["DmdVideo"] = "Relative path to the DMD video.\nThese are relative to your table folder.\n/path/to/tables/<table_folder>/";
    explanations["MainMonitor"] = "Index of the monitor for the table playfield window.\nYou can use 'xrandr' to get yours.";
    explanations["MainWidth"] = "Width of the main window in pixels.\nThis should be relative to your playfield media width.";
    explanations["MainHeight"] = "Height of the main window in pixels.\nThis should be relative to your playfield media height.";
    explanations["SecondMonitor"] = "Index of the monitor for the backglass/DMD window.\nYou can use 'xrandr' to get yours.";
    explanations["SecondWidth"] = "Width of the secondary window in pixels.\nThis should be relative to your backglass + DMD media width.";
    explanations["SecondHeight"] = "Height of the secondary window in pixels.\nThis should be relative to your backglass + DMD media height.";
    explanations["Path"] = "Absolute path to the font file used in the UI.";
    explanations["Size"] = "Font size in points for table title text rendering.";
    explanations["WheelImageSize"] = "Size of the wheel image in pixels.\nThis considers a square image.";
    explanations["WheelImageMargin"] = "Margin around the wheel image in pixels.";
    explanations["BackglassWidth"] = "Width of the backglass media in pixels.";
    explanations["BackglassHeight"] = "Height of the backglass media in pixels.";
    explanations["DmdWidth"] = "Width of the DMD media in pixels.";
    explanations["DmdHeight"] = "Height of the DMD media in pixels.";
    explanations["FadeTargetAlpha"] = "Goes from 0 (transparent) to 255.\nUse 128 for ~50 percent alpha.";
    explanations["FadeDurationMs"] = "Table images switch transition time in ms\nSet to 1 if using videos.";
}

void IniEditor::drawGUI() {
    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_Once); // Once to allow resizing
    ImGui::Begin("ASAPCabinetFE Configuration", &showFlag);
    ImGui::SetWindowFocus();

    // Left column: Section listbox
    ImGui::BeginChild("SectionsPane", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
    if (ImGui::BeginListBox("##Sections", ImVec2(-FLT_MIN, 0))) { // Auto-height
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