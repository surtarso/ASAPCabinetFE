#include "editor/ui/editor_header.h"
#include <imgui.h>

namespace editor_header {

void drawHeader(EditorUI& ui) {
    // ------------------------------ FUZZY SEARCH BAR ------------------------------
    if (!ImGui::IsItemActive()) {
        // Detect typing and auto-focus search field
        ui.actions().handleKeyboardSearchFocus(
            ui.searchBuffer(),
            ui.searchQuery(),
            [&ui]() { ui.filterAndSortTablesPublic(); },
            [&ui]() {
                if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
                    const auto& t_filtered = ui.filteredTables()[ui.selectedIndex()];

                    // Use the shared, centralized launch logic from ButtonActions
                    ui.actions().launchTableWithStats(
                        t_filtered,
                        ui.tables(), // Mutable master list
                        ui.tableLauncher(),
                        [&ui](){ ui.filterAndSortTablesPublic(); } // Refresh UI callback
                    );
                } else {
                    LOG_DEBUG("'Play' pressed but no table selected");
                }
            }
        );
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() * 1.4f);
    if (ImGui::InputTextWithHint("##SearchInputTop", "Search by Year, Name, Author, Manufacturer, File, or ROM",
                                 ui.searchBuffer(), ui.searchBufferSize())) {
        ui.setSearchQuery(ui.searchBuffer());
        ui.filterAndSortTablesPublic();
    }

    // -------------- DEBUG: show raw buffer length and bytes ----------------
    // ImGui::Text("Buffer len: %zu  Query len: %zu", strlen(ui.searchBuffer()), ui.searchQuery().size());
    // ImGui::Text("strlen: %zu  raw bytes:", strlen(ui.searchBuffer()));
    // for (int i = 0; i < 16; i++) {
    //     ImGui::SameLine();
    //     ImGui::Text("%02X", static_cast<unsigned char>(ui.searchBuffer()[i]));
    // }

    ImGui::PopItemWidth();
    ImGui::SameLine();

