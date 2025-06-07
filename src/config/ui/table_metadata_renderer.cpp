#include "table_metadata_renderer.h"

void TableMetadataSectionRenderer::render(const std::string& sectionName, nlohmann::json& sectionData) {
    if (ImGui::CollapsingHeader(sectionName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        // Render ordered keys
        for (const auto& key : orderedKeys_) {
            if (!sectionData.contains(key)) continue;
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            if (key == "showMetadata" || key == "fetchVPSdb" || key == "forceRebuildMetadata") {
                renderBool(key, value, sectionName);
            } else if (key == "metadataPanelWidth" || key == "metadataPanelHeight" || key == "metadataPanelAlpha") {
                renderFloat(key, value, sectionName, 0.0f, 1.0f, "%.2f");
            } else if (key == "titleSource") {
                std::string val = value.get<std::string>();
                const char* options[] = {"filename", "metadata"};
                int index = (val == "metadata") ? 1 : 0;
                if (ImGui::Combo(key.c_str(), &index, options, IM_ARRAYSIZE(options))) {
                    value = options[index];
                    LOG_DEBUG("TableMetadataSectionRenderer: Updated " << sectionName << "." << key << " to " << value);
                }
            } else if (key == "titleSortBy") {
                std::string val = value.get<std::string>();
                const char* options[] = {"title", "year", "manufacturer", "type", "author"};
                int index = 0;
                if (val == "year") index = 1;
                else if (val == "manufacturer") index = 2;
                else if (val == "type") index = 3;
                else if (val == "author") index = 4;
                if (ImGui::Combo(key.c_str(), &index, options, IM_ARRAYSIZE(options))) {
                    value = options[index];
                    LOG_DEBUG("TableMetadataSectionRenderer: Updated " << sectionName << "." << key << " to " << value);
                }
            }
            ImGui::PopID();
        }
        ImGui::Unindent();
    }
}