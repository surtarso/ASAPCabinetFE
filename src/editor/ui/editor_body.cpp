#include "editor/ui/editor_body.h"
#include "editor/editor_first_run.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

namespace editor_body {

// TODO: move all tooltip logic to util/editor_tooltips.h (and maybe move to editor/ui/)
// TODO: rename button -> footer, menu -> header (_actions.h|cpp)

// Forward declarations for all tooltip helpers
static void drawYearTooltip(const TableData& t);
static void drawNameTooltip(const TableData& t);
static void drawAuthorTooltip(const TableData& t);
static void drawFilesTooltip(const TableData& t);
static void drawRomTooltip(const TableData& t);
static void drawVersionTooltip(const TableData& t);
static void drawExtrasTooltip(const TableData& t);
static void drawAssetsTooltip(const TableData& t);
static void drawManufacturerTooltip(const TableData& t);
static void drawVideosTooltip(const TableData& t);
static void drawSoundsTooltip(const TableData& t);

// ------------------------------------------------------------------
// Tooltip router
static void drawTooltipForColumn(int column, const TableData& t, EditorUI& ui) {
    // --- Global tooltip suppression logic ---
    Settings settings = ui.configService()->getSettings();
    // --------------------------------------------
    // Unified tooltip visibility logic:
    //
    // showTooltips = XOR(settings.showTableTooltips, ctrlHeld)
    //
    // Meaning:
    //   - If tooltips ON  → CTRL hides
    //   - If tooltips OFF → CTRL shows
    // --------------------------------------------
    bool ctrlHeld = ImGui::GetIO().KeyCtrl;
    bool showTooltips = settings.showTableTooltips ^ ctrlHeld;

    if (!showTooltips || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup))
        return; // skip drawing tooltips

    // Only draw when permitted
    ImGui::BeginTooltip();
    switch (column) {
        case 0: drawYearTooltip(t); break;
        case 1: drawNameTooltip(t); break;
        case 2: drawVersionTooltip(t); break;
        case 3: drawAuthorTooltip(t); break;
        case 4: drawManufacturerTooltip(t); break;
        case 5: drawFilesTooltip(t); break;
        case 6: drawRomTooltip(t); break;
        case 7: drawExtrasTooltip(t); break;
        case 8: drawAssetsTooltip(t); break;
        case 9: drawVideosTooltip(t); break;
        case 10: drawSoundsTooltip(t); break;
        default:
            ImGui::TextDisabled("No tooltip defined.");
            break;
    }
    ImGui::EndTooltip();
}

// ------------------------------------------------------------------
// YEAR column
static void drawYearTooltip(const TableData& t) {
    ImGui::Text("Metadata: %s", t.tableYear.empty() ? "-" : t.tableYear.c_str());
    ImGui::Text("VPSDB: %s", t.vpsYear.empty() ? "-" : t.vpsYear.c_str());

    ImGui::Separator();
    ImGui::Text("Best match: %s (%.0f%%)", t.year.c_str(), t.matchConfidence * 100.0f);
}

// ------------------------------------------------------------------
// NAME column
static void drawNameTooltip(const TableData& t) {
    ImGui::Text("File: %s", t.vpxFile.empty() ? "-" : t.vpxFile.c_str());
    ImGui::Text("Metadata: %s", t.tableName.empty() ? "-" : t.tableName.c_str());
    ImGui::Text("VPSDB: %s", t.vpsName.empty() ? "-" : t.vpsName.c_str());

    ImGui::Separator();
    ImGui::Text("Best match: %s (%.0f%%)", t.title.c_str(), t.matchConfidence * 100.0f);
}

// ------------------------------------------------------------------
// AUTHOR column
static void drawAuthorTooltip(const TableData& t) {
    ImGui::Text("Metadata: %s", t.tableAuthor.empty() ? "-" : t.tableAuthor.c_str());
    ImGui::Text("VPSDB: %s", t.vpsAuthors.empty() ? "-" : t.vpsAuthors.c_str());

    std::string best = !t.vpsAuthors.empty() ? t.vpsAuthors :
                       !t.tableAuthor.empty() ? t.tableAuthor : "-";
    ImGui::Separator();
    ImGui::Text("Best match: %s (%.0f%%)", best.c_str(), t.matchConfidence * 100.0f);
}

// ------------------------------------------------------------------
// FILES column
static void drawFilesTooltip(const TableData& t) {
    ImGui::Text("Legend: I=INI, V=VBS, B=B2S");
    ImGui::TextColored(t.hasINI ? (t.hasDiffVbs ? ImVec4(1.f,1.f,0.f,1.f) : ImVec4(0.8f,0.8f,0.8f,1.f))
                                : ImVec4(0.5f,0.5f,0.5f,1.f),
                       "INI: %s", t.hasINI ? (t.hasDiffVbs ? "modified" : "present") : "-");
    ImGui::TextColored(t.hasVBS ? (t.hasDiffVbs ? ImVec4(1.f,1.f,0.f,1.f) : ImVec4(0.8f,0.8f,0.8f,1.f))
                                : ImVec4(0.5f,0.5f,0.5f,1.f),
                       "VBS: %s", t.hasVBS ? (t.hasDiffVbs ? "modified" : "present") : "-");
    ImGui::Text("B2S: %s", t.hasB2S ? "present" : "-");
}

