#include "editor/ui/editor_header.h"
#include <imgui.h>

namespace editor_header {

void drawHeader(EditorUI& ui) {
    // Detect typing and auto-focus search field
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
        });

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 2.5f);
    if (ImGui::InputTextWithHint("##SearchInputTop", "Search by Name, File, or ROM",
                                 ui.searchBuffer(), sizeof(ui.searchBuffer()))) {
        ui.setSearchQuery(ui.searchBuffer());
        ui.filterAndSortTablesPublic();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Clear##TopSearch")) {
        ui.searchBuffer()[0] = '\0';
        ui.searchQuery().clear();
        ui.filterAndSortTablesPublic();
    }
}
}
