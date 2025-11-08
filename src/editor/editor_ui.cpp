#include "editor/editor_ui.h"
#include "log/logging.h"
#include "config/settings.h"
#include <thread>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
    // Helper to compare two values with direction and a unique tie-breaker
    template <typename T>
    bool compareWithTieBreaker(const T& val_a, const T& val_b, bool ascending, const TableData& a, const TableData& b) {
        if (val_a == val_b) {
            // CRITICAL: Stable sort tie-breaker on unique file path
            return a.vpxFile < b.vpxFile;
        }
        return ascending ? (val_a < val_b) : (val_a > val_b);
    }

    // The actual sort logic wrapped in a function for clarity
    void performSort(std::vector<TableData>& tables, int sortColumn, bool sortAscending) {
        std::sort(tables.begin(), tables.end(), [sortColumn, sortAscending](const TableData& a, const TableData& b) {

            // Check if the current sort column is one of the complex text fields
            if (sortColumn == 1 || sortColumn == 3 || sortColumn == 4 || sortColumn == 6) {
                // Name (ID 1) should use 'title'
                if (sortColumn == 1)
                    return compareWithTieBreaker(a.title, b.title, sortAscending, a, b);

                // Author (ID 3) should use 'tableAuthor'
                if (sortColumn == 3)
                    return compareWithTieBreaker(a.tableAuthor, b.tableAuthor, sortAscending, a, b);

                // Manufacturer (ID 4) should use 'manufacturer'
                if (sortColumn == 4)
                    return compareWithTieBreaker(a.manufacturer, b.manufacturer, sortAscending, a, b);

                // ROM (ID 6) should use 'romName'
                if (sortColumn == 6)
                    return compareWithTieBreaker(a.romName, b.romName, sortAscending, a, b);
            }

            // Handle integer and simple string fields
            switch (sortColumn) {
                case 0: return compareWithTieBreaker(a.year, b.year, sortAscending, a, b);
                case 2: return compareWithTieBreaker(a.tableVersion, b.tableVersion, sortAscending, a, b);

                // If the sort column is not a dedicated data field (e.g., Files, Images, Videos),
                // default the primary sort to the Name (title).
                default: return compareWithTieBreaker(a.title, b.title, sortAscending, a, b);
            }
        });
    }
} // namespace

// Constructor already used the loader to fill tables_ originally.
// Keep same behavior: read index once and store.
EditorUI::EditorUI(IConfigService* config, ITableLoader* tableLoader, ITableLauncher* launcher,
                   bool& showMeta, bool& showIni)
    : config_(config),
      tableLoader_(tableLoader),
      tableLauncher_(launcher), // <-- ADD THIS
      actions_(config),
      showMetadataEditor_(showMeta), // <-- ADD THIS
      showIniEditor_(showIni) {      // <-- ADD THIS

    Settings settings = config_->getSettings();
    settings.ignoreScanners = true; // ignore scanners on start (not persisted)
    tables_ = tableLoader_->loadTableList(settings, nullptr);
    // Apply default sort on load
    performSort(tables_, sortColumn_, sortAscending_);
}

