#include "editor/ui/editor_body.h"
#include "editor/editor_first_run.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

namespace editor_body {

// Forward declarations for all tooltip helpers
static void drawYearTooltip(const TableData& t);
static void drawNameTooltip(const TableData& t);
static void drawTypeTooltip(const TableData& t);
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
        case 2: drawTypeTooltip(t); break;
        case 3: drawVersionTooltip(t); break;
        case 4: drawAuthorTooltip(t); break;
        case 5: drawManufacturerTooltip(t); break;
        case 6: drawFilesTooltip(t); break;
        case 7: drawRomTooltip(t); break;
        case 8: drawExtrasTooltip(t); break;
        case 9: drawAssetsTooltip(t); break;
        case 10: drawVideosTooltip(t); break;
        case 11: drawSoundsTooltip(t); break;
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
// TYPE column
static void drawTypeTooltip(const TableData& t) {
    ImGui::Text("Metadata: %s", t.tableType.empty() ? "-" : t.tableType.c_str());
    ImGui::Text("VPSDB: %s", t.vpsType.empty() ? "-" : t.vpsType.c_str());
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
    ImGui::Text("Legend: I=INI, V=VBS\n  B=B2S O=Override");
    ImGui::TextColored(t.hasINI ? (t.hasDiffVbs ? ImVec4(1.f,1.f,0.f,1.f) : ImVec4(0.8f,0.8f,0.8f,1.f))
                                : ImVec4(0.5f,0.5f,0.5f,1.f),
                       "INI: %s", t.hasINI ? (t.hasDiffVbs ? "modified" : "present") : "-");
    ImGui::TextColored(t.hasVBS ? (t.hasDiffVbs ? ImVec4(1.f,1.f,0.f,1.f) : ImVec4(0.8f,0.8f,0.8f,1.f))
                                : ImVec4(0.5f,0.5f,0.5f,1.f),
                       "VBS: %s", t.hasVBS ? (t.hasDiffVbs ? "modified" : "present") : "-");
    ImGui::Text("B2S: %s", t.hasB2S ? "present" : "-");
    ImGui::Text("Override: %s", t.hasOverride ? "present" : "-");
}

// ------------------------------------------------------------------
// ROM column
static void drawRomTooltip(const TableData& t) {
    ImGui::Text("Local: %s", t.romName.empty() ? "-" : t.romName.c_str());
    ImGui::Text("Metadata: %s", t.tableRom.empty() ? "-" : t.tableRom.c_str());
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
    ImGui::Text("Legend: P=Playfield, B=Backglass, D=DMD, T=Topper\nW=Wheel, F=Flyer (front), Fb=Flyer(Back)");
    ImGui::Text("Playfield: %s", t.hasPlayfieldImage ? t.playfieldImage.c_str() : "-");
    ImGui::Text("Backglass: %s", t.hasBackglassImage ? t.backglassImage.c_str() : "-");
    ImGui::Text("DMD: %s", t.hasDmdImage ? t.dmdImage.c_str() : "-");
    ImGui::Text("Topper: %s", t.hasTopperImage ? t.topperImage.c_str() : "-");
    ImGui::Text("Wheel: %s", t.hasWheelImage ? t.wheelImage.c_str() : "-");
    ImGui::Text("Flyer (front): %s", t.hasFlyerFront ? t.flyerFront.c_str() : "-");
    ImGui::Text("Flyer (back): %s", t.hasFlyerBack ? t.flyerBack.c_str() : "-");
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

            if (ImGui::BeginTable("table_list", 14, flags, tableSize)) {
                ImGui::TableSetupScrollFreeze(0,1);
                ImGui::TableSetupColumn("Year",        ImGuiTableColumnFlags_WidthFixed,  30.0f, 0);
                ImGui::TableSetupColumn("Name",        ImGuiTableColumnFlags_WidthStretch,  0.0f,1);
                ImGui::TableSetupColumn("Type",        ImGuiTableColumnFlags_WidthFixed,   20.0f,2);
                ImGui::TableSetupColumn("Version",     ImGuiTableColumnFlags_WidthFixed,   75.0f,3);
                ImGui::TableSetupColumn("Author",      ImGuiTableColumnFlags_WidthFixed,  100.0f,4);
                ImGui::TableSetupColumn("Manufacturer",ImGuiTableColumnFlags_WidthFixed,   80.0f,5);
                ImGui::TableSetupColumn("Files",       ImGuiTableColumnFlags_WidthFixed,   50.0f,6);
                ImGui::TableSetupColumn("ROM",         ImGuiTableColumnFlags_WidthFixed,   70.0f,7);
                ImGui::TableSetupColumn("Extras",      ImGuiTableColumnFlags_WidthFixed,   65.0f,8);
                ImGui::TableSetupColumn("Images",      ImGuiTableColumnFlags_WidthFixed,   100.0f,9);
                ImGui::TableSetupColumn("Videos",      ImGuiTableColumnFlags_WidthFixed,   55.0f,10);
                ImGui::TableSetupColumn("Sounds",      ImGuiTableColumnFlags_WidthFixed,   30.0f,11);
                ImGui::TableSetupColumn("Patched",     ImGuiTableColumnFlags_WidthFixed,   30.0f,12);
                ImGui::TableSetupColumn("Broken",      ImGuiTableColumnFlags_WidthFixed,   30.0f,13);

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

                    std::string displayType = !t.vpsType.empty() ? t.vpsType
                                             : !t.tableType.empty() ? t.tableType
                                             : "-";

                    std::string displayAuthor = !t.vpsAuthors.empty() ? t.vpsAuthors
                                             : !t.tableAuthor.empty() ? t.tableAuthor
                                             : "-";

                    std::string displayManufacturer = !t.vpsManufacturer.empty() ? t.vpsManufacturer
                                                   : !t.tableManufacturer.empty() ? t.tableManufacturer
                                                   : !t.manufacturer.empty() ? t.manufacturer
                                                   : "-";

                    // ================================ Row colors ================================
                    // ----------- Last Win Schema --------
                    // green row for known good tables
                    if (!t.isBroken && t.playCount >= 1) {
                        ImGui::TableSetBgColor(
                            ImGuiTableBgTarget_RowBg0,
                            IM_COL32(120, 255, 120, 40)   // green but very transparent
                        );
                    }
                    // red row for broken tables
                    if (t.isBroken) {
                        ImGui::TableSetBgColor(
                            ImGuiTableBgTarget_RowBg0,
                            IM_COL32(255, 120, 120, 40)   // red but very transparent
                        );
                    }

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

                    // Check if we need special coloring
                    bool usePurple = (t.matchConfidence == 0.0f);

                    // Light / faint purple (readable)
                    ImVec4 faintPurple(0.70f, 0.55f, 1.00f, 1.00f); // tweak if needed

                    int colorStack = 0;
                    if (usePurple) {
                        ImGui::PushStyleColor(ImGuiCol_Text, faintPurple);
                        colorStack++;
                    }

                    bool isSelected = (ui.selectedIndex() == i);
                    if (ImGui::Selectable(displayName.c_str(), isSelected,
                                        ImGuiSelectableFlags_SpanAllColumns)) {
                        ui.setSelectedIndex(isSelected ? -1 : i);
                        ui.setScrollToSelected(false);
                    }

                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                        drawTooltipForColumn(1, t, ui);

                    // Pop color if applied
                    while (colorStack-- > 0)
                        ImGui::PopStyleColor();

                    ImGui::PopID();}

                    // ----------------------------------------- TYPE
                    {ImGui::TableSetColumnIndex(2);

                    ImGui::TextUnformatted(displayType.c_str());

                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(2, t, ui);}

                    // ----------------------------------------- VERSION
                    {ImGui::TableSetColumnIndex(3);

                    if (!t.tableVersion.empty()) {
                        ImGui::TextUnformatted(t.tableVersion.c_str());
                    } else {
                        ImGui::TextUnformatted("");
                    }
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(3, t, ui);}

                    // ----------------------------------------- AUTHOR
                    {ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(displayAuthor.c_str());
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(4, t, ui);}

                    // ----------------------------------------- MANUFACTURER
                    {ImGui::TableSetColumnIndex(5);
                    ImGui::TextUnformatted(displayManufacturer.c_str());
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(5, t, ui);}

                    // ----------------------------------------- Extra Files
                    {ImGui::TableSetColumnIndex(6);

                    // Start text line
                    if (t.hasINI)
                        ImGui::TextUnformatted("I ");
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                        ImGui::TextUnformatted("- ");
                        ImGui::PopStyleColor();
                    }
                    // Keep same line for next text
                    ImGui::SameLine(0, 0);
                    // "V" in yellow if t.hasDiff, otherwise normal
                    if (t.hasVBS) {
                        if (t.hasDiffVbs)
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "V ");
                        else
                            ImGui::TextUnformatted("V ");
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                        ImGui::TextUnformatted("- ");
                        ImGui::PopStyleColor();
                    }
                    // Same line again for "B"
                    ImGui::SameLine(0, 0);
                    if (t.hasB2S)
                        ImGui::TextUnformatted("B ");
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                        ImGui::TextUnformatted("- ");
                        ImGui::PopStyleColor();
                    }
                    // Same line again for "O"
                    ImGui::SameLine(0, 0);
                    if (t.hasOverride)
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "O "); //yellow
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                        ImGui::TextUnformatted("- ");
                        ImGui::PopStyleColor();
                    }
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(6, t, ui);}


                    // ----------------------------------------- ROM Name
                    {ImGui::TableSetColumnIndex(7);

                    if (!t.romName.empty()) {
                        ImGui::TextUnformatted(t.romName.c_str());
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                        ImGui::TextUnformatted("");
                        ImGui::PopStyleColor();
                    }
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    if (ImGui::IsMouseHoveringRect(min, max))
                       drawTooltipForColumn(7, t, ui);}

                    // ----------------------------------------- Media Extras
                    {
                        ImGui::TableSetColumnIndex(8);

                        // We print each symbol separately, pushing gray ONLY for "-"
                        auto printFlag = [](bool flag, const char* letter)
                        {
                            if (flag) {
                                ImGui::TextUnformatted(letter);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                                ImGui::TextUnformatted("- ");
                                ImGui::PopStyleColor();
                            }
                            ImGui::SameLine(0, 0); // keep continuous spacing
                        };

                        printFlag(t.hasAltSound, "S ");
                        printFlag(t.hasAltColor, "C ");
                        printFlag(t.hasPup,      "P ");
                        printFlag(t.hasUltraDMD, "U ");
                        printFlag(t.hasAltMusic, "M ");

                        ImGui::NewLine(); // finish the row

                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        if (ImGui::IsMouseHoveringRect(min, max))
                            drawTooltipForColumn(8, t, ui);
                    }


                    // ----------------------------------------- Media Assets - Images
                    {
                        ImGui::TableSetColumnIndex(9);

                        auto printFlag = [](bool flag, const char* letter)
                        {
                            if (flag) {
                                ImGui::TextUnformatted(letter);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                                ImGui::TextUnformatted("- ");
                                ImGui::PopStyleColor();
                            }
                            ImGui::SameLine(0, 0);
                        };

                        printFlag(t.hasPlayfieldImage, "P ");
                        printFlag(t.hasBackglassImage, "B ");
                        printFlag(t.hasDmdImage,       "D ");
                        printFlag(t.hasTopperImage,    "T ");
                        printFlag(t.hasWheelImage,     "W ");
                        printFlag(t.hasFlyerFront,     "F ");
                        printFlag(t.hasFlyerBack,      "Fb");

                        ImGui::NewLine();

                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        if (ImGui::IsMouseHoveringRect(min, max))
                            drawTooltipForColumn(9, t, ui);
                    }


                    // ----------------------------------------- Media Assets - Videos
                    {
                        ImGui::TableSetColumnIndex(10);

                        auto printFlag = [](bool flag, const char* letter)
                        {
                            if (flag) {
                                ImGui::TextUnformatted(letter);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                                ImGui::TextUnformatted("- ");
                                ImGui::PopStyleColor();
                            }
                            ImGui::SameLine(0, 0);
                        };

                        printFlag(t.hasPlayfieldVideo, "P ");
                        printFlag(t.hasBackglassVideo, "B ");
                        printFlag(t.hasDmdVideo,       "D ");
                        printFlag(t.hasTopperVideo,    "T ");

                        ImGui::NewLine();

                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        if (ImGui::IsMouseHoveringRect(min, max))
                            drawTooltipForColumn(10, t, ui);
                    }


                    // ----------------------------------------- Media Assets - Sounds
                    {
                        ImGui::TableSetColumnIndex(11);

                        // Same helper used in the other columns
                        auto printFlag = [](bool flag, const char* letter)
                        {
                            if (flag) {
                                ImGui::TextUnformatted(letter);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                                ImGui::TextUnformatted("- ");
                                ImGui::PopStyleColor();
                            }
                            ImGui::SameLine(0, 0);
                        };

                        printFlag(t.hasTableMusic,  "M ");
                        printFlag(t.hasLaunchAudio, "L ");

                        ImGui::NewLine();

                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        if (ImGui::IsMouseHoveringRect(min, max))
                            drawTooltipForColumn(11, t, ui);
                    }


                    // ----------------------------------------- Patch Status
                    {ImGui::TableSetColumnIndex(12);

                        if (t.isPatched) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 1.0f, 0.20f, 1.0f)); // green
                            ImGui::Text(" P ");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
                            ImGui::Text(" - ");
                            ImGui::PopStyleColor();
                        }
                    }

                    // ----------------------------------------- Launch Status
                    {ImGui::TableSetColumnIndex(13);

                        if (t.isBroken) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.20f, 0.20f, 1.0f)); // red
                            ImGui::Text(" B ");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f)); // gray
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