// ------------------------------------------------------------------
// ROM column
static void drawRomTooltip(const TableData& t) {
    ImGui::Text("Local: %s", t.romName.empty() ? "-" : t.romName.c_str());
    ImGui::Text("Metadata: (placeholder"); // TODO: add rom from metadata (vpxtool/vpin)
}

// ------------------------------------------------------------------
// MANUFACTURER column
static void drawManufacturerTooltip(const TableData& t) {
    ImGui::Text("Metadata: %s", t.tableManufacturer.empty() ? "-" : t.tableManufacturer.c_str());
    ImGui::Text("VPSDB: %s", t.vpsManufacturer.empty() ? "-" : t.vpsManufacturer.c_str());
    ImGui::Separator();
    ImGui::Text("Best match: %s (%.0f%%)", t.manufacturer.c_str(), t.matchConfidence * 100.0f);
}

// ------------------------------------------------------------------
// VERSION column
static void drawVersionTooltip(const TableData& t) {
    ImGui::Text("Metadata: %s", t.tableVersion.empty() ? "-" : t.tableVersion.c_str());
    ImGui::Text("VPSDB: %s (latest)", t.vpsVersion.empty() ? "-" : t.vpsVersion.c_str());
    ImGui::Separator();
    ImGui::Text("Match confidence: %.0f%%", t.matchConfidence * 100.0f);
}

// ------------------------------------------------------------------
// EXTRAS column
static void drawExtrasTooltip(const TableData& t) {
    ImGui::Text("Legend: C=Alt color, S=Alt sound, P=Pup videos, U=UltraDMD, M=Alt music");
    ImGui::Text("Alt Color: %s", t.hasAltColor ? "present" : "-");
    ImGui::Text("Alt Sound: %s", t.hasAltSound ? "present" : "-");
    ImGui::Text("Pup Videos: %s", t.hasPup ? "present" : "-");
    ImGui::Text("UltraDMD: %s", t.hasUltraDMD ? "present" : "-");
    ImGui::Text("Alt music: %s", t.hasAltMusic ? "present" : "-");
}

// ------------------------------------------------------------------
// ASSETS column (Images)
static void drawAssetsTooltip(const TableData& t) {
    ImGui::Text("Legend: P=Playfield, B=Backglass, D=DMD, T=Topper, W=Wheel");
    ImGui::Text("Playfield: %s", t.hasPlayfieldImage ? t.playfieldImage.c_str() : "-");
    ImGui::Text("Backglass: %s", t.hasBackglassImage ? t.backglassImage.c_str() : "-");
    ImGui::Text("DMD: %s", t.hasDmdImage ? t.dmdImage.c_str() : "-");
    ImGui::Text("Topper: %s", t.hasTopperImage ? t.topperImage.c_str() : "-");
    ImGui::Text("Wheel: %s", t.hasWheelImage ? t.wheelImage.c_str() : "-");
}

// ------------------------------------------------------------------
// VIDEOS column
static void drawVideosTooltip(const TableData& t) {
    ImGui::Text("Legend: P=Playfield, B=Backglass, D=DMD, T=Topper");
    ImGui::Text("Playfield: %s", t.hasPlayfieldVideo ? t.playfieldVideo.c_str() : "-");
    ImGui::Text("Backglass: %s", t.hasBackglassVideo ? t.backglassVideo.c_str() : "-");
    ImGui::Text("DMD: %s", t.hasDmdVideo ? t.dmdVideo.c_str() : "-");
    ImGui::Text("Topper: %s", t.hasTopperVideo ? t.topperVideo.c_str() : "-");
}

// ------------------------------------------------------------------
// SOUNDS column
static void drawSoundsTooltip(const TableData& t) {
    ImGui::Text("Legend: M=Front-end Music, L=Launch audio");
    ImGui::Text("Table music: %s", t.hasTableMusic ? t.music.c_str() : "-");
    ImGui::Text("Launch audio: %s", t.hasLaunchAudio ? t.launchAudio.c_str() : "-");
}
// ------------------------------------------------------------------

