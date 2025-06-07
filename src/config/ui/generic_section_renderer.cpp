#include "generic_section_renderer.h"
#include <set>
#include <algorithm>

void GenericSectionRenderer::render(const std::string& sectionName, nlohmann::json& sectionData) {
    if (ImGui::CollapsingHeader(sectionName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        // Render ordered keys
        std::set<std::string> orderedSet(orderedKeys_.begin(), orderedKeys_.end());
        for (const auto& key : orderedKeys_) {
            if (!sectionData.contains(key)) continue;
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            if (value.is_boolean()) {
                renderBool(key, value, sectionName);
            } else if (value.is_number_float()) {
                float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                renderFloat(key, value, sectionName, minVal, maxVal, "%.2f");
            } else if (value.is_number_integer()) {
                int minVal = (key.find("Rotation") != std::string::npos) ? 0 : 0;
                int maxVal = (key.find("Rotation") != std::string::npos) ? 360 : 10000;
                renderInt(key, value, sectionName, minVal, maxVal);
            } else if (value.is_string()) {
                renderString(key, value, sectionName);
            } else if (value.is_array() && value.size() == 4) {
                renderColor(key, value, sectionName);
            }
            ImGui::PopID();
        }
        // Render remaining fields alphabetically
        std::vector<std::string> remainingKeys;
        for (const auto& [key, _] : sectionData.items()) {
            if (!orderedSet.count(key)) remainingKeys.push_back(key);
        }
        std::sort(remainingKeys.begin(), remainingKeys.end());
        for (const auto& key : remainingKeys) {
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            if (value.is_boolean()) {
                renderBool(key, value, sectionName);
            } else if (value.is_number_float()) {
                float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                renderFloat(key, value, sectionName, minVal, maxVal, "%.2f");
            } else if (value.is_number_integer()) {
                int minVal = (key.find("Rotation") != std::string::npos) ? 0 : 0;
                int maxVal = (key.find("Rotation") != std::string::npos) ? 360 : 10000;
                renderInt(key, value, sectionName, minVal, maxVal);
            } else if (value.is_string()) {
                renderString(key, value, sectionName);
            } else if (value.is_array() && value.size() == 4) {
                renderColor(key, value, sectionName);
            }
            ImGui::PopID();
        }
        ImGui::Unindent();
    }
}