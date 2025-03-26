// Tarso Galvão Mar/2025

#include "config/config_gui.h"
#include "config/tooltips.h"
#include "config/config_manager.h"
#include "input/input_manager.h"
#include "utils/logging.h"
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

IniEditor::IniEditor(const std::string &filename, bool &showFlag, ConfigManager *configManager,
                     AssetManager *assets, size_t *currentIndex, std::vector<Table> *tables)
    : iniFilename(filename),
      showFlag(showFlag),
      configManager_(configManager),
      assets_(assets),
      currentIndex_(currentIndex),
      tables_(tables),
      tempSettings_(configManager ? configManager->getSettings() : Settings{}),
      originalLines(),
      iniData(),
      sections(),
      currentSection(),
      lineToKey(),
      explanations(),
      hasChanges(false),
      isCapturingKey_(false),
      capturingKeyName_(),
      capturedKeyName_(),
      saveMessageTimer_(0.0f)
{
    loadIniFile(filename);
    initExplanations();
    if (!sections.empty())
    {
        currentSection = sections[0];
    }
}

IniEditor::~IniEditor()
{
    // No dynamic allocations to clean up
}

void IniEditor::loadIniFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LOG_DEBUG("Could not open " << filename);
        return;
    }

    std::string line;
    originalLines.clear();
    while (std::getline(file, line))
    {
        originalLines.push_back(line);
    }
    file.close();

    std::string currentSectionName;
    size_t lineIndex = 0;
    iniData.clear();
    sections.clear();
    lineToKey.clear();
    for (const auto &line : originalLines)
    {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            lineIndex++;
            continue;
        }
        std::string trimmedLine = line.substr(start);

        if (trimmedLine.empty() || trimmedLine[0] == ';')
        {
            lineIndex++;
            continue;
        }

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']')
        {
            currentSectionName = trimmedLine.substr(1, trimmedLine.size() - 2);
            sections.push_back(currentSectionName);
            iniData[currentSectionName] = ConfigSection();
        }
        else if (!currentSectionName.empty())
        {
            size_t pos = trimmedLine.find('=');
            if (pos != std::string::npos)
            {
                std::string key = trimmedLine.substr(0, pos);
                std::string value = trimmedLine.substr(pos + 1);
                size_t endKey = key.find_last_not_of(" \t");
                if (endKey != std::string::npos)
                    key = key.substr(0, endKey + 1);
                size_t startValue = value.find_first_not_of(" \t");
                if (startValue != std::string::npos)
                    value = value.substr(startValue);
                iniData[currentSectionName].keyValues.emplace_back(key, value);
                iniData[currentSectionName].keyToLineIndex[key] = lineIndex;
                lineToKey[lineIndex] = {currentSectionName, key};
            }
        }
        lineIndex++;
    }
    hasChanges = false;
    LOG_DEBUG("Loaded config file: " << filename);
}

void IniEditor::saveIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_DEBUG("Could not write " << filename);
        return;
    }

    LOG_DEBUG("Saving config to " << filename << ":");
    for (size_t i = 0; i < originalLines.size(); ++i) {
        if (lineToKey.find(i) != lineToKey.end()) {
            auto [section, key] = lineToKey[i];
            for (const auto& kv : iniData[section].keyValues) {
                if (kv.first == key && iniData[section].keyToLineIndex[key] == i) {
                    LOG_DEBUG(key << " = " << kv.second);
                    file << key << " = " << kv.second << "\n";
                    break;
                }
            }
        } else {
            file << originalLines[i] << "\n";
        }
    }
    file.close();
    LOG_DEBUG("Config saved to " << filename);

    // Only notify if we have all required pointers (not in initial config)
    if (configManager_ && assets_ && currentIndex_ && tables_) {
        configManager_->notifyConfigChanged(*assets_, *currentIndex_, *tables_);
    }
    hasChanges = false;
    saveMessageTimer_ = 3.0f;
}

void IniEditor::initExplanations()
{
    explanations = Tooltips::getTooltips();
}

