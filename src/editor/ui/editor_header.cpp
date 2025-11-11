#include "editor/ui/editor_header.h"
#include <imgui.h>

namespace editor_header {

void drawHeader(EditorUI& ui) {
    // Detect typing and auto-focus search field
    if (!ImGui::IsItemActive()) {
        ui.actions().handleKeyboardSearchFocus(
            ui.searchBuffer(),
            ui.searchQuery(),
            [&ui]() { ui.filterAndSortTablesPublic(); },
            [&ui]() {
                if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
                    const auto& t = ui.filteredTables()[ui.selectedIndex()];
                    ui.tableLauncher()->launchTable(t);
                } else {
                    LOG_DEBUG("Enter pressed but no table selected");
                }
            }
        );
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 1.4f);
    if (ImGui::InputTextWithHint("##SearchInputTop", "Search by Name, File, or ROM",
                                 ui.searchBuffer(), sizeof(ui.searchBuffer()))) {
        ui.setSearchQuery(ui.searchBuffer());
        ui.filterAndSortTablesPublic();
    }
    // ImGui::Text("Buffer len: %zu  Query len: %zu", strlen(ui.searchBuffer()), ui.searchQuery().size());
    // ImGui::Text("strlen: %zu  raw bytes:", strlen(ui.searchBuffer()));
    // for (int i = 0; i < 16; i++) {
    //     ImGui::SameLine();
    //     ImGui::Text("%02X", static_cast<unsigned char>(ui.searchBuffer()[i]));
    // }

    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::BeginCombo("##advanced_combo", "Advanced", ImGuiComboFlags_NoPreview | ImGuiComboFlags_HeightLargest)) {

        ImGui::TextDisabled("Test choices");
        if (ImGui::Selectable("test choice 1", false))
            LOG_DEBUG("Advanced Menu test choice 1 button pressed (placeholder)");
        if (ImGui::Selectable("test choice 2", true))
            LOG_DEBUG("Advanced Menu test choice 2 button pressed (placeholder)");
        if (ImGui::Selectable("test choice 3", false))
            LOG_DEBUG("Advanced Menu test choice 3 button pressed (placeholder)");

        ImGui::TextDisabled("Test options");
        bool flag = false;
        if (ImGui::Checkbox("test option 1", &flag)) {
            LOG_DEBUG("Advanced Menu test option 1 button pressed (placeholder)");
        }
        bool flagUse = true;
        if (ImGui::Checkbox("test option 2", &flagUse)) {
            LOG_DEBUG("Advanced Menu test option 2 button pressed (placeholder)");
        }

        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.24f, 0.24f, 1.0f));
        if (ImGui::Selectable("Exit Editor", false)) {
            LOG_DEBUG("Exit Editor requested from Advanced Menu");
            ui.requestExit();
        }
        ImGui::PopStyleColor();
        ImGui::EndCombo();
    }
}
}
