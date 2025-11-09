#include "editor/ui/editor_body.h"
#include "editor/editor_first_run.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

namespace editor_body {

void drawBody(EditorUI& ui) {
    std::lock_guard<std::mutex> lock(ui.tableMutex());

    if (ui.loading()) {
        ImGui::Text("Scanning tables... (please wait)");
        return;
    }

    if (ui.tables().empty()) {
        editor_first_run::drawFirstRun(ui);

    } else {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.0f;
        ImVec2 tableSize(avail.x, avail.y - footerHeight);

        ImGui::BeginChild("TableContainer", tableSize, false, ImGuiWindowFlags_NoScrollbar);

        if (ui.filteredTables().empty() && !ui.tables().empty()) {
            ImGui::TextDisabled("No tables match the current filter: '%s'",
                                ui.searchQuery().c_str());
        } else if (!ui.filteredTables().empty()) {
            ImGuiTableFlags flags = ImGuiTableFlags_ScrollY |
                                    ImGuiTableFlags_RowBg |
                                    ImGuiTableFlags_BordersOuter |
                                    ImGuiTableFlags_Resizable |
                                    ImGuiTableFlags_Reorderable |
                                    ImGuiTableFlags_Hideable |
                                    ImGuiTableFlags_Sortable;

            if (ImGui::BeginTable("table_list", 11, flags, tableSize)) {
                ImGui::TableSetupScrollFreeze(0,1);
                ImGui::TableSetupColumn("Year",        ImGuiTableColumnFlags_WidthFixed,   30.0f, 0);
                ImGui::TableSetupColumn("Name",        ImGuiTableColumnFlags_WidthStretch,  0.0f,1);
                ImGui::TableSetupColumn("Version",     ImGuiTableColumnFlags_WidthFixed,   75.0f,2);
                ImGui::TableSetupColumn("Author",      ImGuiTableColumnFlags_WidthFixed,  100.0f,3);
                ImGui::TableSetupColumn("Manufacturer",ImGuiTableColumnFlags_WidthFixed,   80.0f,4);
                ImGui::TableSetupColumn("Files",       ImGuiTableColumnFlags_WidthFixed,   45.0f,5);
                ImGui::TableSetupColumn("ROM",         ImGuiTableColumnFlags_WidthFixed,   75.0f,6);
                ImGui::TableSetupColumn("Extras",      ImGuiTableColumnFlags_WidthFixed,   75.0f,7);
                ImGui::TableSetupColumn("Images",      ImGuiTableColumnFlags_WidthFixed,   75.0f,8);
                ImGui::TableSetupColumn("Videos",      ImGuiTableColumnFlags_WidthFixed,   55.0f,9);
                ImGui::TableSetupColumn("Sounds",      ImGuiTableColumnFlags_WidthFixed,   30.0f,10);

                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
                    if (sortSpecs->SpecsDirty) {
                        const ImGuiTableColumnSortSpecs* spec = sortSpecs->Specs;
                        ui.setSortColumn(spec->ColumnUserID);
                        ui.setSortAscending(spec->SortDirection == ImGuiSortDirection_Ascending);
                        sortSpecs->SpecsDirty = false;
                        ui.filterAndSortTablesPublic();
                    }
                }

                for (int i = 0; i < static_cast<int>(ui.filteredTables().size()); ++i) {
                    const auto& t = ui.filteredTables()[i];
                    ImGui::TableNextRow();

                    std::string displayYear = !t.vpsYear.empty() ? t.vpsYear
                                             : !t.tableYear.empty() ? t.tableYear
                                             : !t.year.empty() ? t.year
                                             : "-";

                    std::string displayName = !t.vpsName.empty() ? t.vpsName
                                            : !t.tableName.empty() ? t.tableName
                                            : !t.title.empty() ? t.title
                                            : "-";

                    std::string displayAuthor = !t.vpsAuthors.empty() ? t.vpsAuthors
                                             : !t.tableAuthor.empty() ? t.tableAuthor
                                             : "-";

                    std::string displayManufacturer = !t.vpsManufacturer.empty() ? t.vpsManufacturer
                                                   : !t.tableManufacturer.empty() ? t.tableManufacturer
                                                   : !t.manufacturer.empty() ? t.manufacturer
                                                   : "-";

                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(displayYear.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(i);
                    bool isSelected = (ui.selectedIndex() == i);
                    if (ImGui::Selectable(displayName.c_str(), isSelected,
                                          ImGuiSelectableFlags_SpanAllColumns)) {
                        ui.setSelectedIndex(isSelected ? -1 : i);
                        ui.setScrollToSelected(false);
                    }
                    ImGui::PopID();

                    ImGui::TableSetColumnIndex(2);
                    if (!t.tableVersion.empty()) {
                        ImGui::TextUnformatted(t.tableVersion.c_str());
                    } else {
                        ImGui::TextUnformatted("");
                    }
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(displayAuthor.c_str());
                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(displayManufacturer.c_str());
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%s%s%s",
                                t.hasINI ? "I " : "- ",
                                t.hasVBS ? "V " : "- ",
                                t.hasB2S ? "B " : "- ");
                    ImGui::TableSetColumnIndex(6);
                    if (!t.romName.empty()) {
                        ImGui::TextUnformatted(t.romName.c_str());
                    } else {
                        ImGui::TextUnformatted("");
                    }
                    ImGui::TableSetColumnIndex(7);
                    ImGui::Text("%s%s%s%s%s",
                                t.hasAltSound ? "S " : "- ",
                                t.hasAltColor ? "C " : "- ",
                                t.hasPup ? "P " : "- ",
                                t.hasUltraDMD ? "U " : "- ",
                                t.hasAltMusic ? "M " : "- ");
                    ImGui::TableSetColumnIndex(8);
                    ImGui::Text("%s%s%s%s%s",
                                t.hasPlayfieldImage ? "P " : "- ",
                                t.hasBackglassImage ? "B " : "- ",
                                t.hasDmdImage ? "D " : "- ",
                                t.hasTopperImage ? "T " : "- ",
                                t.hasWheelImage ? "W " : "- ");
                    ImGui::TableSetColumnIndex(9);
                    ImGui::Text("%s%s%s%s",
                                t.hasPlayfieldVideo ? "P " : "- ",
                                t.hasBackglassVideo ? "B " : "- ",
                                t.hasDmdVideo ? "D " : "- ",
                                t.hasTopperVideo ? "T " : "- ");
                    ImGui::TableSetColumnIndex(10);
                    ImGui::Text("%s%s",
                                t.hasTableMusic ? "M " : "- ",
                                t.hasLaunchAudio ? "L " : "- ");
                }

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }
}
}