// Main body drawing function
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

            if (ImGui::BeginTable("table_list", 13, flags, tableSize)) {
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
                ImGui::TableSetupColumn("Patched",     ImGuiTableColumnFlags_WidthFixed,   30.0f,11);
                ImGui::TableSetupColumn("Broken",      ImGuiTableColumnFlags_WidthFixed,   30.0f,12);

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
                    // Pick correct values by scanner with fallback to best match
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

                    // ================================ COLUMNS =================================
                    // ----------------------------------------- YEAR
                    {ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(displayYear.c_str());
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                        drawTooltipForColumn(0, t, ui);}

                    // ----------------------------------------- NAME
                    {ImGui::TableSetColumnIndex(1);

                    ImGui::PushID(i);
                    bool isSelected = (ui.selectedIndex() == i);
                    if (t.isBroken) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                    if (ImGui::Selectable(displayName.c_str(), isSelected,
                                          ImGuiSelectableFlags_SpanAllColumns)) {
                        ui.setSelectedIndex(isSelected ? -1 : i);
                        ui.setScrollToSelected(false);
                    }
                    if (t.isBroken) {
                        ImGui::PopStyleColor();
                    }
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(1, t, ui);
                    ImGui::PopID();}

                    // ----------------------------------------- VERSION
                    {ImGui::TableSetColumnIndex(2);

                    if (!t.tableVersion.empty()) {
                        ImGui::TextUnformatted(t.tableVersion.c_str());
                    } else {
                        ImGui::TextUnformatted("");
                    }
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(2, t, ui);}

                    // ----------------------------------------- AUTHOR
                    {ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(displayAuthor.c_str());
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(3, t, ui);}

                    // ----------------------------------------- MANUFACTURER
                    {ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(displayManufacturer.c_str());
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(4, t, ui);}

                    // ----------------------------------------- Extra Files
                    {ImGui::TableSetColumnIndex(5);

                    // Start text line
                    if (t.hasINI)
                        ImGui::TextUnformatted("I ");
                    else
                        ImGui::TextUnformatted("- ");
                    // Keep same line for next text
                    ImGui::SameLine(0, 0);
                    // "V" in yellow if t.hasDiff, otherwise normal
                    if (t.hasVBS) {
                        if (t.hasDiffVbs)
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "V ");
                        else
                            ImGui::TextUnformatted("V ");
                    } else {
                        ImGui::TextUnformatted("- ");
                    }
                    // Same line again for "B"
                    ImGui::SameLine(0, 0);
                    if (t.hasB2S)
                        ImGui::TextUnformatted("B ");
                    else
                        ImGui::TextUnformatted("- ");
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(5, t, ui);}

                    // ----------------------------------------- ROM Name
                    {ImGui::TableSetColumnIndex(6);

                    if (!t.romName.empty()) {
                        ImGui::TextUnformatted(t.romName.c_str());
                    } else {
                        ImGui::TextUnformatted("");
                    }
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(6, t, ui);}

                    // ----------------------------------------- Media Extras
                    {ImGui::TableSetColumnIndex(7);

                    ImGui::Text("%s%s%s%s%s",
                                t.hasAltSound ? "S " : "- ",
                                t.hasAltColor ? "C " : "- ",
                                t.hasPup ? "P " : "- ",
                                t.hasUltraDMD ? "U " : "- ",
                                t.hasAltMusic ? "M " : "- ");
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(7, t, ui);}

                    // ----------------------------------------- Media Assets - Images
                    {ImGui::TableSetColumnIndex(8);

                    ImGui::Text("%s%s%s%s%s",
                                t.hasPlayfieldImage ? "P " : "- ",
                                t.hasBackglassImage ? "B " : "- ",
                                t.hasDmdImage ? "D " : "- ",
                                t.hasTopperImage ? "T " : "- ",
                                t.hasWheelImage ? "W " : "- ");
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(8, t, ui);}

                    // ----------------------------------------- Media Assets - Videos
                    {ImGui::TableSetColumnIndex(9);

                    ImGui::Text("%s%s%s%s",
                                t.hasPlayfieldVideo ? "P " : "- ",
                                t.hasBackglassVideo ? "B " : "- ",
                                t.hasDmdVideo ? "D " : "- ",
                                t.hasTopperVideo ? "T " : "- ");
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(9, t, ui);}

                    // ----------------------------------------- Media Assets - Sounds
                    {ImGui::TableSetColumnIndex(10);

                    ImGui::Text("%s%s",
                                t.hasTableMusic ? "M " : "- ",
                                t.hasLaunchAudio ? "L " : "- ");
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(10, t, ui);}

                    // ----------------------------------------- Patch Status
                    {ImGui::TableSetColumnIndex(11);

                        if (t.isPatched) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 1.0f, 0.20f, 1.0f)); // green
                            ImGui::Text(" P ");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.60f, 1.0f)); // gray
                            ImGui::Text(" - ");
                            ImGui::PopStyleColor();
                        }
                    }

                    // ----------------------------------------- Launch Status
                    {ImGui::TableSetColumnIndex(12);

                        if (t.isBroken) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.20f, 0.20f, 1.0f)); // red
                            ImGui::Text(" B ");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.60f, 1.0f)); // gray
                            if (t.playCount >= 1) {
                                ImGui::Text(" ok ");
                            } else {
                                ImGui::Text(" - ");
                            }
                            ImGui::PopStyleColor();
                        }
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }
}
}
