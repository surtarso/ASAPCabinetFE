#include "editor/editor_ui.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace
{
    // Helper to compare two values with direction and a unique tie-breaker
    template <typename T>
    bool compareWithTieBreaker(const T &val_a, const T &val_b, bool ascending, const TableData &a, const TableData &b)
    {
        if (val_a == val_b)
        {
            // CRITICAL: Stable sort tie-breaker on unique file path
            return a.vpxFile < b.vpxFile;
        }
        return ascending ? (val_a < val_b) : (val_a > val_b);
    }

    // The actual sort logic wrapped in a function for clarity
    void performSort(std::vector<TableData> &tables, int sortColumn, bool sortAscending)
    {
        std::sort(tables.begin(), tables.end(), [sortColumn, sortAscending](const TableData &a, const TableData &b)
                  {

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
            } });
    }
} // namespace

void EditorUI::filterAndSortTables()
{
    // std::lock_guard<std::mutex> lock(tableMutex_);
    filteredTables_.clear();

    std::string lowerQuery;
    if (!searchQuery_.empty())
    {
        lowerQuery = searchQuery_;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    }

    for (const auto &table : tables_)
    {
        // --- FUZZY SEARCH LOGIC (from vpxguitools pattern) ---
        if (searchQuery_.empty())
        {
            filteredTables_.push_back(table);
            continue;
        }

        // Prepare fields for case-insensitive search
        std::string lowerTitle = table.title;
        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);

        std::string lowerTableName = table.tableName;
        std::transform(lowerTableName.begin(), lowerTableName.end(), lowerTableName.begin(), ::tolower);

        std::string lowerVpsName = table.vpsName;
        std::transform(lowerVpsName.begin(), lowerVpsName.end(), lowerVpsName.begin(), ::tolower);


        fs::path vpxPath(table.vpxFile);
        std::string lowerFilename = vpxPath.stem().string();
        std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

        std::string lowerRom = table.romName;
        std::transform(lowerRom.begin(), lowerRom.end(), lowerRom.begin(), ::tolower);

        // Check if query is found in Name (title), Filename (vpxFile stem), or ROM name
        if (lowerTitle.find(lowerQuery) != std::string::npos ||
            lowerTableName.find(lowerQuery) != std::string::npos ||
            lowerVpsName.find(lowerQuery) != std::string::npos ||
            lowerFilename.find(lowerQuery) != std::string::npos ||
            lowerRom.find(lowerQuery) != std::string::npos)
        {
            filteredTables_.push_back(table);
        }
    }

    // 1. Apply the existing sorting logic to the filtered list
    performSort(filteredTables_, sortColumn_, sortAscending_);

    // 2. Implement the selection rule:
    if (filteredTables_.empty())
    {
        // If there are no tables (filtered list is empty), clear selection.
        selectedIndex_ = -1;
    }
    else if (searchQuery_.empty())
    {
        // If the search is CLEARED (full list is shown), explicitly unselect.
        selectedIndex_ = -1;
    }
    else
    {
        // If a search is active AND results were found, pre-select the first item.
        selectedIndex_ = 0;
    }
}

// Constructor already used the loader to fill tables_ originally.
// Keep same behavior: read index once and store.
EditorUI::EditorUI(IConfigService *config, ITableLoader *tableLoader, ITableLauncher *launcher,
                   bool &showMeta, bool &showIni)
    : config_(config),
      tableLoader_(tableLoader),
      tableLauncher_(launcher),
      actions_(config),
      selectedIndex_(-1),
      showMetadataEditor_(showMeta),
      showIniEditor_(showIni)
{

    Settings settings = config_->getSettings();
    settings.ignoreScanners = true; // ignore scanners on start (not persisted)
    tables_ = tableLoader_->loadTableList(settings, nullptr);
    // Apply default sort on load
    filterAndSortTables();
}

