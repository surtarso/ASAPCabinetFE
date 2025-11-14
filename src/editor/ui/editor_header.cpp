#include "editor/ui/editor_header.h"
#include "editor/menu_actions.h"
#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

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

        ImGui::TextDisabled("Selected Table Actions");
        if (ImGui::BeginMenu("VPXTool")) {
            if (ImGui::MenuItem("Table Info"))              menu_actions::vpxtoolRun(ui, "info");
            if (ImGui::MenuItem("Diff Script vs VBS"))      menu_actions::vpxtoolRun(ui, "diff");
            if (ImGui::MenuItem("Extract Script (VBS)"))    menu_actions::vpxtoolRun(ui, "extractvbs");
            if (ImGui::MenuItem("Import Script (VBS)"))     menu_actions::vpxtoolRun(ui, "importvbs");
            if (ImGui::MenuItem("Verify Structure"))        menu_actions::vpxtoolRun(ui, "verify");
            if (ImGui::MenuItem("Apply Patch"))             menu_actions::vpxtoolRun(ui, "patch");
            if (ImGui::MenuItem("List Contents"))           menu_actions::vpxtoolRun(ui, "ls");
            // if (ImGui::MenuItem("Reindex Directory"))       menu_actions::vpxtoolRun(ui, "index");
            ImGui::EndMenu();
        }

        // --- Compress submenu
        if (ImGui::Selectable("Backup/Archive")) {
                menu_actions::requestCompressTableFolder(ui);
        }

        if (ImGui::Selectable("Edit Metadata", false)) {
            ui.setShowMetadataEditor(true);
            LOG_INFO("Edit Table Metadata requested");
        }

        // --- Delete submenu
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.24f, 0.24f, 1.0f));
        if (ImGui::BeginMenu("Delete")) {
            if (ImGui::MenuItem("Table Folder")) menu_actions::requestDeleteTableFolder(ui);
            if (ImGui::MenuItem("Table INI")) menu_actions::requestDeleteTableFile(ui, "ini");
            if (ImGui::MenuItem("Table VBS")) menu_actions::requestDeleteTableFile(ui, "vbs");
            // if (ImGui::MenuItem("Table Metadata")) menu_actions::requestDeleteTableFile(ui, "metadata");
            if (ImGui::MenuItem("Table Overrides")) menu_actions::requestDeleteTableFile(ui, "json");
            ImGui::EndMenu();
        }
        ImGui::PopStyleColor();

        ImGui::Separator();

        // ------------------------------ TODO: MEDIA PANEL
        // ImGui::TextDisabled("Media Tools");
        // if (ImGui::Selectable("Resize Media...", false)) {
        //     LOG_WARN("Resize Media requested [Placeholder]");
        //     // TODO: open modal for resize presets
        // }
        // if (ImGui::Selectable("Compress Media...", false)) {
        //     LOG_WARN("Compress Media requested [Placeholder]");
        //     // TODO: open modal or run ffmpeg compression task
        // }
        // ImGui::Separator();

        // ------------------------------
        ImGui::TextDisabled("System");
        Settings& settings = ui.configService()->getMutableSettings();

        // toggle table tooltips
        bool showTooltips = settings.showTableTooltips;
        if (ImGui::Checkbox("Show Table Tooltips", &showTooltips)) {
            LOG_INFO(std::string("Show Table Tooltips toggled: ") + (showTooltips ? "ON" : "OFF"));
            settings.showTableTooltips = showTooltips;
            ui.configService()->saveConfig();
        }

        // Fast Startup (Skip Scanning)
        bool autoRefresh = settings.ignoreScanners;
        if (ImGui::Checkbox("Fast Startup (Skip Scanners!)", &autoRefresh)) {
            LOG_INFO(std::string("Fast Startup (Skip Scanning) toggled: ") + (autoRefresh ? "ON" : "OFF"));
            settings.ignoreScanners = autoRefresh;
            if (settings.ignoreScanners){
                settings.forceRebuildMetadata = false;
            }
            ui.configService()->saveConfig();
        }

        ImGui::Separator();

        // --- Maintenance submenu
        if (ImGui::BeginMenu("Maintenance")) {
            if (ImGui::MenuItem("Clear All Caches")) {
                menu_actions::clearAllCaches(ui);
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

    menu_actions::drawModals(ui);
}
}