void IniEditor::drawGUI()
{
    LOG_DEBUG("drawGUI called, showConfig_: " << (showFlag ? 1 : 0));
    if (!showFlag)
    {
        LOG_DEBUG("Exiting drawGUI due to showFlag false");
        return;
    }
    LOG_DEBUG("Drawing config GUI");
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    LOG_DEBUG("ImGui context in drawGUI: " << (void *)ctx);

    float windowWidth = 800.0f;
    float windowHeight = 500.0f;
    ImGui::SetNextWindowPos(ImVec2((tempSettings_.mainWindowWidth - windowWidth) / 2.0f, (tempSettings_.mainWindowHeight - windowHeight) / 2.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Once);
    ImGui::Begin("ASAPCabinetFE Configuration", &showFlag, ImGuiWindowFlags_NoTitleBar);
    ImGui::SetWindowFocus();

    ImGui::BeginChild("SectionsPane", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
    if (ImGui::BeginListBox("##Sections", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y)))
    {
        for (const auto &section : sections)
        {
            bool is_selected = (currentSection == section);
            if (ImGui::Selectable(section.c_str(), is_selected))
            {
                if (currentSection != section)
                {
                    isCapturingKey_ = false;
                    capturingKeyName_.clear();
                    capturedKeyName_.clear();
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

    ImGui::SameLine();
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
    if (iniData.find(currentSection) != iniData.end())
    {
        static bool firstRenderOfKeybinds = true;
        if (currentSection == "Keybinds" && firstRenderOfKeybinds)
        {
            if (isCapturingKey_)
            {
                LOG_DEBUG("Resetting unexpected key capture state on entering Keybinds section");
                isCapturingKey_ = false;
                capturingKeyName_.clear();
                capturedKeyName_.clear();
            }
            firstRenderOfKeybinds = false;
        }
        else if (currentSection != "Keybinds")
        {
            firstRenderOfKeybinds = true;
        }

        for (auto &kv : iniData[currentSection].keyValues)
        {
            ImGui::Text("%s", kv.first.c_str());
            ImGui::SameLine(150);
            if (explanations.find(kv.first) != explanations.end())
            {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                    ImGui::TextWrapped("%s", explanations[kv.first].c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::SameLine(200);

            if (currentSection == "TitleDisplay" && (kv.first == "FontColor" || kv.first == "FontBgColor"))
            {
                float color[4];
                parseColorString(kv.second, color);

                ImGui::ColorButton(("##ColorButton_" + kv.first).c_str(), ImVec4(color[0], color[1], color[2], color[3]));
                ImGui::SameLine();
                if (ImGui::Button(("Pick##" + kv.first).c_str()))
                {
                    showPicker[kv.first] = !showPicker[kv.first];
                }
                if (showPicker[kv.first])
                {
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::BeginChild(("##ColorPickerChild_" + kv.first).c_str(), ImVec2(300, 250), true);
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                    if (ImGui::ColorPicker4(("##ColorPicker_" + kv.first).c_str(), color, ImGuiColorEditFlags_AlphaBar))
                    {
                        kv.second = colorToString(color);
                        hasChanges = true;
                    }
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                    ImGui::EndGroup();
                }
            }
            else if (currentSection == "Keybinds")
            {
                SDL_Keycode keyCode = SDLK_UNKNOWN;
                if (kv.first == "PreviousTable")
                    keyCode = tempSettings_.keyPreviousTable;
                else if (kv.first == "NextTable")
                    keyCode = tempSettings_.keyNextTable;
                else if (kv.first == "FastPrevTable")
                    keyCode = tempSettings_.keyFastPrevTable;
                else if (kv.first == "FastNextTable")
                    keyCode = tempSettings_.keyFastNextTable;
                else if (kv.first == "LaunchTable")
                    keyCode = tempSettings_.keyLaunchTable;
                else if (kv.first == "ToggleConfig")
                    keyCode = tempSettings_.keyToggleConfig;
                else if (kv.first == "Quit")
                    keyCode = tempSettings_.keyQuit;
                else if (kv.first == "ConfigSave")
                    keyCode = tempSettings_.keyConfigSave;
                else if (kv.first == "ConfigClose")
                    keyCode = tempSettings_.keyConfigClose;
                else if (kv.first == "ScreenshotMode")
                    keyCode = tempSettings_.keyScreenshotMode;

                const char *keyDisplayName = SDL_GetKeyName(keyCode);
                if (keyCode == SDLK_UNKNOWN)
                    keyDisplayName = "Unknown Key";

                ImGui::Text("%s", keyDisplayName);
                ImGui::SameLine(350);
                std::string buttonLabel = (isCapturingKey_ && capturingKeyName_ == kv.first) ? "Waiting...##" + kv.first : "Set Key##" + kv.first;
                if (ImGui::Button(buttonLabel.c_str()))
                {
                    if (!(isCapturingKey_ && capturingKeyName_ == kv.first))
                    {
                        isCapturingKey_ = true;
                        capturingKeyName_ = kv.first;
                        capturedKeyName_.clear();
                    }
                }
            }
            else
            {
                char buf[256];
                std::strncpy(buf, kv.second.c_str(), sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
                if (ImGui::InputText(("##" + kv.first).c_str(), buf, sizeof(buf)))
                {
                    kv.second = std::string(buf);
                    hasChanges = true;
                }
            }
        }
    }
    else
    {
        ImGui::Text("No section data available.");
    }
    ImGui::EndChild();

    if (saveMessageTimer_ > 0.0f)
    {
        saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
    }

    ImGui::BeginChild("ButtonPane", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 0.8f), false, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::Button("Save"))
    {
        saveIniFile(iniFilename);
        hasChanges = false;
        saveMessageTimer_ = 3.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close"))
    {
        loadIniFile(iniFilename);
        hasChanges = false;
        isCapturingKey_ = false;
        capturingKeyName_.clear();
        capturedKeyName_.clear();
        showFlag = false;
    }
    if (saveMessageTimer_ > 0.0f)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved successfully");
    }
    ImGui::EndChild();

    ImGui::End();
}

void IniEditor::handleEvent(const SDL_Event &event)
{
    if (isCapturingKey_ && event.type == SDL_KEYDOWN)
    {
        SDL_Keycode keyCode = event.key.keysym.sym;
        if (keyCode == SDLK_ESCAPE)
        {
            isCapturingKey_ = false;
            capturingKeyName_.clear();
            capturedKeyName_.clear();
        }
        else if (keyCode != SDLK_UNKNOWN)
        {
            const char *keyName = SDL_GetKeyName(keyCode);
            if (keyName && *keyName)
            {
                std::string sdlKeyName = std::string(keyName);
                if (sdlKeyName.substr(0, 5) == "SDLK_")
                {
                    sdlKeyName = sdlKeyName.substr(5);
                }
                std::transform(sdlKeyName.begin(), sdlKeyName.end(), sdlKeyName.begin(), ::toupper);
                capturedKeyName_ = sdlKeyName;

                for (auto &keyVal : iniData[currentSection].keyValues)
                {
                    if (keyVal.first == capturingKeyName_)
                    {
                        keyVal.second = capturedKeyName_;
                        hasChanges = true;

                        if (capturingKeyName_ == "PreviousTable")
                            tempSettings_.keyPreviousTable = keyCode;
                        else if (capturingKeyName_ == "NextTable")
                            tempSettings_.keyNextTable = keyCode;
                        else if (capturingKeyName_ == "FastPrevTable")
                            tempSettings_.keyFastPrevTable = keyCode;
                        else if (capturingKeyName_ == "FastNextTable")
                            tempSettings_.keyFastNextTable = keyCode;
                        else if (capturingKeyName_ == "LaunchTable")
                            tempSettings_.keyLaunchTable = keyCode;
                        else if (capturingKeyName_ == "ToggleConfig")
                            tempSettings_.keyToggleConfig = keyCode;
                        else if (capturingKeyName_ == "Quit")
                            tempSettings_.keyQuit = keyCode;
                        else if (capturingKeyName_ == "ConfigSave")
                            tempSettings_.keyConfigSave = keyCode;
                        else if (capturingKeyName_ == "ConfigClose")
                            tempSettings_.keyConfigClose = keyCode;
                        else if (capturingKeyName_ == "ScreenshotMode")
                            tempSettings_.keyScreenshotMode = keyCode;
                        break;
                    }
                }
                isCapturingKey_ = false;
                capturingKeyName_.clear();
                capturedKeyName_.clear();
            }
        }
        return;
    }
}

void IniEditor::parseColorString(const std::string &colorStr, float color[4])
{
    std::stringstream ss(colorStr);
    std::string token;
    int values[4] = {255, 255, 255, 255};
    int i = 0;

    while (std::getline(ss, token, ',') && i < 4)
    {
        try
        {
            values[i] = std::stoi(token);
            values[i] = std::max(0, std::min(255, values[i]));
        }
        catch (...)
        {
            LOG_DEBUG("Invalid color component in: " + colorStr + ", using default");
            break;
        }
        i++;
    }

    for (int j = 0; j < 4; j++)
    {
        color[j] = values[j] / 255.0f;
    }
}

std::string IniEditor::colorToString(const float color[4])
{
    int r = static_cast<int>(color[0] * 255.0f);
    int g = static_cast<int>(color[1] * 255.0f);
    int b = static_cast<int>(color[2] * 255.0f);
    int a = static_cast<int>(color[3] * 255.0f);
    std::stringstream ss;
    ss << r << "," << g << "," << b << "," << a;
    return ss.str();
}