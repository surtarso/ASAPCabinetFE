#include "editor/editor_ui.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

namespace Tooltips {
    // Define the static constant map to hold button tooltips
    // std::string_view for keys and values for better performance and memory management
    static const std::unordered_map<std::string, std::string> BUTTON_TOOLTIPS = {
        {"Exit Editor", "Close the Editor"},
        {"Rescan Tables", "Rescan the table folder and refresh the list."},
        {"Play Selected", "Launch the current selection in VPinballX."},
        {"Extract VBS", "Extract the VBS script from the selected table.\nOpen the script in external editor if already extracted."},
        {"Open Folder", "Open the table folder.\nOpen tables root folder if no table selected."},
        {"INI Editor", "Create or open selected table configuration files.\nOpen vpinballx.ini if no table selected."},
        {"View Metadata", "View detailed metadata for the selected table."},
        {"Apply Patch", "Apply community patches to the selected table.\nApply to all tables if none selected."},
        {"Download Media", "Download images for selected table.\nDownloads for all tables if none selected."},
        {"Screenshot", "Take a screenshot of the selected table.\nTakes screenshots for all tables if none selected."},
        {"Browse Tables", "Open Virtual Pinball Spreadsheet database browser.\nBrowse and download tables from VPSDB."}
    };
}

void EditorUI::filterAndSortTables() {
    // Filtering, sorting, and selection logic
    tableFilter_.filterAndSort(tables_, filteredTables_, searchQuery_,
                               sortColumn_, sortAscending_, selectedIndex_);
}

// Constructor already used the loader to fill tables_ originally.
// Keep same behavior: read index once and store.
EditorUI::EditorUI(IConfigService *config, ITableLoader *tableLoader, ITableLauncher *launcher,
                   bool &showMeta, bool &showIni, bool &showBrowser)
    : config_(config),
      tableLoader_(tableLoader),
      tableLauncher_(launcher),
      actions_(config),
      selectedIndex_(-1),
      showMetadataEditor_(showMeta),
      showIniEditor_(showIni),
      showVpsdbBrowser_(showBrowser) {

    Settings settings = config_->getSettings();
    settings.ignoreScanners = true; // ignore scanners on start (not persisted)
    tables_ = tableLoader_->loadTableList(settings, nullptr);

    filterAndSortTables(); // Apply default sort on load
}

