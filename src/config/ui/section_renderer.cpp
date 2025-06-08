#include "section_renderer.h"
#include "section_config.h"
#include "config_ui.h"
#include <set>
#include <algorithm>
#include <map>

void SectionRenderer::render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, ImGuiFileDialog* fileDialog, bool defaultOpen, bool& isDialogOpen, std::string& dialogKey) {
    SectionConfig config;
    std::string displayName = config.getSectionDisplayName(sectionName);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (defaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (ImGui::CollapsingHeader(displayName.c_str(), flags)) {
        ImGui::Indent();
        float singleFieldWidth = ImGui::GetContentRegionAvail().x * 0.5f;
        float pairedFieldWidth = ImGui::GetContentRegionAvail().x * 0.25f;

        const std::map<std::pair<std::string, std::string>, std::string> groupedKeys = {
            {{"playfieldWindowWidth", "playfieldWindowHeight"}, "Playfield Window Size [W,H]"},
            {{"playfieldX", "playfieldY"}, "Playfield Window Position [X,Y]"},
            {{"dmdWindowWidth", "dmdWindowHeight"}, "DMD Window Size [W,H]"},
            {{"dmdX", "dmdY"}, "DMD Window Position [X,Y]"},
            {{"backglassWindowWidth", "backglassWindowHeight"}, "Backglass Window Size [W,H]"},
            {{"backglassX", "backglassY"}, "Backglass Window Position [X,Y]"},
            {{"topperWindowWidth", "topperWindowHeight"}, "Topper Window Size [W,H]"},
            {{"topperWindowX", "topperWindowY"}, "Topper Window Position [X,Y]"},
            {{"titleX", "titleY"}, "Title Position [X,Y]"},
            {{"arrowHintWidth", "arrowHintHeight"}, "Arrow Widget Size [W,H]"},
            {{"metadataPanelWidth", "metadataPanelHeight"}, "Metadata Panel Size [W,H]"},
            {{"wheelMediaWidth", "wheelMediaHeight"}, "Wheel Media Size [W,H]"},
            {{"wheelMediaX", "wheelMediaY"}, "Wheel Media Position [X,Y]"},
            {{"playfieldMediaWidth", "playfieldMediaHeight"}, "Playfield Media Size [W,H]"},
            {{"playfieldMediaX", "playfieldMediaY"}, "Playfield Media Position [X,Y]"},
            {{"backglassMediaWidth", "backglassMediaHeight"}, "Backglass Media Size [W,H]"},
            {{"backglassMediaX", "backglassMediaY"}, "Backglass Media Position [X,Y]"},
            {{"dmdMediaWidth", "dmdMediaHeight"}, "DMD Media Size [W,H]"},
            {{"dmdMediaX", "dmdMediaY"}, "DMD Media Position [X,Y]"},
            {{"topperMediaWidth", "topperMediaHeight"}, "Topper Media Size [W,H]"},
            {{"topperMediaX", "topperMediaY"}, "Topper Window Position [X,Y]"}
        };
        std::set<std::string> processedKeys;

        std::set<std::string> orderedSet(orderedKeys_.begin(), orderedKeys_.end());
        for (const auto& key : orderedKeys_) {
            if (!sectionData.contains(key)) continue;
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            std::string keyDisplayName = config.getKeyDisplayName(sectionName, key);
            ImGui::PushItemWidth(singleFieldWidth);

            for (const auto& [keyPair, groupLabel] : groupedKeys) {
                const auto& [key1, key2] = keyPair;
                if (key == key1 && sectionData.contains(key2) && !processedKeys.count(key1)) {
                    ImGui::PushItemWidth(pairedFieldWidth);
                    if (value.is_number_integer()) {
                        int val = value.get<int>();
                        if (ImGui::InputInt("##first", &val)) {
                            value = val;
                        }
                    } else if (value.is_number_float()) {
                        float val = value.get<float>();
                        if (ImGui::InputFloat("##first", &val)) {
                            value = val;
                        }
                    } else {
                        LOG_DEBUG("ConfigUI: Skipping invalid type for " << key1 << ", expected number, got " << value.type_name());
                        int val = 0;
                        ImGui::InputInt("##first", &val);
                    }
                    ImGui::SameLine();
                    auto& value2 = sectionData[key2];
                    if (value2.is_number_integer()) {
                        int val = value2.get<int>();
                        if (ImGui::InputInt("##second", &val)) {
                            value2 = val;
                        }
                    } else if (value2.is_number_float()) {
                        float val = value2.get<float>();
                        if (ImGui::InputFloat("##second", &val)) {
                            value2 = val;
                        }
                    } else {
                        LOG_DEBUG("ConfigUI: Skipping invalid type for " << key2 << ", expected number, got " << value2.type_name());
                        int val = 0;
                        ImGui::InputInt("##second", &val);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", groupLabel.c_str());
                    ImGui::PopItemWidth();
                    processedKeys.insert(key1);
                    processedKeys.insert(key2);
                    //LOG_DEBUG("ConfigUI: Rendered group " << groupLabel << " with keys " << key1 << ", " << key2);
                    break;
                }
            }

            if (!processedKeys.count(key)) {
                if (value.is_boolean()) {
                    renderBool(keyDisplayName, value, sectionName);
                } else if (value.is_number_float()) {
                    float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                    float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                    renderFloat(keyDisplayName, value, sectionName, minVal, maxVal);
                } else if (value.is_number_integer()) {
                    if (key.find("Rotation") != std::string::npos) {
                        renderRotation(keyDisplayName, value, sectionName);
                    } else {
                        renderInt(keyDisplayName, value, sectionName);
                    }
                } else if (value.is_string()) {
                    const auto& options = config.getDropdownOptions(sectionName, key);
                    if (!options.empty()) {
                        std::string val = value.get<std::string>();
                        int currentIndex = -1;
                        for (size_t i = 0; i < options.size(); ++i) {
                            if (options[i] == val) {
                                currentIndex = static_cast<int>(i);
                                break;
                            }
                        }
                        if (ImGui::Combo(keyDisplayName.c_str(), &currentIndex, [](void* data, int idx, const char** out_text) {
                            auto& options = *static_cast<std::vector<std::string>*>(data);
                            if (idx >= 0 && idx < static_cast<int>(options.size())) {
                                *out_text = options[idx].c_str();
                                return true;
                            }
                            return false;
                        }, (void*)&options, static_cast<int>(options.size()))) {
                            if (currentIndex >= 0 && currentIndex < static_cast<int>(options.size())) {
                                value = options[currentIndex];
                                LOG_DEBUG("ConfigUI: Updated " << keyDisplayName << " to " << options[currentIndex]);
                            }
                        }
                    } else if (sectionName == "VPX" && (key == "VPXTablesPath" || key == "VPinballXPath" || key == "vpxIniPath")) {
                        //LOG_DEBUG("ConfigUI: Rendering path for key " << key << " in section " << sectionName);
                        renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                    } else if (sectionName == "Keybinds") {
                        renderKeybind(keyDisplayName, value, sectionName, isCapturing, capturingKeyName);
                    } else {
                        renderString(keyDisplayName, value, sectionName);
                    }
                } else if (value.is_array() && value.size() == 4) {
                    renderColor(keyDisplayName, value, sectionName);
                } else {
                    LOG_DEBUG("ConfigUI: Skipping invalid type for " << keyDisplayName << ", expected valid type, got " << value.type_name());
                    int val = 0;
                    ImGui::InputInt(keyDisplayName.c_str(), &val);
                }
                if (ImGui::IsItemHovered()) {
                    auto it = Settings::settingsMetadata.find(key);
                    if (it != Settings::settingsMetadata.end()) {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(it->second.second.c_str());
                        ImGui::EndTooltip();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }

        std::vector<std::string> remainingKeys;
        for (const auto& [key, _] : sectionData.items()) {
            if (!orderedSet.count(key) && !processedKeys.count(key)) remainingKeys.push_back(key);
        }
        std::sort(remainingKeys.begin(), remainingKeys.end());

        for (const auto& key : remainingKeys) {
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            std::string keyDisplayName = config.getKeyDisplayName(sectionName, key);
            ImGui::PushItemWidth(singleFieldWidth);

            if (value.is_boolean()) {
                renderBool(keyDisplayName, value, sectionName);
            } else if (value.is_number_float()) {
                float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                renderFloat(keyDisplayName, value, sectionName, minVal, maxVal);
            } else if (value.is_number_integer()) {
                if (key.find("Rotation") != std::string::npos) {
                    renderRotation(keyDisplayName, value, sectionName);
                } else {
                    renderInt(keyDisplayName, value, sectionName);
                }
            } else if (value.is_string()) {
                const auto& options = config.getDropdownOptions(sectionName, key);
                if (!options.empty()) {
                    std::string val = value.get<std::string>();
                    int currentIndex = -1;
                    for (size_t i = 0; i < options.size(); ++i) {
                        if (options[i] == val) {
                            currentIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    if (ImGui::Combo(keyDisplayName.c_str(), &currentIndex, [](void* data, int idx, const char** out_text) {
                        auto& options = *static_cast<std::vector<std::string>*>(data);
                        if (idx >= 0 && idx < static_cast<int>(options.size())) {
                            *out_text = options[idx].c_str();
                            return true;
                        }
                        return false;
                    }, (void*)&options, static_cast<int>(options.size()))) {
                        if (currentIndex >= 0 && currentIndex < static_cast<int>(options.size())) {
                            value = options[currentIndex];
                            LOG_DEBUG("ConfigUI: Updated " << keyDisplayName << " to " << options[currentIndex]);
                        }
                    }
                } else if (sectionName == "VPX" && (key == "VPXTablesPath" || key == "VPinballXPath" || key == "vpxIniPath")) {
                    LOG_DEBUG("ConfigUI: Rendering path for key " << key << " in section " << sectionName);
                    renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                } else if (sectionName == "Keybinds") {
                    renderKeybind(keyDisplayName, value, sectionName, isCapturing, capturingKeyName);
                } else {
                    renderString(keyDisplayName, value, sectionName);
                }
            } else if (value.is_array() && value.size() == 4) {
                renderColor(keyDisplayName, value, sectionName);
            } else {
                LOG_DEBUG("ConfigUI: Skipping invalid type for " << keyDisplayName << ", expected valid type, got " << value.type_name());
                int val = 0;
                ImGui::InputInt(keyDisplayName.c_str(), &val);
            }
            if (ImGui::IsItemHovered()) {
                auto it = Settings::settingsMetadata.find(key);
                if (it != Settings::settingsMetadata.end()) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(it->second.second.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
        ImGui::PushID("ResetButton");
        if (ImGui::Button("Reset to Defaults", ImVec2(130, 0))) {
            ConfigUI* configUI = dynamic_cast<ConfigUI*>(this);
            if (configUI) {
                configUI->resetSectionToDefault(sectionName);
            }
        }
        ImGui::PopID();
        ImGui::Unindent();
    }
}