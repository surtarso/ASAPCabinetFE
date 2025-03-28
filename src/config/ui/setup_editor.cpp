// Tarso Galvão Mar/2025

#include "config/ui/setup_editor.h"
#include "config/ui/tooltips.h"
#include "config/settings_manager.h"
#include "utils/logging.h"
#include "render/asset_manager.h"
#include "render/table_loader.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <algorithm>
#include "core/app.h"

namespace SettingsGuiUtils
{
    void DrawSectionsPane(const std::vector<std::string>& sections, std::string& currentSection,
                          bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName)
    {
        ImGui::BeginChild("SectionsPane", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
        if (ImGui::BeginListBox("##Sections", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y)))
        {
            for (const auto& section : sections)
            {
                bool is_selected = (currentSection == section);
                if (ImGui::Selectable(section.c_str(), is_selected))
                {
                    if (currentSection != section)
                    {
                        isCapturingKey = false;
                        capturingKeyName.clear();
                        capturedKeyName.clear();
                        LOG_DEBUG("Switched to section: " << section);
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

    void DrawKeyValuesPane(SettingsSection& section, IKeybindProvider* keybindProvider,
                           bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName,
                           const std::string& currentSection, std::map<std::string, bool>& showPicker,
                           const std::unordered_map<std::string, std::string>& explanations, bool& hasChanges)
    {
        ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        static bool firstRenderOfKeybinds = true;
        if (currentSection == "Keybinds" && firstRenderOfKeybinds)
        {
            if (isCapturingKey)
            {
                isCapturingKey = false;
                capturingKeyName.clear();
                capturedKeyName.clear();
            }
            firstRenderOfKeybinds = false;
        }
        else if (currentSection != "Keybinds")
        {
            firstRenderOfKeybinds = true;
        }

        for (auto& kv : section.keyValues)
        {
            ImGui::Text("%s", kv.first.c_str());
            ImGui::SameLine(150);
            if (currentSection == "Keybinds")
            {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered())
                {
                    std::string tooltip = keybindProvider->getTooltip(kv.first);
                    if (!tooltip.empty())
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                        ImGui::TextWrapped("%s", tooltip.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }
            }
            else if (explanations.find(kv.first) != explanations.end())
            {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
                    ImGui::TextWrapped("%s", explanations.at(kv.first).c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::SameLine(200);

            if (currentSection == "TitleDisplay" && (kv.first == "FontColor" || kv.first == "FontBgColor"))
            {
                float color[4];
                SettingsGuiUtils::ParseColorString(kv.second, color);
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
                        kv.second = SettingsGuiUtils::ColorToString(color);
                        hasChanges = true;
                    }
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                    ImGui::EndGroup();
                }
            }
            else if (currentSection == "Keybinds")
            {
                SDL_Keycode keyCode = keybindProvider->getKey(kv.first);
                const char* keyDisplayName = SDL_GetKeyName(keyCode);
                if (!keyDisplayName || !*keyDisplayName || std::strcmp(keyDisplayName, "Unknown Key") == 0)
                {
                    keyDisplayName = kv.second.c_str();
                }
                ImGui::Text("%s", keyDisplayName);
                ImGui::SameLine(350);
                std::string buttonLabel = (isCapturingKey && capturingKeyName == kv.first) ? "Waiting...##" + kv.first : "Set Key##" + kv.first;
                if (ImGui::Button(buttonLabel.c_str()))
                {
                    if (!(isCapturingKey && capturingKeyName == kv.first))
                    {
                        isCapturingKey = true;
                        capturingKeyName = kv.first;
                        capturedKeyName.clear();
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
        ImGui::EndChild();
    }

    void DrawButtonPane(bool& showFlag, const std::string& iniFilename, SettingsManager* configManager,
                        AssetManager* assets, size_t* currentIndex, std::vector<TableLoader>* tables,
                        bool& hasChanges, bool& isCapturingKey, std::string& capturingKeyName,
                        std::string& capturedKeyName, float& saveMessageTimer,
                        const std::map<std::string, SettingsSection>& iniData, App* app)
    {
        ImGui::BeginChild("ButtonPane", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 0.8f), false, ImGuiWindowFlags_NoScrollbar);
        if (ImGui::Button("Save"))
        {
            LOG_DEBUG("Save button clicked in DrawButtonPane");
            std::ofstream file(iniFilename);
            if (!file.is_open())
            {
                LOG_DEBUG("Could not write " << iniFilename);
            }
            else
            {
                for (const auto& [section, configSection] : iniData)
                {
                    file << "[" << section << "]\n";
                    for (const auto& kv : configSection.keyValues)
                    {
                        file << kv.first << " = " << kv.second << "\n";
                    }
                    file << "\n";
                }
                file.close();
                configManager->loadConfig();
                if (app)
                {
                    LOG_DEBUG("Calling app->onConfigSaved from DrawButtonPane");
                    app->onConfigSaved();
                }
                else
                {
                    LOG_DEBUG("App is null in DrawButtonPane");
                }
                hasChanges = false;
                saveMessageTimer = 3.0f;
                LOG_DEBUG("Config saved to " << iniFilename);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            configManager->loadConfig();
            hasChanges = false;
            isCapturingKey = false;
            capturingKeyName.clear();
            capturedKeyName.clear();
            showFlag = false;
        }
        if (saveMessageTimer > 0.0f)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved successfully");
        }
        ImGui::EndChild();
    }

    void ParseColorString(const std::string& colorStr, float color[4])
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
                break;
            }
            i++;
        }
        for (int j = 0; j < 4; j++)
        {
            color[j] = values[j] / 255.0f;
        }
    }

    std::string ColorToString(const float color[4])
    {
        int r = static_cast<int>(color[0] * 255.0f);
        int g = static_cast<int>(color[1] * 255.0f);
        int b = static_cast<int>(color[2] * 255.0f);
        int a = static_cast<int>(color[3] * 255.0f);
        std::stringstream ss;
        ss << r << "," << g << "," << b << "," << a;
        return ss.str();
    }
}

const std::vector<std::string> ConfigEditor::sectionOrder_ = {
    "VPX", "WindowSettings", "CustomMedia", "MediaDimensions", "TitleDisplay", "UISounds", "Keybinds", "DefaultMedia", "Internal"};

ConfigEditor::ConfigEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager,
                           IKeybindProvider* keybindProvider, App* app)
    : iniFilename_(filename),
      showFlag_(showFlag),
      configManager_(configManager),
      keybindProvider_(keybindProvider),
      tempSettings_(configManager ? configManager->getSettings() : Settings{}),
      app_(app)
{
    loadIniFile(filename);
    initExplanations();
    if (!sections_.empty())
    {
        currentSection_ = sections_[0];
    }
}

void ConfigEditor::loadIniFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LOG_DEBUG("Could not open " << filename);
        return;
    }

    std::string line;
    originalLines_.clear();
    while (std::getline(file, line))
    {
        originalLines_.push_back(line);
    }
    file.close();

    std::string currentSectionName;
    size_t lineIndex = 0;
    iniData_.clear();
    sections_.clear();
    lineToKey_.clear();
    for (const auto& line : originalLines_)
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
#ifdef NDEBUG
            if (currentSectionName == "Internal")
            {
                currentSectionName.clear();
                continue;
            }
#endif
            sections_.push_back(currentSectionName);
            iniData_[currentSectionName] = SettingsSection();
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
                iniData_[currentSectionName].keyValues.emplace_back(key, value);
                iniData_[currentSectionName].keyToLineIndex[key] = lineIndex;
                lineToKey_[lineIndex] = {currentSectionName, key};
            }
        }
        lineIndex++;
    }

    std::vector<std::string> sortedSections;
    for (const auto& orderedSection : sectionOrder_)
    {
        if (std::find(sections_.begin(), sections_.end(), orderedSection) != sections_.end())
        {
            sortedSections.push_back(orderedSection);
        }
    }
    for (const auto& section : sections_)
    {
        if (std::find(sectionOrder_.begin(), sectionOrder_.end(), section) == sectionOrder_.end())
        {
            sortedSections.push_back(section);
        }
    }
    sections_ = sortedSections;

    hasChanges_ = false;
    LOG_DEBUG("Loaded config file: " << filename);
}

