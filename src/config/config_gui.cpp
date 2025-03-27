// Tarso Galvão Mar/2025

#include "config/config_gui.h"
#include "config/tooltips.h"
#include "config/config_manager.h"
#include "utils/logging.h"
#include "table/asset_manager.h"
#include "table/table_manager.h"
#include "imgui.h"
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>    // For toupper
#include <algorithm> // For std::transform
#include <sstream>   // For std::stringstream

namespace ConfigGuiUtils {
    void DrawSectionsPane(const std::vector<std::string>& sections, std::string& currentSection, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName) {
        ImGui::BeginChild("SectionsPane", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
        if (ImGui::BeginListBox("##Sections", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y))) {
            for (const auto& section : sections) {
                bool is_selected = (currentSection == section);
                if (ImGui::Selectable(section.c_str(), is_selected)) {
                    if (currentSection != section) {
                        isCapturingKey = false;
                        capturingKeyName.clear();
                        capturedKeyName.clear();
                        LOG_DEBUG("Switched to section: " << section << ", reset key capture state");
                    }
                    currentSection = section;
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }
        ImGui::EndChild();
    }

    void DrawKeyValuesPane(ConfigSection& section, IKeybindProvider* keybindProvider, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, const std::string& currentSection, std::map<std::string, bool>& showPicker, const std::unordered_map<std::string, std::string>& explanations, bool& hasChanges) {
        ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        static bool firstRenderOfKeybinds = true;
        if (currentSection == "Keybinds" && firstRenderOfKeybinds) {
            if (isCapturingKey) {
                LOG_DEBUG("Resetting unexpected key capture state on entering Keybinds section");
                isCapturingKey = false;
                capturingKeyName.clear();
                capturedKeyName.clear();
            }
            firstRenderOfKeybinds = false;
        } else if (currentSection != "Keybinds") {
            firstRenderOfKeybinds = true;
        }

        for (auto& kv : section.keyValues) {
            ImGui::Text("%s", kv.first.c_str());
            ImGui::SameLine(150);
            if (currentSection == "Keybinds") {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered()) {
                    std::string tooltip = keybindProvider->getTooltip(kv.first);
                    if (!tooltip.empty()) {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                        ImGui::TextWrapped("%s", tooltip.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }
            } else if (explanations.find(kv.first) != explanations.end()) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                    std::string tooltip = explanations.at(kv.first);
                    ImGui::TextWrapped("%s", tooltip.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::SameLine(200);

            if (currentSection == "TitleDisplay" && (kv.first == "FontColor" || kv.first == "FontBgColor")) {
                float color[4];
                ParseColorString(kv.second, color);

                ImGui::ColorButton(("##ColorButton_" + kv.first).c_str(), ImVec4(color[0], color[1], color[2], color[3]));
                ImGui::SameLine();
                if (ImGui::Button(("Pick##" + kv.first).c_str())) {
                    showPicker[kv.first] = !showPicker[kv.first];
                }
                if (showPicker[kv.first]) {
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::BeginChild(("##ColorPickerChild_" + kv.first).c_str(), ImVec2(300, 250), true);
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                    if (ImGui::ColorPicker4(("##ColorPicker_" + kv.first).c_str(), color, ImGuiColorEditFlags_AlphaBar)) {
                        kv.second = ColorToString(color);
                        hasChanges = true;
                    }
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                    ImGui::EndGroup();
                }
            } else if (currentSection == "Keybinds") {
                SDL_Keycode keyCode = keybindProvider->getKey(kv.first);
                LOG_DEBUG("Displaying key for " << kv.first << ", keycode: " << keyCode);
                const char* keyDisplayName = SDL_GetKeyName(keyCode);
                if (keyDisplayName == nullptr || std::strcmp(keyDisplayName, "") == 0 || std::strcmp(keyDisplayName, "Unknown Key") == 0) {
                    keyDisplayName = kv.second.c_str();
                    LOG_DEBUG("SDL_GetKeyName failed for " << kv.first << ", falling back to ini value: " << keyDisplayName);
                }

                ImGui::Text("%s", keyDisplayName);
                ImGui::SameLine(350);
                std::string buttonLabel = (isCapturingKey && capturingKeyName == kv.first) ? "Waiting...##" + kv.first : "Set Key##" + kv.first;
                if (ImGui::Button(buttonLabel.c_str())) {
                    if (!(isCapturingKey && capturingKeyName == kv.first)) {
                        isCapturingKey = true;
                        capturingKeyName = kv.first;
                        capturedKeyName.clear();
                    }
                }
            } else {
                char buf[256];
                std::strncpy(buf, kv.second.c_str(), sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                if (ImGui::InputText(("##" + kv.first).c_str(), buf, sizeof(buf))) {
                    kv.second = std::string(buf);
                    hasChanges = true;
                }
            }
        }
        ImGui::EndChild();
    }

    void DrawButtonPane(bool& showFlag, const std::string& iniFilename, ConfigManager* configManager, AssetManager* assets, size_t* currentIndex, std::vector<Table>* tables, bool& hasChanges, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, float& saveMessageTimer, const std::map<std::string, ConfigSection>& iniData) {
        ImGui::BeginChild("ButtonPane", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 0.8f), false, ImGuiWindowFlags_NoScrollbar);
        if (ImGui::Button("Save")) {
            std::ofstream file(iniFilename);
            if (!file.is_open()) {
                LOG_DEBUG("Could not write " << iniFilename);
            } else {
                LOG_DEBUG("Saving config to " << iniFilename << ":");
                for (const auto& [section, configSection] : iniData) {
                    file << "[" << section << "]\n";
                    for (const auto& kv : configSection.keyValues) {
                        LOG_DEBUG(kv.first << " = " << kv.second);
                        file << kv.first << " = " << kv.second << "\n";
                    }
                    file << "\n";
                }
                file.close();
                LOG_DEBUG("Config saved to " << iniFilename);

                configManager->loadConfig();
                if (configManager && assets && currentIndex && tables) {
                    configManager->notifyConfigChanged(*assets, *currentIndex, *tables);
                }
                hasChanges = false;
                saveMessageTimer = 3.0f;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            configManager->loadConfig();
            hasChanges = false;
            isCapturingKey = false;
            capturingKeyName.clear();
            capturedKeyName.clear();
            showFlag = false;
        }
        if (saveMessageTimer > 0.0f) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved successfully");
        }
        ImGui::EndChild();
    }

    void ParseColorString(const std::string& colorStr, float color[4]) {
        std::stringstream ss(colorStr);
        std::string token;
        int values[4] = {255, 255, 255, 255};
        int i = 0;

        while (std::getline(ss, token, ',') && i < 4) {
            try {
                values[i] = std::stoi(token);
                values[i] = std::max(0, std::min(255, values[i]));
            } catch (...) {
                LOG_DEBUG("Invalid color component in: " + colorStr + ", using default");
                break;
            }
            i++;
        }

        for (int j = 0; j < 4; j++) {
            color[j] = values[j] / 255.0f;
        }
    }

    std::string ColorToString(const float color[4]) {
        int r = static_cast<int>(color[0] * 255.0f);
        int g = static_cast<int>(color[1] * 255.0f);
        int b = static_cast<int>(color[2] * 255.0f);
        int a = static_cast<int>(color[3] * 255.0f);
        std::stringstream ss;
        ss << r << "," << g << "," << b << "," << a;
        return ss.str();
    }
}

// InitialConfigEditor implementation
InitialConfigEditor::InitialConfigEditor(const std::string& filename, bool& showFlag, ConfigManager* configManager,
                                         IKeybindProvider* keybindProvider)
    : iniFilename_(filename),
      showFlag_(showFlag),
      configManager_(configManager),
      keybindProvider_(keybindProvider),
      tempSettings_(configManager ? configManager->getSettings() : Settings{}) {
    loadIniFile(filename);
    initExplanations();
    if (!sections_.empty()) {
        currentSection_ = sections_[0];
    }
}

void InitialConfigEditor::loadIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not open " << filename);
        return;
    }

    std::string line;
    originalLines_.clear();
    while (std::getline(file, line)) {
        originalLines_.push_back(line);
    }
    file.close();

    std::string currentSectionName;
    size_t lineIndex = 0;
    iniData_.clear();
    sections_.clear();
    lineToKey_.clear();
    for (const auto& line : originalLines_) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            lineIndex++;
            continue;
        }
        std::string trimmedLine = line.substr(start);

        if (trimmedLine.empty() || trimmedLine[0] == ';') {
            lineIndex++;
            continue;
        }

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
            currentSectionName = trimmedLine.substr(1, trimmedLine.size() - 2);
            sections_.push_back(currentSectionName);
            iniData_[currentSectionName] = ConfigSection();
        } else if (!currentSectionName.empty()) {
            size_t pos = trimmedLine.find('=');
            if (pos != std::string::npos) {
                std::string key = trimmedLine.substr(0, pos);
                std::string value = trimmedLine.substr(pos + 1);
                size_t endKey = key.find_last_not_of(" \t");
                if (endKey != std::string::npos)
                    key = key.substr(0, endKey + 1);
                size_t startValue = value.find_first_not_of(" \t");
                if (startValue != std::string::npos)
                    value = value.substr(startValue);
                iniData_[currentSectionName].keyValues.emplace_back(key, value);
                iniData_[currentSectionName].keyToLineIndex[key] = lineIndex;
                lineToKey_[lineIndex] = {currentSectionName, key};
            }
        }
        lineIndex++;
    }
    hasChanges_ = false;
    LOG_DEBUG("Loaded config file: " << filename);
}

