#include "editor/ui/editor_body.h"
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
        Settings settings = ui.configService()->getSettings();

        // Tables folder check
        const std::string& tablesPath = settings.VPXTablesPath;
        if (!fs::exists(tablesPath)) {
            ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),
                               "Tables path does not exist:\n%s", tablesPath.c_str());
            ImGui::TextDisabled("Please set a valid tables folder in settings.");
        } else {
            bool hasAnyFiles = false;
            for (auto& p : fs::recursive_directory_iterator(tablesPath)) {
                if (p.path().extension() == ".vpx") {
                    hasAnyFiles = true;
                    break;
                }
            }
            if (!hasAnyFiles) {
                ImGui::TextColored(ImVec4(1,0.8f,0.2f,1),
                                   "No .vpx tables found in:\n%s", tablesPath.c_str());
                ImGui::TextDisabled("Please point to a folder containing .vpx tables.");
            } else {
                ImGui::TextDisabled("No table index found, first run?\nPlease pick a scanner and run a rescan tables.");
            }
        }

        const std::string& vpxPath = settings.VPinballXPath;
        if (!fs::exists(vpxPath)) {
            ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),
                               "VPX executable not found:\n%s", vpxPath.c_str());
            ImGui::TextDisabled("Please set the correct path to VPinballX executable in settings.");
        } else if (!fs::is_regular_file(vpxPath)) {
            ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),
                               "VPX path is not a file:\n%s", vpxPath.c_str());
            ImGui::TextDisabled("Please point to the actual VPinballX executable binary.");
        } else if ((fs::status(vpxPath).permissions() & fs::perms::owner_exec) == fs::perms::none) {
            ImGui::TextColored(ImVec4(1,0.8f,0.2f,1),
                               "VPX file is not executable:\n%s", vpxPath.c_str());
            ImGui::TextDisabled("Please make the file executable (chmod +x).");
        }

        ImGui::Separator();
        ImGui::Text("Quick Setup: Correct missing paths");

        Settings& mutableSettings = ui.configService()->getMutableSettings();
        static char tablesPathBuf[1024];
        static char vpxPathBuf[1024];
        static bool initialized = false;
        static bool pathsValid = false;

        if (!initialized) {
            strncpy(tablesPathBuf, mutableSettings.VPXTablesPath.c_str(), sizeof(tablesPathBuf));
            strncpy(vpxPathBuf, mutableSettings.VPinballXPath.c_str(), sizeof(vpxPathBuf));
            initialized = true;
        }

        ImGui::InputText("Tables Folder", tablesPathBuf, sizeof(tablesPathBuf));
        ImGui::InputText("VPX Executable", vpxPathBuf, sizeof(vpxPathBuf));

        if (ImGui::Button("Save Paths##FirstRun")) {
            mutableSettings.VPXTablesPath = tablesPathBuf;
            mutableSettings.VPinballXPath = vpxPathBuf;
            ui.configService()->saveConfig();
            LOG_INFO("First-run paths updated by user.");

            pathsValid = true;
            if (!fs::exists(tablesPathBuf) || !fs::is_directory(tablesPathBuf)) {
                pathsValid = false;
            } else {
                bool hasAny = false;
                for (auto& p : fs::recursive_directory_iterator(tablesPathBuf)) {
                    if (p.path().extension() == ".vpx") {
                        hasAny = true; break;
                    }
                }
                if (!hasAny) pathsValid = false;
            }
            if (!fs::exists(vpxPathBuf) ||
                !fs::is_regular_file(vpxPathBuf) ||
                ((fs::status(vpxPathBuf).permissions() & fs::perms::owner_exec)
                 == fs::perms::none)) {
                pathsValid = false;
            }
        }

        if (pathsValid) {
            ImGui::TextColored(ImVec4(0.2f,0.8f,0.2f,1.0f),
                               "Paths saved and valid. Please pick a scanner and run a rescan.\n"
                               "If you already have an index, exit and re-open the editor.");
        }

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