// Draw editor UI embedded in the main window
void EditorUI::draw() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("ASAPCabinetFE Editor", nullptr, windowFlags);

    // --------------------------- Top search bar ---------------------------
    // Detect typing and auto-focus search field
    actions_.handleKeyboardSearchFocus(
    searchBuffer_,
    searchQuery_,
    [this]() { filterAndSortTables(); },
    [this]() {
        // Only launch if a table is actually selected
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(filteredTables_.size())) {
            const auto& t = filteredTables_[selectedIndex_];
            tableLauncher_->launchTable(t);
        } else {
            LOG_DEBUG("Enter pressed but no table selected");
        }
    });


    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 2.5f);
    if (ImGui::InputTextWithHint("##SearchInputTop", "Search by Name, File, or ROM",
                                searchBuffer_, sizeof(searchBuffer_))) {
        searchQuery_ = searchBuffer_;
        filterAndSortTables();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Clear##TopSearch")) {
        searchBuffer_[0] = '\0';
        searchQuery_.clear();
        filterAndSortTables();
    }

    std::lock_guard<std::mutex> lock(tableMutex_);

    if (loading_) {
        ImGui::Text("Scanning tables... (please wait)");
        ImGui::End();
        return;
    }

    if (tables_.empty()) {
        ImGui::TextDisabled("No tables found. Run a rescan from the main frontend.");
        ImGui::End();
        return;
    }

    // --------------------------- Spreadsheet region ---------------------------
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.0f; // room for buttons
    ImVec2 tableSize(avail.x, avail.y - footerHeight);

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_BordersOuter |
                            ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_Reorderable |
                            ImGuiTableFlags_Hideable |
                            ImGuiTableFlags_Sortable;

    ImGui::BeginChild("TableContainer", tableSize, false, ImGuiWindowFlags_NoScrollbar);

    // Check for no results: display message instead of table, but DON'T return.
    if (filteredTables_.empty() && !tables_.empty()) {
        ImGui::TextDisabled("No tables match the current filter: '%s'", searchQuery_.c_str());
    } else if (!filteredTables_.empty() && ImGui::BeginTable("table_list", 11, flags, tableSize)) {

        ImGui::TableSetupScrollFreeze(0, 1);

        // CRUCIAL: The 4th argument is the unique ID (user_id) used by the sorting logic (case 0, case 1, etc.)
        ImGui::TableSetupColumn("Year", ImGuiTableColumnFlags_WidthFixed, 30.0f, 0); // ID 0
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1); // ID 1
        ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 75.0f, 2); // ID 2
        ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthFixed, 100.0f, 3); // ID 3
        ImGui::TableSetupColumn("Manufacturer", ImGuiTableColumnFlags_WidthFixed, 80.0f, 4); // ID 4
        ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthFixed, 45.0f, 5);            // ID 5
        ImGui::TableSetupColumn("ROM", ImGuiTableColumnFlags_WidthFixed, 75.0f, 6); // ID 6
        ImGui::TableSetupColumn("Extras", ImGuiTableColumnFlags_WidthFixed, 75.0f, 7); // ID 7
        ImGui::TableSetupColumn("Images", ImGuiTableColumnFlags_WidthFixed, 75.0f, 8); // ID 8
        ImGui::TableSetupColumn("Videos", ImGuiTableColumnFlags_WidthFixed, 55.0f, 9); // ID 9
        ImGui::TableSetupColumn("Sounds", ImGuiTableColumnFlags_WidthFixed, 30.0f, 10); // ID 10

        ImGui::TableHeadersRow();

        // --- INLINE SORT HOOK: This is the standard ImGui pattern ---
        if (ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty) {
                const ImGuiTableColumnSortSpecs *spec = sortSpecs->Specs;

                // 1. Update internal state
                sortColumn_ = spec->ColumnUserID;
                sortAscending_ = (spec->SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;

                // 2. Perform the filtering and sorting again.
                // Since the sortColumn_ and sortAscending_ members have just been updated,
                // calling filterAndSortTables() will re-sort the currently filtered list
                // based on the new criteria (while maintaining the existing search filter).
                filterAndSortTables();
            }
        }

        for (int i = 0; i < (int)filteredTables_.size(); ++i) {
            const auto &t = filteredTables_[i];
            ImGui::TableNextRow();

            // --- Normalized metadata ---
            // Year: prefer vpsYear, then tableYear, then file-derived year.
            std::string displayYear =
                !t.vpsYear.empty() ? t.vpsYear : !t.tableYear.empty() ? t.tableYear
                                             : !t.year.empty()        ? t.year
                                                                      : "-";

            // Name: prefer vpsName, then tableName, then filename/title.
            std::string displayName =
                !t.vpsName.empty() ? t.vpsName : !t.tableName.empty() ? t.tableName
                                             : !t.title.empty()       ? t.title
                                                                      : "-";

            // Author: prefer vpsAuthors, then tableAuthor.
            std::string displayAuthor =
                !t.vpsAuthors.empty() ? t.vpsAuthors : !t.tableAuthor.empty() ? t.tableAuthor
                                                                              : "-";

            // Manufacturer: prefer vpsManufacturer, then tableManufacturer, then manufacturer.
            std::string displayManufacturer =
                !t.vpsManufacturer.empty() ? t.vpsManufacturer : !t.tableManufacturer.empty() ? t.tableManufacturer
                                                             : !t.manufacturer.empty()        ? t.manufacturer
                                                                                              : "-";

            // Column 0: Year
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(displayYear.c_str());

            // Column 1: Name (main selectable)
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            bool isSelected = (selectedIndex_ == i);
            if (ImGui::Selectable(displayName.c_str(), isSelected,
                                  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex_ = isSelected ? -1 : i;
                // remove scroll-to-center behavior
                scrollToSelected_ = false;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    LOG_DEBUG(std::string("Row double-click (placeholder) for: ") + displayName + " -> " + t.vpxFile);
                }
            }
            ImGui::PopID();

            // Column 2: Version
            ImGui::TableSetColumnIndex(2);
            if (!t.tableVersion.empty()) {
                ImGui::TextUnformatted(t.tableVersion.c_str());
            } else {
                ImGui::TextUnformatted("");
            }
            // Column 2: Author
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(displayAuthor.c_str());

            // Column 3: Manufacturer
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(displayManufacturer.c_str());

            // Column 4: INI + VBS + B2S existence (TODO: collect these vars (only added in table_data.h))
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s%s%s",
                        t.hasINI ? "I " : "- ", // todo: add check for ini diff and recolor
                        t.hasVBS ? "V " : "- ", // todo: color if t.hasDiffVbs
                        t.hasB2S ? "B " : "- ");

            // Column 5: ROM name (shows name or empty)
            ImGui::TableSetColumnIndex(6);
            if (!t.romName.empty()) {
                ImGui::TextUnformatted(t.romName.c_str());
            } else {
                ImGui::TextUnformatted("");
            }
            // Column 6: Alt/Color/PUP/UDMD/Music
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%s%s%s%s%s",
                        t.hasAltSound ? "S " : "- ",
                        t.hasAltColor ? "C " : "- ",
                        t.hasPup ? "P " : "- ",
                        t.hasUltraDMD ? "U " : "- ",
                        t.hasAltMusic ? "M " : "- ");

            // Column 7: Images
            ImGui::TableSetColumnIndex(8);
            ImGui::Text("%s%s%s%s%s",
                        t.hasPlayfieldImage ? "P " : "- ",
                        t.hasBackglassImage ? "B " : "- ",
                        t.hasDmdImage ? "D " : "- ",
                        t.hasTopperImage ? "T " : "- ",
                        t.hasWheelImage ? "W " : "- ");

            // Column 8: Videos
            ImGui::TableSetColumnIndex(9);
            ImGui::Text("%s%s%s%s",
                        t.hasPlayfieldVideo ? "P " : "- ",
                        t.hasBackglassVideo ? "B " : "- ",
                        t.hasDmdVideo ? "D " : "- ",
                        t.hasTopperVideo ? "T " : "- ");

            // Column 9: Sounds
            ImGui::TableSetColumnIndex(10);
            ImGui::Text("%s%s",
                        t.hasTableMusic ? "M " : "- ",
                        t.hasLaunchAudio ? "L " : "- ");
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::Separator();

    // --- Last scan info (above buttons) ---
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size()) {
        const auto &t = filteredTables_[selectedIndex_];
        if (!t.jsonOwner.empty()) {
            ImGui::TextDisabled("Last table scanner: %s", t.jsonOwner.c_str());
        }
    }

    // --------------------------- Footer buttons ---------------------------
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() * 2.0f);

    ImGui::BeginGroup(); // Start Button Group

    {
        // Build label showing current mode
        const char* modeLabel = (selectedScanner_ == ScannerMode::File) ? "File" :
                                (selectedScanner_ == ScannerMode::VPin) ? "VPin" : "VPSDb";
        std::string comboLabel = std::string("Rescan (") + modeLabel + ")";

        // Define the custom color (e.g., Orange, represented as R, G, B, A floats 0.0 to 1.0)
        // Base Purple/Indigo
        ImVec4 customColor = ImVec4(0.35f, 0.20f, 0.55f, 1.0f);
        // Lighter for hover
        ImVec4 hoveredColor = ImVec4(0.45f, 0.30f, 0.65f, 1.0f);
        // Darker for active click
        ImVec4 activeColor = ImVec4(0.25f, 0.15f, 0.40f, 1.0f);

        // 1. Push the new colors onto the style stack
        ImGui::PushStyleColor(ImGuiCol_Button, customColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

        // Use a combo so it behaves like a single control and shows the selection
        if (ImGui::BeginCombo("##rescan_combo", comboLabel.c_str(), ImGuiComboFlags_NoPreview)) {
            ImGui::TextDisabled("Scanner Mode");
            if (ImGui::Selectable("File Scanner", selectedScanner_ == ScannerMode::File))
                selectedScanner_ = ScannerMode::File;
            if (ImGui::Selectable("VPin Scanner", selectedScanner_ == ScannerMode::VPin))
                selectedScanner_ = ScannerMode::VPin;
            if (ImGui::Selectable("VPSDb Scanner", selectedScanner_ == ScannerMode::VPSDb))
                selectedScanner_ = ScannerMode::VPSDb;

            ImGui::TextDisabled("Options");
            // Clear labels, use standard checkboxes for clarity
            ImGui::Checkbox("Force Rebuild Metadata", &forceRebuildMetadata_);
            ImGui::Checkbox("Use External VPXTool", &useVpxtool_);

            ImGui::EndCombo();
        }
        // Keep a standalone action button
        ImGui::SameLine();
        if (ImGui::Button("Rescan Tables")) {
            rescanAsync(selectedScanner_);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Rescan Tables").c_str());
        }
        ImGui::PopStyleColor(3); // Pop the 3 colors
    }

    ImGui::SameLine();

    if (ImGui::Button("Open Folder")) {
        std::string path;
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
            path = filteredTables_[selectedIndex_].vpxFile;
        actions_.openFolder(path);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Open Folder").c_str());
    }
    ImGui::SameLine();

    // color next buttons in yellow theme (operational actions)
    ImVec4 yellow = ImVec4(0.8f, 0.7f, 0.3f, 0.85f);     // Soft Golden Yellow
    ImVec4 yellowHover = ImVec4(0.9f, 0.8f, 0.4f, 0.85f); // Lighter for hover
    ImVec4 yellowActive = ImVec4(0.7f, 0.6f, 0.2f, 0.85f); // Darker for active

    ImGui::PushStyleColor(ImGuiCol_Button, yellow);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, yellowHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, yellowActive);

    if (ImGui::Button("Extract VBS")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size()) {
            const auto &t = filteredTables_[selectedIndex_];
            // 1. Extract the VBS
            actions_.extractVBS(t.vpxFile); // <-- USE ACTION

            // 2. Build the path to the new .vbs file
            fs::path vpxPath(t.vpxFile);
            std::string vbsPath = (vpxPath.parent_path() / (vpxPath.stem().string() + ".vbs")).string();

            // 3. Open it
            if (fs::exists(vbsPath)) {
                actions_.openInExternalEditor(vbsPath); // <-- USE ACTION
            } else {
                LOG_ERROR("Tried to open VBS, but extraction failed or file not found at: " + vbsPath);
            }
        } else {
            LOG_DEBUG("Extract VBS pressed but no table selected");
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Extract VBS").c_str());
    }
    ImGui::SameLine();

    if (ImGui::Button("INI Editor")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size()) {
            // This will now be handled by the main editor loop
            showIniEditor_ = true; // <-- TOGGLE STATE
            LOG_DEBUG("Toggling INI editor ON");
        } else {
            LOG_DEBUG("INI Editor pressed but no table selected");
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("INI Editor").c_str());
    }
    ImGui::SameLine();

    if (ImGui::Button("Apply Patch")) {
        LOG_DEBUG("Apply Patch pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Apply Patch").c_str());
    }
    ImGui::SameLine();

    // pop yellow colors (operational actions end)
    ImGui::PopStyleColor(3); // Pop 3 colors (Button, Hovered, Active)

    if (ImGui::Button("Download Media")) {
        LOG_DEBUG("Download Media pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Download Media").c_str());
    }
    ImGui::SameLine();

    if (ImGui::Button("Screenshot")) {
        LOG_DEBUG("Screenshot pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Screenshot").c_str());
    }
    ImGui::SameLine();

    if (ImGui::Button("View Metadata")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size()) {
            // This will now be handled by the main editor loop
            showMetadataEditor_ = true; // <-- TOGGLE STATE
            LOG_DEBUG("Toggling metadata editor ON");
        } else {
            LOG_DEBUG("View Metadata pressed but no table selected");
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("View Metadata").c_str());
    }
    ImGui::SameLine();

    if (ImGui::Button("Browse Tables")) {
        // This will now be handled by the main editor loop
        showVpsdbBrowser_ = true; // <-- TOGGLE STATE
        LOG_DEBUG("Browse Tables pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Browse Tables").c_str());
    }
    ImGui::SameLine();

    // color play button green
    ImVec4 green = ImVec4(0.2f, 0.7f, 0.2f, 1.0f); // Standard Green
    ImVec4 greenHover = ImVec4(0.3f, 0.8f, 0.3f, 1.0f); // Lighter for hover
    ImVec4 greenActive = ImVec4(0.1f, 0.6f, 0.1f, 1.0f); // Darker for active

    ImGui::PushStyleColor(ImGuiCol_Button, green);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, greenHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, greenActive);

    if (ImGui::Button("Play Selected")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size()) {
            const auto &t = filteredTables_[selectedIndex_];
            tableLauncher_->launchTable(t);
        } else {
            LOG_DEBUG("Play pressed but no table selected");
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Play Selected").c_str());
    }
    ImGui::PopStyleColor(3); // Pop 3 colors (Button, Hovered, Active)

    // Exit button always on far right, red colored
    float exitWidth = ImGui::CalcTextSize("Exit Editor").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float rightAlignPos = ImGui::GetContentRegionAvail().x - exitWidth;
    ImGui::SameLine(rightAlignPos);

    // push red shades
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Exit Editor")) {
        exitRequested_ = true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Exit Editor").c_str());
    }

    ImGui::PopStyleColor(3); // restore original colors

    ImGui::EndGroup(); // End Button Group

    // --------------------------- Footer info ---------------------------
    std::ostringstream ss;
    ss << filteredTables_.size() << " tables found";
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size()) {
        const auto &t = filteredTables_[selectedIndex_];
        fs::path p(t.vpxFile);
        ss << "  |  Selected: /" << p.parent_path().filename().string()
           << "/" << p.filename().string();
    }
    ImGui::TextDisabled("%s", ss.str().c_str());

    ImGui::End();
}

void EditorUI::rescanAsync(ScannerMode mode) {
    if (loading_) return;
    loading_ = true;

    std::thread([this, mode]() {
        Settings settings = config_->getSettings();
        settings.ignoreScanners = false;
        settings.forceRebuildMetadata = forceRebuildMetadata_;
        settings.useVpxtool = useVpxtool_;

        switch (mode) {
            case ScannerMode::File:
                settings.titleSource = "filename";
                settings.fetchVPSdb = false;
                break;
            case ScannerMode::VPin:
                settings.titleSource = "metadata";
                settings.fetchVPSdb = false;
                break;
            case ScannerMode::VPSDb:
                settings.titleSource = "metadata";
                settings.fetchVPSdb = true;
                break;
        }

        auto newTables = tableLoader_->loadTableList(settings, nullptr);
        {
            std::lock_guard<std::mutex> lock(tableMutex_);
            tables_ = std::move(newTables);
            filterAndSortTables();
        }
        loading_ = false;
    }).detach();
}