void InitialConfigEditor::saveIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not write " << filename);
        return;
    }

    LOG_DEBUG("Saving config to " << filename << ":");
    for (size_t i = 0; i < originalLines_.size(); ++i) {
        if (lineToKey_.find(i) != lineToKey_.end()) {
            auto [section, key] = lineToKey_[i];
            for (const auto& kv : iniData_[section].keyValues) {
                if (kv.first == key && iniData_[section].keyToLineIndex[key] == i) {
                    LOG_DEBUG(key << " = " << kv.second);
                    file << key << " = " << kv.second << "\n";
                    break;
                }
            }
        } else {
            file << originalLines_[i] << "\n";
        }
    }
    file.close();
    LOG_DEBUG("Config saved to " << filename);

    configManager_->loadConfig();
    hasChanges_ = false;
    saveMessageTimer_ = 3.0f;
}

void InitialConfigEditor::initExplanations() {
    explanations_ = Tooltips::getTooltips();
}

void InitialConfigEditor::drawGUI() {
    LOG_DEBUG("drawGUI called, showConfig_: " << (showFlag_ ? 1 : 0));
    if (!showFlag_) {
        LOG_DEBUG("Exiting drawGUI due to showFlag false");
        return;
    }
    LOG_DEBUG("Drawing config GUI");
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    LOG_DEBUG("ImGui context in drawGUI: " << (void*)ctx);

    if (fillParentWindow_) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::Begin("ASAPCabinetFE Configuration", &showFlag_, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    } else {
        float windowWidth = 800.0f;
        float windowHeight = 500.0f;
        ImGui::SetNextWindowPos(ImVec2((tempSettings_.mainWindowWidth - windowWidth) / 2.0f, (tempSettings_.mainWindowHeight - windowHeight) / 2.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Once);
        ImGui::Begin("ASAPCabinetFE Configuration", &showFlag_, ImGuiWindowFlags_NoTitleBar);
    }

    ImGui::SetWindowFocus();

    ConfigGuiUtils::DrawSectionsPane(sections_, currentSection_, isCapturingKey_, capturingKeyName_, capturedKeyName_);

    ImGui::SameLine();
    if (iniData_.find(currentSection_) != iniData_.end()) {
        ConfigGuiUtils::DrawKeyValuesPane(iniData_[currentSection_], keybindProvider_, isCapturingKey_, capturingKeyName_, capturedKeyName_, currentSection_, showPicker_, explanations_, hasChanges_);
    } else {
        ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        ImGui::Text("No section data available.");
        ImGui::EndChild();
    }

    if (saveMessageTimer_ > 0.0f) {
        saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
    }

    ConfigGuiUtils::DrawButtonPane(showFlag_, iniFilename_, configManager_, nullptr, nullptr, nullptr, hasChanges_, isCapturingKey_, capturingKeyName_, capturedKeyName_, saveMessageTimer_, iniData_);

    ImGui::End();
}

void InitialConfigEditor::handleEvent(const SDL_Event& event) {
    if (isCapturingKey_ && event.type == SDL_KEYDOWN) {
        SDL_Keycode keyCode = event.key.keysym.sym;
        if (keyCode == SDLK_ESCAPE) {
            isCapturingKey_ = false;
            capturingKeyName_.clear();
            capturedKeyName_.clear();
        } else if (keyCode != SDLK_UNKNOWN) {
            const char* keyName = SDL_GetKeyName(keyCode);
            if (keyName && *keyName) {
                std::string sdlKeyName = std::string(keyName);
                if (sdlKeyName.substr(0, 5) == "SDLK_") {
                    sdlKeyName = sdlKeyName.substr(5);
                }
                std::transform(sdlKeyName.begin(), sdlKeyName.end(), sdlKeyName.begin(), ::toupper);
                capturedKeyName_ = sdlKeyName;

                for (auto& keyVal : iniData_[currentSection_].keyValues) {
                    if (keyVal.first == capturingKeyName_) {
                        keyVal.second = capturedKeyName_;
                        hasChanges_ = true;
                        keybindProvider_->setKey(capturingKeyName_, keyCode);
                        break;
                    }
                }
                isCapturingKey_ = false;
                capturingKeyName_.clear();
                capturedKeyName_.clear();
            }
        }
    }
}

// InGameConfigEditor implementation
InGameConfigEditor::InGameConfigEditor(const std::string& filename, bool& showFlag, ConfigManager* configManager,
                                       IKeybindProvider* keybindProvider, AssetManager* assets,
                                       size_t* currentIndex, std::vector<Table>* tables)
    : iniFilename_(filename),
      showFlag_(showFlag),
      configManager_(configManager),
      keybindProvider_(keybindProvider),
      assets_(assets),
      currentIndex_(currentIndex),
      tables_(tables),
      tempSettings_(configManager ? configManager->getSettings() : Settings{}) {
    loadIniFile(filename);
    initExplanations();
    if (!sections_.empty()) {
        currentSection_ = sections_[0];
    }
    sections_.push_back("Table Overrides"); // Add a custom section for table overrides
}

void InGameConfigEditor::loadIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not open " << filename);
        return;
    }

    std::string line;
    originalLines_.clear();
    while (std::getline(file, line)) {
        originalLines_.push_back(line);
    }
    file.close();

    std::string currentSectionName;
    size_t lineIndex = 0;
    iniData_.clear();
    sections_.clear();
    lineToKey_.clear();
    for (const auto& line : originalLines_) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            lineIndex++;
            continue;
        }
        std::string trimmedLine = line.substr(start);

        if (trimmedLine.empty() || trimmedLine[0] == ';') {
            lineIndex++;
            continue;
        }

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
            currentSectionName = trimmedLine.substr(1, trimmedLine.size() - 2);
            sections_.push_back(currentSectionName);
            iniData_[currentSectionName] = ConfigSection();
        } else if (!currentSectionName.empty()) {
            size_t pos = trimmedLine.find('=');
            if (pos != std::string::npos) {
                std::string key = trimmedLine.substr(0, pos);
                std::string value = trimmedLine.substr(pos + 1);
                size_t endKey = key.find_last_not_of(" \t");
                if (endKey != std::string::npos)
                    key = key.substr(0, endKey + 1);
                size_t startValue = value.find_first_not_of(" \t");
                if (startValue != std::string::npos)
                    value = value.substr(startValue);
                iniData_[currentSectionName].keyValues.emplace_back(key, value);
                iniData_[currentSectionName].keyToLineIndex[key] = lineIndex;
                lineToKey_[lineIndex] = {currentSectionName, key};
            }
        }
        lineIndex++;
    }
    hasChanges_ = false;
    LOG_DEBUG("Loaded config file: " << filename);
}