// Draw editor UI embedded in the main window
void EditorUI::draw() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("ASAPCabinetFE Editor", nullptr, windowFlags);

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

        // --------- Spreadsheet region ---------
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

    if (ImGui::BeginTable("table_list", 11, flags, tableSize)) {
        ImGui::TableSetupScrollFreeze(0, 1);

        // CRUCIAL: The 4th argument is the unique ID (user_id) used by the sorting logic (case 0, case 1, etc.)
        ImGui::TableSetupColumn("Year", ImGuiTableColumnFlags_WidthFixed, 30.0f, 0);       // ID 0
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);     // ID 1
        ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 75.0f, 2);  // ID 2
        ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthFixed, 100.0f, 3);   // ID 3
        ImGui::TableSetupColumn("Manufacturer", ImGuiTableColumnFlags_WidthFixed, 80.0f, 4); // ID 4
        ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthFixed, 45.0f, 5);      // ID 5
        ImGui::TableSetupColumn("ROM", ImGuiTableColumnFlags_WidthFixed, 75.0f, 6);       // ID 6
        ImGui::TableSetupColumn("Extras", ImGuiTableColumnFlags_WidthFixed, 75.0f, 7);     // ID 7
        ImGui::TableSetupColumn("Images", ImGuiTableColumnFlags_WidthFixed, 75.0f, 8);     // ID 8
        ImGui::TableSetupColumn("Videos", ImGuiTableColumnFlags_WidthFixed, 55.0f, 9);     // ID 9
        ImGui::TableSetupColumn("Sounds", ImGuiTableColumnFlags_WidthFixed, 30.0f, 10);    // ID 10

        ImGui::TableHeadersRow();

        // --- INLINE SORT HOOK: This is the standard ImGui pattern ---
        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty) {
                const ImGuiTableColumnSortSpecs* spec = sortSpecs->Specs;

                // 1. Update internal state
                sortColumn_ = spec->ColumnUserID;
                sortAscending_ = (spec->SortDirection == ImGuiSortDirection_Ascending);

                // 2. Perform the sort immediately
                performSort(tables_, sortColumn_, sortAscending_);

                // 3. Reset the flag
                sortSpecs->SpecsDirty = false;
            }
        }

        for (int i = 0; i < (int)tables_.size(); ++i) {
            const auto& t = tables_[i];
            ImGui::TableNextRow();

            // --- Normalized metadata ---
            // Year: prefer vpsYear, then tableYear, then file-derived year.
            std::string displayYear =
                !t.vpsYear.empty() ? t.vpsYear :
                !t.tableYear.empty() ? t.tableYear :
                !t.year.empty() ? t.year : "-";

            // Name: prefer vpsName, then tableName, then filename/title.
            std::string displayName =
                !t.vpsName.empty() ? t.vpsName :
                !t.tableName.empty() ? t.tableName :
                !t.title.empty() ? t.title : "-";

            // Author: prefer vpsAuthors, then tableAuthor.
            std::string displayAuthor =
                !t.vpsAuthors.empty() ? t.vpsAuthors :
                !t.tableAuthor.empty() ? t.tableAuthor : "-";

            // Manufacturer: prefer vpsManufacturer, then tableManufacturer, then manufacturer.
            std::string displayManufacturer =
                !t.vpsManufacturer.empty() ? t.vpsManufacturer :
                !t.tableManufacturer.empty() ? t.tableManufacturer :
                !t.manufacturer.empty() ? t.manufacturer : "-";

            // Column 0: Year
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(displayYear.c_str());

            // Column 1: Name (main selectable)
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            bool isSelected = (selectedIndex_ == i);
            if (ImGui::Selectable(displayName.c_str(), isSelected,
                                  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex_ = i;
                // remove scroll-to-center behavior
                scrollToSelected_ = false;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    LOG_DEBUG(std::string("Row double-click (placeholder) for: ") + displayName + " -> " + t.vpxFile);
                }
            }
            ImGui::PopID();

            // Column 2: Version
            ImGui::TableSetColumnIndex(2);
            if (!t.tableVersion.empty())
                ImGui::TextUnformatted(t.tableVersion.c_str());
            else
                ImGui::TextUnformatted("");

            // Column 2: Author
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(displayAuthor.c_str());

            // Column 3: Manufacturer
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(displayManufacturer.c_str());

            // Column 4: INI + VBS + B2S existence (TODO: collect these vars (only added in table_data.h))
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s%s%s",
                        t.hasINI ? "I " : "- ",  // todo: add check for ini diff and recolor
                        t.hasVBS ? "V " : "- ",  // todo: color if t.hasDiffVbs
                        t.hasB2S ? "B " : "- ");

            // Column 5: ROM name (shows name or empty)
            ImGui::TableSetColumnIndex(6);
            if (!t.romName.empty())
                ImGui::TextUnformatted(t.romName.c_str());
            else
                ImGui::TextUnformatted("");

            // Column 6: Alt/Color/PUP/UDMD/Music
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%s%s%s%s%s",
                        t.hasAltSound ? "S " : "- ",
                        t.hasAltColor ? "C " : "- ",
                        t.hasPup ? "P " : "- ",
                        t.hasUltraDMD ? "U " : "- ",
                        t.hasAltMusic ? "M " : "- "
                    );

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

    ImGui::Separator();

    // --- Last scan info (above buttons) ---
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
        const auto& t = tables_[selectedIndex_];
        if (!t.jsonOwner.empty()) {
            ImGui::TextDisabled("Last table scanner: %s", t.jsonOwner.c_str());
        }
    }
    // --------- Footer buttons, inline at bottom ---------
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() * 2.0f);

    ImGui::BeginGroup();

    if (ImGui::Button("Rescan Tables")) {
        rescanAsync();
    }
    ImGui::SameLine();

    if (ImGui::Button("Open Folder")) {
        std::string path;
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size())
            path = tables_[selectedIndex_].vpxFile;
        actions_.openFolder(path);
    }

    ImGui::SameLine();

    if (ImGui::Button("Extract VBS")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
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
    ImGui::SameLine();

    if (ImGui::Button("View Metadata")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            // This will now be handled by the main editor loop
            showMetadataEditor_ = true; // <-- TOGGLE STATE
            LOG_DEBUG("Toggling metadata editor ON");
        } else {
            LOG_DEBUG("View Metadata pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("INI Editor")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            // This will now be handled by the main editor loop
            showIniEditor_ = true; // <-- TOGGLE STATE
            LOG_DEBUG("Toggling INI editor ON");
        } else {
            LOG_DEBUG("INI Editor pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Play Selected")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
            // --- USE THE LAUNCHER ---
            tableLauncher_->launchTable(t);
        } else {
            LOG_DEBUG("Play pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Exit Editor")) {
        exitRequested_ = true;
    }

    ImGui::EndGroup();

    // --- Footer info ---
    std::ostringstream ss;
    ss << tables_.size() << " tables found";
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
        const auto& t = tables_[selectedIndex_];
        fs::path p(t.vpxFile);
        ss << "  |  Selected: /" << p.parent_path().filename().string()
        << "/" << p.filename().string();
    }
    ImGui::TextDisabled("%s", ss.str().c_str());

    ImGui::End();
}

// Asynchronous rescan helper. Calls the existing tableLoader with same Settings logic.
void EditorUI::rescanAsync() {
    if (loading_) return;
    loading_ = true;

    std::thread([this]() {
        Settings settings = config_->getSettings();
        settings.ignoreScanners = true;
        auto newTables = tableLoader_->loadTableList(settings, nullptr);
        {
            std::lock_guard<std::mutex> lock(tableMutex_);
            tables_ = std::move(newTables);
            if (selectedIndex_ >= (int)tables_.size()) selectedIndex_ = -1;
        }
        loading_ = false;
    }).detach();
}
