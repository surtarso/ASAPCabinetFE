// Tarso Galvão Mar/2025

#include "config/config_gui.h"
#include "config/tooltips.h"
#include "config/config_loader.h"
#include "input/input_manager.h"
#include "imgui.h"
#include <fstream>
//#include <sstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>     // For toupper
#include <algorithm>  // For std::transform

// Define the global configChangesPending variable
bool configChangesPending = false;

IniEditor::IniEditor(const std::string& filename, bool& showFlag) 
    : iniFilename(filename), showFlag(showFlag) {
    loadIniFile(filename);
    initExplanations();
    if (!sections.empty()) {
        currentSection = sections[0];
    }
    // Initialize key capture state
    isCapturingKey_ = false;
    capturingKeyName_.clear();
    capturedKeyName_.clear();
    saveMessageTimer_ = 0.0f; // Initialize save message timer
}

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
    originalLines.clear();
    while (std::getline(file, line)) {
        originalLines.push_back(line);
    }
    file.close();

    // Parse the lines to populate iniData, sections, and line mappings
    std::string currentSectionName;
    size_t lineIndex = 0;
    iniData.clear();
    sections.clear();
    lineToKey.clear();
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
    hasChanges = false;
    std::cout << "Loaded config file: " << filename << std::endl;
}

void IniEditor::saveIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not write " << filename << std::endl;
        return;
    }

    std::cout << "Saving config to " << filename << ":" << std::endl;
    for (size_t i = 0; i < originalLines.size(); ++i) {
        if (lineToKey.find(i) != lineToKey.end()) {
            auto [section, key] = lineToKey[i];
            for (const auto& kv : iniData[section].keyValues) {
                if (kv.first == key && iniData[section].keyToLineIndex[key] == i) {
                    std::cout << key << " = " << kv.second << std::endl;
                    file << key << " = " << kv.second << "\n";
                    break;
                }
            }
        } else {
            file << originalLines[i] << "\n";
        }
    }
    file.close();
    std::cout << "Config saved to " << filename << std::endl;
}

void IniEditor::initExplanations() {
    explanations = Tooltips::getTooltips();
}