void ConfigEditor::initExplanations()
{
    explanations_ = Tooltips::getTooltips();
}

void ConfigEditor::drawGUI()
{
    if (!showFlag_)
    {
        configLoaded_ = false;
        return;
    }

    if (fillParentWindow_)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::Begin("ASAPCabinetFE Configuration", &showFlag_, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    }
    else
    {
        float windowWidth = 800.0f;
        float windowHeight = 500.0f;
        ImGui::SetNextWindowPos(ImVec2((tempSettings_.mainWindowWidth - windowWidth) / 2.0f,
                                       (tempSettings_.mainWindowHeight - windowHeight) / 2.0f),
                                ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Once);
        ImGui::Begin("ASAPCabinetFE Configuration", &showFlag_, ImGuiWindowFlags_NoTitleBar);
    }

    ImGui::SetWindowFocus();

    if (!configLoaded_)
    {
        loadIniFile(iniFilename_);
        configLoaded_ = true;
    }

    SettingsGuiUtils::DrawSectionsPane(sections_, currentSection_, isCapturingKey_, capturingKeyName_, capturedKeyName_);

    ImGui::SameLine();
    if (currentSection_ == "Table Overrides")
    {
        drawTableOverridesGUI();
    }
    else if (iniData_.find(currentSection_) != iniData_.end())
    {
        SettingsGuiUtils::DrawKeyValuesPane(iniData_[currentSection_], keybindProvider_,
                                            isCapturingKey_, capturingKeyName_, capturedKeyName_,
                                            currentSection_, showPicker_, explanations_, hasChanges_);
    }
    else
    {
        ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
        ImGui::Text("No section data available.");
        ImGui::EndChild();
    }

    if (saveMessageTimer_ > 0.0f)
    {
        saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
    }

    SettingsGuiUtils::DrawButtonPane(showFlag_, iniFilename_, configManager_, getAssets(), getCurrentIndex(),
                                     getTables(), hasChanges_, isCapturingKey_, capturingKeyName_,
                                     capturedKeyName_, saveMessageTimer_, iniData_, app_);
    ImGui::End();
}