    // ------------------------------ ADVANCED DROPMENU ------------------------------
    if (ImGui::BeginCombo("##advanced_combo", "Advanced", ImGuiComboFlags_NoPreview | ImGuiComboFlags_HeightLargest)) {

        ImGui::TextDisabled("Table Actions");
        if (ImGui::Selectable("Repair Table (via VPXTool)", false)) {
            LOG_INFO("Repair Table requested via VPXTool");
            // TODO: run VPXTool repair subprocess, e.g. system("vpxtool repair <table>");
        }
        if (ImGui::Selectable("Rebuild Table Index", false)) {
            LOG_INFO("Rebuild Table Index requested");
            // TODO: trigger table index regeneration (reload json)
        }
        if (ImGui::Selectable("Launch in VPinballX", false)) {
            LOG_INFO("Launch current table in VPinballX requested");
            // TODO: launch external VPX executable using current table path
        }
        if (ImGui::Selectable("Export Metadata Override", false)) {
            LOG_INFO("Export Metadata Override requested");
            // TODO: write JSON override file to /metadata/overrides/<table>.json
        }

        // --- Delete submenu
        if (ImGui::BeginMenu("Delete")) {
            if (ImGui::MenuItem("Table Folder")) {
                LOG_WARN("Delete Table Folder requested");
                // TODO: std::filesystem::remove_all(table.folder)
            }
            if (ImGui::MenuItem("Table INI")) {
                LOG_WARN("Delete Table INI requested");
                // TODO: std::filesystem::remove(table.ini)
            }
            if (ImGui::MenuItem("Table VBS")) {
                LOG_WARN("Delete Table VBS requested");
                // TODO: std::filesystem::remove(table.vbs)
            }
            if (ImGui::MenuItem("Table Metadata")) {
                LOG_WARN("Delete Table Metadata requested");
                // TODO: std::filesystem::remove(table.metadata)
            }
            if (ImGui::MenuItem("Table Overrides")) {
                LOG_WARN("Delete Table Overrides requested");
                // TODO: std::filesystem::remove(table.override)
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete All Related Files")) {
                LOG_WARN("Delete All Related Files requested");
                // TODO: batch remove all files above
            }
            ImGui::EndMenu();
        }

        // --- Compress submenu
        if (ImGui::BeginMenu("Compress")) {
            if (ImGui::MenuItem("Compress Table Folder (.zip)")) {
                LOG_INFO("Compress Table Folder requested");
                // TODO: use zip library or system("zip -r table.zip tablefolder")
            }
            if (ImGui::MenuItem("Compress Individual Components")) {
                LOG_INFO("Compress Individual Components requested");
                // TODO: compress each component individually
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // ------------------------------
        ImGui::TextDisabled("Media Tools");
        if (ImGui::Selectable("Resize Media...", false)) {
            LOG_INFO("Resize Media requested");
            // TODO: open modal for resize presets
        }
        if (ImGui::Selectable("Compress Media...", false)) {
            LOG_INFO("Compress Media requested");
            // TODO: open modal or run ffmpeg compression task
        }
        if (ImGui::Selectable("Clear Media Cache", false)) {
            LOG_INFO("Clear Media Cache requested");
            // TODO: clear cache folder (e.g., std::filesystem::remove_all(mediaCache))
        }
        if (ImGui::Selectable("Rebuild Media Cache", false)) {
            LOG_INFO("Rebuild Media Cache requested");
            // TODO: rebuild all cached media thumbnails/videos
        }

        ImGui::Separator();

        // ------------------------------
        ImGui::TextDisabled("System");

        Settings& settings = ui.configService()->getMutableSettings();
        bool showTooltips = settings.showTableTooltips;

        if (ImGui::Checkbox("Show Table Tooltips", &showTooltips)) {
            LOG_INFO(std::string("Show Table Tooltips toggled: ") + (showTooltips ? "ON" : "OFF"));
            settings.showTableTooltips = showTooltips;
            ui.configService()->saveConfig();
        }

        static bool autoRefresh = false;  // TODO: load from settings.Editor.autoRefresh
        if (ImGui::Checkbox("Auto-Refresh on Startup", &autoRefresh)) {
            LOG_INFO(std::string("Auto-Refresh on Startup toggled: ") + (autoRefresh ? "ON" : "OFF"));
            // TODO: persist in settings.Editor.autoRefresh
        }

        // --- Maintenance submenu
        if (ImGui::BeginMenu("Maintenance")) {
            if (ImGui::MenuItem("Clear All Caches")) {
                LOG_INFO("Clear All Caches requested");
                // TODO: clear texture, video, metadata, and temp caches
            }
            if (ImGui::MenuItem("Reload Configuration")) {
                LOG_INFO("Reload Configuration requested");
                // TODO: reload settings.json and refresh UI
            }
            if (ImGui::MenuItem("Reinitialize UI")) {
                LOG_INFO("Reinitialize UI requested");
                // TODO: trigger full ImGui/UI rebuild
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.24f, 0.24f, 1.0f));
        if (ImGui::Selectable("Exit Editor", false)) {
            LOG_INFO("Exit Editor requested from Advanced Menu");
            ui.requestExit();
        }
        ImGui::PopStyleColor();

        ImGui::EndCombo();
    }
}
}

// ---------- possible actions:
// vpxtool actions [submenu?] (repair, rebuild, etc)
// vpinballx actions [submenu?] (launch, etc)
// [delete submenu?]
// - delete table folder
// - delete table ini
// - delete table vbs
// - delete table metadata
// - delete overrides (in metadata, the files that override table defaults)
// - compress? (create an archive w/ all table files)
// [media submenu?]
// - compress media (quality settings?)
// - resize media (dimensions?)
// - clear cache(s)
// - export metadata (create 'an override' file with all current metadata for the table)


// ----------- possible options:
// - show/hide tooltips
// - auto-refresh on startup (fast path)