void IniEditor::drawGUI() {
    // Center the window on the screen
    float windowWidth = 800.0f;
    float windowHeight = 500.0f;
    ImGui::SetNextWindowPos(ImVec2((MAIN_WINDOW_WIDTH - windowWidth) / 2.0f, (MAIN_WINDOW_HEIGHT - windowHeight) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Once);
    ImGui::Begin("ASAPCabinetFE Configuration", &showFlag, ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowFocus();

    // Left column: Section listbox
    ImGui::BeginChild("SectionsPane", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
    if (ImGui::BeginListBox("##Sections", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y))) {
        for (const auto& section : sections) {
            bool is_selected = (currentSection == section);
            if (ImGui::Selectable(section.c_str(), is_selected)) {
                // Reset key capture state when switching sections
                if (currentSection != section) {
                    isCapturingKey_ = false;
                    capturingKeyName_.clear();
                    capturedKeyName_.clear();
                    std::cout << "Switched to section: " << section << ", reset key capture state" << std::endl;
                }
                currentSection = section;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
    ImGui::EndChild();

    // Right column: Key-value pairs
    ImGui::SameLine();
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true); // Adjusted height to fit content
    if (iniData.find(currentSection) != iniData.end()) {
        // Reset key capture state when first entering Keybinds section
        static bool firstRenderOfKeybinds = true;
        if (currentSection == "Keybinds" && firstRenderOfKeybinds) {
            if (isCapturingKey_) {
                std::cout << "Resetting unexpected key capture state on entering Keybinds section" << std::endl;
                isCapturingKey_ = false;
                capturingKeyName_.clear();
                capturedKeyName_.clear();
            }
            firstRenderOfKeybinds = false;
        } else if (currentSection != "Keybinds") {
            firstRenderOfKeybinds = true; // Reset flag when leaving Keybinds section
        }

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

            // Special handling for [Keybinds] section
            if (currentSection == "Keybinds") {
                // Map the keybinding name to its corresponding SDL_Keycode from config_loader
                SDL_Keycode keyCode = SDLK_UNKNOWN;
                if (kv.first == "PreviousTable") keyCode = KEY_PREVIOUS_TABLE;
                else if (kv.first == "NextTable") keyCode = KEY_NEXT_TABLE;
                else if (kv.first == "FastPrevTable") keyCode = KEY_FAST_PREV_TABLE;
                else if (kv.first == "FastNextTable") keyCode = KEY_FAST_NEXT_TABLE;
                else if (kv.first == "JumpNextLetter") keyCode = KEY_JUMP_NEXT_LETTER;
                else if (kv.first == "JumpPrevLetter") keyCode = KEY_JUMP_PREV_LETTER;
                else if (kv.first == "LaunchTable") keyCode = KEY_LAUNCH_TABLE;
                else if (kv.first == "ToggleConfig") keyCode = KEY_TOGGLE_CONFIG;
                else if (kv.first == "Quit") keyCode = KEY_QUIT;
                else if (kv.first == "ConfigSave") keyCode = KEY_CONFIG_SAVE;
                else if (kv.first == "ConfigClose") keyCode = KEY_CONFIG_CLOSE;
                else if (kv.first == "ScreenshotMode") keyCode = KEY_SCREENSHOT_MODE;
                else if (kv.first == "ScreenshotKey") keyCode = KEY_SCREENSHOT_KEY;
                else if (kv.first == "ScreenshotQuit") keyCode = KEY_SCREENSHOT_QUIT;

                // Get the human-readable name for the key
                const char* keyDisplayName = SDL_GetKeyName(keyCode);
                if (keyCode == SDLK_UNKNOWN) {
                    keyDisplayName = "Unknown Key";
                    std::cout << "Failed to map key for: " << kv.first << " (value: " << kv.second << ")" << std::endl;
                }

                // Display the current key
                ImGui::Text("%s", keyDisplayName);
                ImGui::SameLine(350); // Fixed position for the "Set Key" button to align vertically

                // "Set Key" button (changes to "Waiting..." while capturing)
                std::string buttonLabel = (isCapturingKey_ && capturingKeyName_ == kv.first) ? "Waiting...##" + kv.first : "Set Key##" + kv.first;
                if (ImGui::Button(buttonLabel.c_str())) {
                    if (!(isCapturingKey_ && capturingKeyName_ == kv.first)) { // Prevent re-triggering while already capturing
                        std::cout << "Set Key button clicked for: " << kv.first << std::endl;
                        isCapturingKey_ = true;
                        capturingKeyName_ = kv.first;
                        capturedKeyName_.clear();
                    }
                }
            } else {
                // Default text input for non-keybind sections
                char buf[256];
                std::strncpy(buf, kv.second.c_str(), sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                if (ImGui::InputText(("##" + kv.first).c_str(), buf, sizeof(buf))) {
                    kv.second = std::string(buf);
                    hasChanges = true;
                    std::cout << "Field modified: " << kv.first << " = " << kv.second << ", hasChanges set to true" << std::endl;
                }
            }
        }
    } else {
        ImGui::Text("No section data available.");
    }
    ImGui::EndChild();

    // Update save message timer
    if (saveMessageTimer_ > 0.0f) {
        saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
    }

    // Buttons at the bottom
    ImGui::BeginChild("ButtonPane", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 0.8f), false, ImGuiWindowFlags_NoScrollbar);
    // Align buttons to the left
    if (ImGui::Button("Save")) {
        saveIniFile(iniFilename);
        hasChanges = false;  // Reset hasChanges after saving
        configChangesPending = true;  // Set the flag to trigger a reload
        saveMessageTimer_ = 3.0f;  // Show "Saved successfully" for 3 seconds
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        loadIniFile(iniFilename);  // Reload the config file to discard changes
        hasChanges = false;  // Reset hasChanges
        // Reset key capture state when closing the GUI
        isCapturingKey_ = false;
        capturingKeyName_.clear();
        capturedKeyName_.clear();
        showFlag = false;  // Close the config GUI
        std::cout << "Config GUI closed via button, reset key capture state" << std::endl;
    }

    // Display "Saved successfully" text to the right of the "Close" button
    if (saveMessageTimer_ > 0.0f) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved successfully");
    }
    ImGui::EndChild();

    ImGui::End();  // Close the main window
}

void IniEditor::handleEvent(const SDL_Event& event) {
    // Handle key capture mode
    if (isCapturingKey_ && event.type == SDL_KEYDOWN) {
        SDL_Keycode keyCode = event.key.keysym.sym;
        if (keyCode == SDLK_ESCAPE) {
            // Cancel key capture
            std::cout << "Key capture cancelled" << std::endl;
            isCapturingKey_ = false;
            capturingKeyName_.clear();
            capturedKeyName_.clear();
        } else if (keyCode != SDLK_UNKNOWN) {
            // Capture the key
            const char* keyName = SDL_GetKeyName(keyCode);
            if (keyName && *keyName) {
                std::string sdlKeyName = std::string(keyName);
                // Convert to the format used in config.ini (strip SDLK_ prefix and convert to uppercase)
                if (sdlKeyName.substr(0, 5) == "SDLK_") {
                    sdlKeyName = sdlKeyName.substr(5);
                }
                std::transform(sdlKeyName.begin(), sdlKeyName.end(), sdlKeyName.begin(), ::toupper);
                capturedKeyName_ = sdlKeyName;
                std::cout << "Key captured: " << capturedKeyName_ << std::endl;

                // Update the keybinding in iniData and config_loader
                for (auto& keyVal : iniData[currentSection].keyValues) {
                    if (keyVal.first == capturingKeyName_) {
                        keyVal.second = capturedKeyName_;
                        hasChanges = true;
                        std::cout << "Keybinding modified: " << capturingKeyName_ << " = " << capturedKeyName_ << std::endl;

                        // Update the corresponding SDL_Keycode in config_loader
                        SDL_Keycode newKeyCode = SDL_GetKeyFromName(capturedKeyName_.c_str());
                        if (newKeyCode == SDLK_UNKNOWN) {
                            std::cout << "Failed to map captured key: " << capturedKeyName_ << std::endl;
                        } else {
                            if (capturingKeyName_ == "PreviousTable") KEY_PREVIOUS_TABLE = newKeyCode;
                            else if (capturingKeyName_ == "NextTable") KEY_NEXT_TABLE = newKeyCode;
                            else if (capturingKeyName_ == "FastPrevTable") KEY_FAST_PREV_TABLE = newKeyCode;
                            else if (capturingKeyName_ == "FastNextTable") KEY_FAST_NEXT_TABLE = newKeyCode;
                            else if (capturingKeyName_ == "JumpNextLetter") KEY_JUMP_NEXT_LETTER = newKeyCode;
                            else if (capturingKeyName_ == "JumpPrevLetter") KEY_JUMP_PREV_LETTER = newKeyCode;
                            else if (capturingKeyName_ == "LaunchTable") KEY_LAUNCH_TABLE = newKeyCode;
                            else if (capturingKeyName_ == "ToggleConfig") KEY_TOGGLE_CONFIG = newKeyCode;
                            else if (capturingKeyName_ == "Quit") KEY_QUIT = newKeyCode;
                            else if (capturingKeyName_ == "ConfigSave") KEY_CONFIG_SAVE = newKeyCode;
                            else if (capturingKeyName_ == "ConfigClose") KEY_CONFIG_CLOSE = newKeyCode;
                            else if (capturingKeyName_ == "ScreenshotMode") KEY_SCREENSHOT_MODE = newKeyCode;
                            else if (capturingKeyName_ == "ScreenshotKey") KEY_SCREENSHOT_KEY = newKeyCode;
                            else if (capturingKeyName_ == "ScreenshotQuit") KEY_SCREENSHOT_QUIT = newKeyCode;

                            const char* newKeyDisplayName = SDL_GetKeyName(newKeyCode);
                            std::cout << "Updated display for " << capturingKeyName_ << " to " << newKeyDisplayName << std::endl;
                        }
                        break;
                    }
                }
                isCapturingKey_ = false;
                capturingKeyName_.clear();
                capturedKeyName_.clear();
            }
        }
        return;  // Don't process other events while capturing a key
    }

    // Normal event handling (e.g., save/close keybinds)
    InputManager input;
    if (input.isConfigSave(event)) {
        saveIniFile(iniFilename);
        hasChanges = false;
        configChangesPending = true;
        saveMessageTimer_ = 3.0f;  // Show "Saved successfully" for 3 seconds
        std::cout << "Config saved to " << iniFilename << " via keybind" << std::endl;
    }
    if (input.isConfigClose(event)) {
        loadIniFile(iniFilename);  // Reload the config file to discard changes
        hasChanges = false;  // Reset hasChanges
        // Reset key capture state when closing the GUI
        isCapturingKey_ = false;
        capturingKeyName_.clear();
        capturedKeyName_.clear();
        showFlag = false;  // Close the config GUI
        std::cout << "Config GUI closed via keybind, reset key capture state" << std::endl;
    }
}