void ConfigEditor::handleEvent(const SDL_Event& event)
{
    if (!isCapturingKey_)
        return;

    if (event.type == SDL_KEYDOWN)
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
            const char* keyName = SDL_GetKeyName(keyCode);
            if (keyName && *keyName)
            {
                std::string sdlKeyName = std::string(keyName);
                if (sdlKeyName.substr(0, 5) == "SDLK_")
                {
                    sdlKeyName = sdlKeyName.substr(5);
                }
                std::transform(sdlKeyName.begin(), sdlKeyName.end(), sdlKeyName.begin(), ::toupper);
                capturedKeyName_ = sdlKeyName;

                for (auto& keyVal : iniData_[currentSection_].keyValues)
                {
                    if (keyVal.first == capturingKeyName_)
                    {
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
    else if (event.type == SDL_JOYBUTTONDOWN)
    {
        int joystickId = event.jbutton.which;
        uint8_t button = event.jbutton.button;
        std::stringstream ss;
        ss << "JOY_" << joystickId << "_BUTTON_" << static_cast<int>(button);
        capturedKeyName_ = ss.str();

        for (auto& keyVal : iniData_[currentSection_].keyValues)
        {
            if (keyVal.first == capturingKeyName_)
            {
                keyVal.second = capturedKeyName_;
                hasChanges_ = true;
                keybindProvider_->setJoystickButton(capturingKeyName_, joystickId, button);
                break;
            }
        }
        isCapturingKey_ = false;
        capturingKeyName_.clear();
        capturedKeyName_.clear();
    }
    else if (event.type == SDL_JOYHATMOTION)
    {
        int joystickId = event.jhat.which;
        uint8_t hat = event.jhat.hat;
        uint8_t value = event.jhat.value;
        if (value == SDL_HAT_UP || value == SDL_HAT_DOWN || value == SDL_HAT_LEFT || value == SDL_HAT_RIGHT)
        {
            std::stringstream ss;
            ss << "JOY_" << joystickId << "_HAT_" << static_cast<int>(hat) << "_";
            switch (value)
            {
            case SDL_HAT_UP:
                ss << "UP";
                break;
            case SDL_HAT_DOWN:
                ss << "DOWN";
                break;
            case SDL_HAT_LEFT:
                ss << "LEFT";
                break;
            case SDL_HAT_RIGHT:
                ss << "RIGHT";
                break;
            }
            capturedKeyName_ = ss.str();

            for (auto& keyVal : iniData_[currentSection_].keyValues)
            {
                if (keyVal.first == capturingKeyName_)
                {
                    keyVal.second = capturedKeyName_;
                    hasChanges_ = true;
                    keybindProvider_->setJoystickHat(capturingKeyName_, joystickId, hat, value);
                    break;
                }
            }
            isCapturingKey_ = false;
            capturingKeyName_.clear();
            capturedKeyName_.clear();
        }
    }
    else if (event.type == SDL_JOYAXISMOTION)
    {
        int joystickId = event.jaxis.which;
        uint8_t axis = event.jaxis.axis;
        int value = event.jaxis.value;
        const int threshold = 16384;
        if (value > threshold || value < -threshold)
        {
            bool positiveDirection = (value > 0);
            std::stringstream ss;
            ss << "JOY_" << joystickId << "_AXIS_" << static_cast<int>(axis) << "_"
               << (positiveDirection ? "POSITIVE" : "NEGATIVE");
            capturedKeyName_ = ss.str();

            for (auto& keyVal : iniData_[currentSection_].keyValues)
            {
                if (keyVal.first == capturingKeyName_)
                {
                    keyVal.second = capturedKeyName_;
                    hasChanges_ = true;
                    keybindProvider_->setJoystickAxis(capturingKeyName_, joystickId, axis, positiveDirection);
                    break;
                }
            }
            isCapturingKey_ = false;
            capturingKeyName_.clear();
            capturedKeyName_.clear();
        }
    }
}

RuntimeEditor::RuntimeEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager,
                             IKeybindProvider* keybindProvider, AssetManager* assets, size_t* currentIndex,
                             std::vector<TableLoader>* tables, App* app)
    : ConfigEditor(filename, showFlag, configManager, keybindProvider, app),
      assets_(assets),
      currentIndex_(currentIndex),
      tables_(tables)
{
    sections_.push_back("Table Overrides");
}

void RuntimeEditor::drawTableOverridesGUI()
{
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.5f), true);
    if (!tables_ || !currentIndex_ || tables_->empty())
    {
        ImGui::Text("No tables available.");
        ImGui::EndChild();
        return;
    }

    ImGui::Text("Table Overrides for: %s", (*tables_)[*currentIndex_].tableName.c_str());
    TableLoader& table = (*tables_)[*currentIndex_];
    char buf[256];

    std::strncpy(buf, table.tableImage.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Table Image", buf, sizeof(buf)))
    {
        table.tableImage = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.backglassImage.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Backglass Image", buf, sizeof(buf)))
    {
        table.backglassImage = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.dmdImage.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("DMD Image", buf, sizeof(buf)))
    {
        table.dmdImage = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.tableVideo.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Table Video", buf, sizeof(buf)))
    {
        table.tableVideo = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.backglassVideo.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Backglass Video", buf, sizeof(buf)))
    {
        table.backglassVideo = std::string(buf);
        hasChanges_ = true;
    }

    std::strncpy(buf, table.dmdVideo.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("DMD Video", buf, sizeof(buf)))
    {
        table.dmdVideo = std::string(buf);
        hasChanges_ = true;
    }

    ImGui::EndChild();
}

void ConfigEditor::saveConfig()
{
    LOG_DEBUG("Starting saveConfig");
    std::ofstream file(iniFilename_);
    if (!file.is_open())
    {
        LOG_DEBUG("Could not write " << iniFilename_);
        return;
    }
    for (const auto& [section, configSection] : iniData_)
    {
        file << "[" << section << "]\n";
        for (const auto& kv : configSection.keyValues)
        {
            file << kv.first << " = " << kv.second << "\n";
        }
        file << "\n";
    }
    file.close();
    LOG_DEBUG("File written, calling loadConfig");
    configManager_->loadConfig();
    if (app_)
    {
        LOG_DEBUG("app_ exists, calling onConfigSaved");
        app_->onConfigSaved();
        LOG_DEBUG("onConfigSaved called");
    }
    else
    {
        LOG_DEBUG("app_ is null, skipping onConfigSaved");
    }
    hasChanges_ = false;
    saveMessageTimer_ = 3.0f;
    loadIniFile(iniFilename_);
    LOG_DEBUG("Config saved to " << iniFilename_);
}

void RuntimeEditor::saveConfig()
{
    ConfigEditor::saveConfig();
}