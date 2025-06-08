#include "generic_section_renderer.h"
#include "section_config.h"
#include <set>
#include <algorithm>
#include <map>

// Renders a configuration section with fields based on JSON data types
void GenericSectionRenderer::render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, bool defaultOpen) {
    SectionConfig config;
    std::string displayName = config.getSectionDisplayName(sectionName);
    // Create collapsible header for the section
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (defaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen; // Expand section by default if requested
    }
    if (ImGui::CollapsingHeader(displayName.c_str(), flags)) {
        ImGui::Indent(); // Indent content under header
        // Set dynamic width for input fields (50% of available content region for single fields)
        float singleFieldWidth = ImGui::GetContentRegionAvail().x * 0.5f;
        float pairedFieldWidth = ImGui::GetContentRegionAvail().x * 0.25f; // 25% for paired fields

        // Define grouped variable pairs and their display labels
        const std::map<std::pair<std::string, std::string>, std::string> groupedKeys = {
            // [WindowSettings]
            {{"playfieldWindowWidth", "playfieldWindowHeight"}, "Playfield Window Size [W,H]"},
            {{"playfieldX", "playfieldY"}, "Playfield Window Position [X,Y]"},
            {{"dmdWindowWidth", "dmdWindowHeight"}, "DMD Window Size [W,H]"},
            {{"dmdX", "dmdY"}, "DMD Window Position [X,Y]"},
            {{"backglassWindowWidth", "backglassWindowHeight"}, "Backglass Window Size [W,H]"},
            {{"backglassX", "backglassY"}, "Backglass Window Position [X,Y]"},
            {{"topperWindowWidth", "topperWindowHeight"}, "Topper Window Size [W,H]"},
            {{"topperWindowX", "topperWindowY"}, "Topper Window Position [X,Y]"},
            // [TitleDisplay]
            {{"titleX", "titleY"}, "Title Position [X,Y]"},
            // [UIWidgets]
            {{"arrowHintWidth", "arrowHintHeight"}, "Arrow Widget Size [W,H]"},
            // [TableMetadata]
            {{"metadataPanelWidth", "metadataPanelHeight"}, "Metadata Panel Size [W,H]"},
            // [MediaDimensions]
            {{"wheelMediaWidth", "wheelMediaHeight"}, "Wheel Media Size [W,H]"},
            {{"wheelMediaX", "wheelMediaY"}, "Wheel Media Position [X,Y]"},
            {{"playfieldMediaWidth", "playfieldMediaHeight"}, "Playfield Media Size [W,H]"},
            {{"playfieldMediaX", "playfieldMediaY"}, "Playfield Media Position [X,Y]"},
            {{"backglassMediaWidth", "backglassMediaHeight"}, "Backglass Media Size [W,H]"},
            {{"backglassMediaX", "backglassMediaY"}, "Backglass Media Position [X,Y]"},
            {{"dmdMediaWidth", "dmdMediaHeight"}, "DMD Media Size [W,H]"},
            {{"dmdMediaX", "dmdMediaY"}, "DMD Media Position [X,Y]"},
            {{"topperMediaWidth", "topperMediaHeight"}, "Topper Media Size [W,H]"},
            {{"topperMediaX", "topperMediaY"}, "Topper Media Position [X,Y]"}
        };
        std::set<std::string> processedKeys; // Track rendered keys to avoid duplication

        // Create ordered set for consistent key rendering
        std::set<std::string> orderedSet(orderedKeys_.begin(), orderedKeys_.end());
        // Render keys in specified order
        for (const auto& key : orderedKeys_) {
            if (!sectionData.contains(key)) continue; // Skip missing keys
            ImGui::PushID(key.c_str()); // Unique ID for each field
            auto& value = sectionData[key];
            std::string keyDisplayName = config.getKeyDisplayName(sectionName, key);
            ImGui::PushItemWidth(singleFieldWidth);

            // Check if this key starts a grouped pair
            for (const auto& [keyPair, groupLabel] : groupedKeys) {
                const auto& [key1, key2] = keyPair;
                if (key == key1 && sectionData.contains(key2) && !processedKeys.count(key1)) {
                    ImGui::PushItemWidth(pairedFieldWidth);
                    // Render first field with type checking
                    if (value.is_number_integer()) {
                        int val = value.get<int>();
                        if (ImGui::InputInt("##first", &val)) {
                            value = val; // Update JSON on change
                        }
                    } else if (value.is_number_float()) {
                        float val = value.get<float>();
                        if (ImGui::InputFloat("##first", &val)) {
                            value = val; // Update JSON on change
                        }
                    } else {
                        LOG_DEBUG("ConfigUI: Skipping invalid type for " << key1 << ", expected number, got " << value.type_name());
                        int val = 0; // Fallback value
                        ImGui::InputInt("##first", &val);
                    }
                    ImGui::SameLine();
                    // Render second field with type checking
                    auto& value2 = sectionData[key2];
                    if (value2.is_number_integer()) {
                        int val = value2.get<int>();
                        if (ImGui::InputInt("##second", &val)) {
                            value2 = val; // Update JSON on change
                        }
                    } else if (value2.is_number_float()) {
                        float val = value2.get<float>();
                        if (ImGui::InputFloat("##second", &val)) {
                            value2 = val; // Update JSON on change
                        }
                    } else {
                        LOG_DEBUG("ConfigUI: Skipping invalid type for " << key2 << ", expected number, got " << value2.type_name());
                        int val = 0; // Fallback value
                        ImGui::InputInt("##second", &val);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", groupLabel.c_str()); // Label after fields
                    ImGui::PopItemWidth();
                    processedKeys.insert(key1);
                    processedKeys.insert(key2);
                    LOG_DEBUG("ConfigUI: Rendered group " << groupLabel << " with keys " << key1 << ", " << key2);
                    break; // Move to next key
                }
            }

            // Render single fields if not part of a processed group
            if (!processedKeys.count(key)) {
                // Use base renderer helpers
                if (value.is_boolean()) {
                    renderBool(keyDisplayName, value, sectionName);
                } else if (value.is_number_float()) {
                    float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                    float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                    renderFloat(keyDisplayName, value, sectionName, minVal, maxVal);
                } else if (value.is_number_integer()) {
                    if (key.find("Rotation") != std::string::npos) {
                        int rawValue = value.get<int>();
                        LOG_DEBUG("ConfigUI: Raw value for " << keyDisplayName << " is " << rawValue);
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
                    } else if (sectionName == "Keybinds") {
                        renderKeybind(keyDisplayName, value, sectionName, isCapturing, capturingKeyName);
                    } else {
                        renderString(keyDisplayName, value, sectionName);
                    }
                } else if (value.is_array() && value.size() == 4) {
                    renderColor(keyDisplayName, value, sectionName);
                } else {
                    LOG_DEBUG("ConfigUI: Skipping invalid type for " << keyDisplayName << ", expected valid type, got " << value.type_name());
                    int val = 0; // Fallback value
                    ImGui::InputInt(keyDisplayName.c_str(), &val);
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopID(); // End field ID scope
        }

        // Collect and sort remaining keys not in orderedKeys_ or groupedKeys
        std::vector<std::string> remainingKeys;
        for (const auto& [key, _] : sectionData.items()) {
            if (!orderedSet.count(key) && !processedKeys.count(key)) remainingKeys.push_back(key);
        }
        std::sort(remainingKeys.begin(), remainingKeys.end());

        // Render remaining keys alphabetically
        for (const auto& key : remainingKeys) {
            ImGui::PushID(key.c_str()); // Unique ID for each field
            auto& value = sectionData[key];
            std::string keyDisplayName = config.getKeyDisplayName(sectionName, key);
            ImGui::PushItemWidth(singleFieldWidth);

            // Use base renderer helpers
            if (value.is_boolean()) {
                renderBool(keyDisplayName, value, sectionName);
            } else if (value.is_number_float()) {
                float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                renderFloat(keyDisplayName, value, sectionName, minVal, maxVal);
            } else if (value.is_number_integer()) {
                if (key.find("Rotation") != std::string::npos) {
                    int rawValue = value.get<int>();
                    LOG_DEBUG("ConfigUI: Raw value for " << keyDisplayName << " is " << rawValue);
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
                } else if (sectionName == "Keybinds") {
                    renderKeybind(keyDisplayName, value, sectionName, isCapturing, capturingKeyName);
                } else {
                    renderString(keyDisplayName, value, sectionName);
                }
            } else if (value.is_array() && value.size() == 4) {
                renderColor(keyDisplayName, value, sectionName);
            } else {
                LOG_DEBUG("ConfigUI: Skipping invalid type for " << keyDisplayName << ", expected valid type, got " << value.type_name());
                int val = 0; // Fallback value
                ImGui::InputInt(keyDisplayName.c_str(), &val);
            }

            ImGui::PopItemWidth();
            ImGui::PopID(); // End field ID scope
        }
        ImGui::Unindent(); // End section indent
    }
}