// Draw editor UI embedded in the main window
void EditorUI::draw()
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("ASAPCabinetFE Editor", nullptr, windowFlags);

    std::lock_guard<std::mutex> lock(tableMutex_);

    if (loading_)
    {
        ImGui::Text("Scanning tables... (please wait)");
        ImGui::End();
        return;
    }

    if (tables_.empty())
    {
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

    ImGui::BeginChild("TableContainer", tableSize, false, ImGuiWindowFlags_NoScrollbar);

    // Check for no results: display message instead of table, but DON'T return.
    if (filteredTables_.empty() && !tables_.empty())
    {
        ImGui::TextDisabled("No tables match the current filter: '%s'", searchQuery_.c_str());
    }
    else if (!filteredTables_.empty() && ImGui::BeginTable("table_list", 11, flags, tableSize))
    {

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
        if (ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs())
        {
            if (sortSpecs->SpecsDirty)
            {
                const ImGuiTableColumnSortSpecs *spec = sortSpecs->Specs;

                // 1. Update internal state
                sortColumn_ = spec->ColumnUserID;
                sortAscending_ = (spec->SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;

                // 2. Perform the sort immediately on the FILTERED list
                performSort(filteredTables_, sortColumn_, sortAscending_);

                // 3. Update selection (first item is selected after sort)
                if (filteredTables_.empty())
                {
                    selectedIndex_ = -1;
                }
            }
        }

        for (int i = 0; i < (int)filteredTables_.size(); ++i)
        {
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
                                  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
            {
                selectedIndex_ = isSelected ? -1 : i;
                // remove scroll-to-center behavior
                scrollToSelected_ = false;
                if (ImGui::IsMouseDoubleClicked(0))
                {
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
                        t.hasINI ? "I " : "- ", // todo: add check for ini diff and recolor
                        t.hasVBS ? "V " : "- ", // todo: color if t.hasDiffVbs
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
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
    {
        const auto &t = filteredTables_[selectedIndex_];
        if (!t.jsonOwner.empty())
        {
            ImGui::TextDisabled("Last table scanner: %s", t.jsonOwner.c_str());
        }
    }

    // --------- Footer buttons and Search Bar (inline at bottom) ---------
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() * 2.0f);

    ImGui::BeginGroup(); // Start Button Group

    if (ImGui::Button("Rescan Tables"))
    {
        rescanAsync();
    }
    ImGui::SameLine();

    if (ImGui::Button("Open Folder"))
    {
        std::string path;
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
            path = filteredTables_[selectedIndex_].vpxFile;
        actions_.openFolder(path);
    }

    ImGui::SameLine();

    if (ImGui::Button("Extract VBS"))
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
        {
            const auto &t = filteredTables_[selectedIndex_];
            // 1. Extract the VBS
            actions_.extractVBS(t.vpxFile); // <-- USE ACTION

            // 2. Build the path to the new .vbs file
            fs::path vpxPath(t.vpxFile);
            std::string vbsPath = (vpxPath.parent_path() / (vpxPath.stem().string() + ".vbs")).string();

            // 3. Open it
            if (fs::exists(vbsPath))
            {
                actions_.openInExternalEditor(vbsPath); // <-- USE ACTION
            }
            else
            {
                LOG_ERROR("Tried to open VBS, but extraction failed or file not found at: " + vbsPath);
            }
        }
        else
        {
            LOG_DEBUG("Extract VBS pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("View Metadata"))
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
        {
            // This will now be handled by the main editor loop
            showMetadataEditor_ = true; // <-- TOGGLE STATE
            LOG_DEBUG("Toggling metadata editor ON");
        }
        else
        {
            LOG_DEBUG("View Metadata pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("INI Editor"))
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
        {
            // This will now be handled by the main editor loop
            showIniEditor_ = true; // <-- TOGGLE STATE
            LOG_DEBUG("Toggling INI editor ON");
        }
        else
        {
            LOG_DEBUG("INI Editor pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Play Selected"))
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
        {
            const auto &t = filteredTables_[selectedIndex_];
            // --- USE THE LAUNCHER ---
            tableLauncher_->launchTable(t);
        }
        else
        {
            LOG_DEBUG("Play pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Exit Editor"))
    {
        exitRequested_ = true;
    }

    // --- SEARCH BAR WIDGET: PLACED HERE (Right side of buttons) ---
    ImGui::SameLine(0.0f, 15.0f); // Add a small separator space after the last button

    // Calculate the width for the search bar, ensuring it fits the remaining space
    float searchWidth = ImGui::GetContentRegionMax().x - ImGui::GetCursorPosX() - (ImGui::GetFrameHeight() * 1.5f) - ImGui::GetStyle().ItemSpacing.x * 2;
    if (searchWidth < 100.0f)
        searchWidth = 100.0f;

    ImGui::PushItemWidth(searchWidth);
    if (ImGui::InputTextWithHint("##SearchInput", "Search by Name, File, or ROM",
                                 searchBuffer_, sizeof(searchBuffer_)))
    {
        searchQuery_ = searchBuffer_;
        filterAndSortTables(); // Live filter on change (safe inside draw()'s lock)
    }
    ImGui::PopItemWidth();

    // Draw Clear Button
    ImGui::SameLine();
    if (ImGui::Button("X"))
    {
        searchBuffer_[0] = '\0'; // Clear buffer immediately
        searchQuery_ = "";
        filterAndSortTables(); // Clear filter and re-populate (safe inside draw()'s lock)
    }
    // --- END SEARCH BAR WIDGET ---

    ImGui::EndGroup(); // End Button Group

    // --- Footer info ---
    std::ostringstream ss;
    ss << filteredTables_.size() << " tables shown"; // Changed from tables_.size() to filteredTables_.size()
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)filteredTables_.size())
    {
        const auto &t = filteredTables_[selectedIndex_];
        fs::path p(t.vpxFile);
        ss << "    |    Selected: /" << p.parent_path().filename().string()
           << "/" << p.filename().string();
    }
    ImGui::TextDisabled("%s", ss.str().c_str());

    ImGui::End();
}

// Asynchronous rescan helper. Calls the existing tableLoader with same Settings logic.
void EditorUI::rescanAsync()
{
    if (loading_)
        return;
    loading_ = true;

    std::thread([this]()
                {
        Settings settings = config_->getSettings();
        settings.ignoreScanners = true;
        auto newTables = tableLoader_->loadTableList(settings, nullptr);
        {
            std::lock_guard<std::mutex> lock(tableMutex_);
            tables_ = std::move(newTables);
            filterAndSortTables();
            // if (selectedIndex_ >= (int)tables_.size()) selectedIndex_ = -1;
        }
        loading_ = false; })
        .detach();
}