void InGameConfigEditor::saveIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not write " << filename);
        return;
    }

    LOG_DEBUG("Saving config to " << filename << ":");
    for (size_t i = 0; i < originalLines_.size(); ++i) {
        if (lineToKey_.find(i) != lineToKey_.end()) {
            auto [section, key] = lineToKey_[i];
            for (const auto& kv : iniData_[section].keyValues) {
                if (kv.first == key && iniData_[section].keyToLineIndex[key] == i) {
                    LOG_DEBUG(key << " = " << kv.second);
                    file << key << " = " << kv.second << "\n";
                    break;
                }
            }
        } else {
            file << originalLines_[i] << "\n";
        }
    }
    file.close();
    LOG_DEBUG("Config saved to " << filename);

    configManager_->loadConfig();
    configManager_->notifyConfigChanged(*assets_, *currentIndex_, *tables_);
    hasChanges_ = false;
    saveMessageTimer_ = 3.0f;
}

void InGameConfigEditor::initExplanations() {
    explanations_ = Tooltips::getTooltips();
}

void InGameConfigEditor::drawGUI() {
    LOG_DEBUG("drawGUI called, showConfig_: " << (showFlag_ ? 1 : 0));
    if (!showFlag_) {
        LOG_DEBUG("Exiting drawGUI due to showFlag false");
        return;
    }
    LOG_DEBUG("Drawing config GUI");
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    LOG_DEBUG("ImGui context in drawGUI: " << (void*)ctx);

    float windowWidth = 800.0f;
    float windowHeight = 500.0f;
    ImGui::SetNextWindowPos(ImVec2((tempSettings_.mainWindowWidth - windowWidth) / 2.0f, (tempSettings_.mainWindowHeight - windowHeight) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Once);
    ImGui::Begin("ASAPCabinetFE Configuration", &showFlag_, ImGuiWindowFlags_NoTitleBar);

    ImGui::SetWindowFocus();

    ConfigGuiUtils::DrawSectionsPane(sections_, currentSection_, isCapturingKey_, capturingKeyName_, capturedKeyName_);

    ImGui::SameLine();
    if (currentSection_ == "Table Overrides") {
        drawTableOverridesGUI();
    } else if (iniData_.find(currentSection_) != iniData_.end()) {
        ConfigGuiUtils::DrawKeyValuesPane(iniData_[currentSection_], keybindProvider_, isCapturingKey_, capturingKeyName_, capturedKeyName_, currentSection_, showPicker_, explanations_, hasChanges_);
    } else {
        ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        ImGui::Text("No section data available.");
        ImGui::EndChild();
    }

    if (saveMessageTimer_ > 0.0f) {
        saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
    }

    ConfigGuiUtils::DrawButtonPane(showFlag_, iniFilename_, configManager_, assets_, currentIndex_, tables_, hasChanges_, isCapturingKey_, capturingKeyName_, capturedKeyName_, saveMessageTimer_, iniData_);

    ImGui::End();
}

void InGameConfigEditor::drawTableOverridesGUI() {
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
    if (!tables_ || !currentIndex_ || tables_->empty()) {
        ImGui::Text("No tables available.");
        ImGui::EndChild();
        return;
    }

    ImGui::Text("Table Overrides for: %s", (*tables_)[*currentIndex_].tableName.c_str());

    Table& table = (*tables_)[*currentIndex_];
    char buf[256];

    std::strncpy(buf, table.tableImage.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Playfield Image", buf, sizeof(buf))) {
        table.tableImage = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.backglassImage.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Backglass Image", buf, sizeof(buf))) {
        table.backglassImage = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.dmdImage.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("DMD Image", buf, sizeof(buf))) {
        table.dmdImage = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.tableVideo.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Playfield Video", buf, sizeof(buf))) {
        table.tableVideo = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.backglassVideo.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Backglass Video", buf, sizeof(buf))) {
        table.backglassVideo = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.dmdVideo.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("DMD Video", buf, sizeof(buf))) {
        table.dmdVideo = std::string(buf);
        hasChanges_ = true;
    }

    ImGui::EndChild();
}

void InGameConfigEditor::handleEvent(const SDL_Event& event) {
    if (isCapturingKey_ && event.type == SDL_KEYDOWN) {
        SDL_Keycode keyCode = event.key.keysym.sym;
        if (keyCode == SDLK_ESCAPE) {
            isCapturingKey_ = false;
            capturingKeyName_.clear();
            capturedKeyName_.clear();
        } else if (keyCode != SDLK_UNKNOWN) {
            const char* keyName = SDL_GetKeyName(keyCode);
            if (keyName && *keyName) {
                std::string sdlKeyName = std::string(keyName);
                if (sdlKeyName.substr(0, 5) == "SDLK_") {
                    sdlKeyName = sdlKeyName.substr(5);
                }
                std::transform(sdlKeyName.begin(), sdlKeyName.end(), sdlKeyName.begin(), ::toupper);
                capturedKeyName_ = sdlKeyName;

                for (auto& keyVal : iniData_[currentSection_].keyValues) {
                    if (keyVal.first == capturingKeyName_) {
                        keyVal.second = capturedKeyName_;
                        hasChanges_ = true;
                        keybindProvider_->setKey(capturingKeyName_, keyCode);
                        break;
                    }
                }
                isCapturingKey_ = false;
                capturingKeyName_.clear();
                capturedKeyName_.clear();
            }
        }
    }
}