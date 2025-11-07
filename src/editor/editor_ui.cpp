#include "editor/editor_ui.h"
#include "log/logging.h"
#include "config/settings.h"
#include <thread>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// Constructor already used the loader to fill tables_ originally.
// Keep same behavior: read index once and store.
EditorUI::EditorUI(IConfigService* config, ITableLoader* tableLoader)
    : config_(config), tableLoader_(tableLoader) {
    Settings settings = config_->getSettings();
    settings.ignoreScanners = true; // ignore scanners on start (not persisted)
    tables_ = tableLoader_->loadTableList(settings, nullptr);
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
        ImGui::TableSetupColumn("Year", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Manufacturer", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthFixed, 45.0f);
        ImGui::TableSetupColumn("ROM", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Extras", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Images", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Videos", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Sounds", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableHeadersRow();

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
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
            LOG_DEBUG(std::string("Open Folder pressed (placeholder) for: ") + t.title + " -> " + t.vpxFile);
        } else {
            LOG_DEBUG("Open Folder pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Extract VBS")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
            LOG_DEBUG(std::string("Extract VBS pressed (placeholder) for: ") + t.title + " -> " + t.vpxFile);
        } else {
            LOG_DEBUG("Extract VBS pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("View Metadata")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
            LOG_DEBUG(std::string("View Metadata pressed (placeholder) for: ") + t.title + " -> " + t.vpxFile);
        } else {
            LOG_DEBUG("View Metadata pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("INI Editor")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
            LOG_DEBUG(std::string("INI Editor pressed (placeholder) for: ") + t.title + " -> " + t.vpxFile);
        } else {
            LOG_DEBUG("INI Editor pressed but no table selected");
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Play Selected")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)tables_.size()) {
            const auto& t = tables_[selectedIndex_];
            LOG_DEBUG(std::string("Play pressed (placeholder) for: ") + t.title + " -> " + t.vpxFile);
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
