#include "editor/ui/editor_header.h"
#include "editor/header_actions.h"
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

                    // 1. Add Modal Opening and External App Flag
                    fs::path p(t_filtered.vpxFile);
                    ui.inExternalAppMode_ = true;
                    ui.modal().openProgress("Launching Game", "Starting " + p.filename().string() + "...");

                    // Use the shared, centralized launch logic from ButtonActions
                    ui.actions().launchTableWithStats(
                        t_filtered,
                        ui.tables(), // Mutable master list
                        ui.tableLauncher(),
                        [&ui]() {
                            ui.requestPostLaunchCleanup();
                        }
                    );
                } else {
                    LOG_DEBUG("'Play' pressed but no table selected");
                    ui.modal().openInfo(
                        "No Table Selected",
                        "You pressed 'Play' but no table was selected."
                        "Please select a table first and try again."
                    );
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

            // --- Info submenu ---
            if (ImGui::BeginMenu("Table Info")) {
                if (ImGui::MenuItem("Show Info")) header_actions::vpxtoolRun(ui, "info show");
                if (ImGui::MenuItem("Extract Info")) header_actions::vpxtoolRun(ui, "info extract");
                // if (ImGui::MenuItem("Import Info")) header_actions::vpxtoolRun(ui, "info import");
                // if (ImGui::MenuItem("Edit Info")) header_actions::vpxtoolRun(ui, "info edit");
                if (ImGui::MenuItem("Diff Info")) header_actions::vpxtoolRun(ui, "info diff");
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Diff Script vs VBS")) header_actions::vpxtoolRun(ui, "diff");
            if (ImGui::MenuItem("Extract Script (VBS)")) header_actions::vpxtoolRun(ui, "extractvbs");
            if (ImGui::MenuItem("Import Script (VBS)")) header_actions::vpxtoolRun(ui, "importvbs");
            if (ImGui::MenuItem("Verify Structure")) header_actions::vpxtoolRun(ui, "verify");
            if (ImGui::MenuItem("Show Gamedata")) header_actions::vpxtoolRun(ui, "gamedata show");
            if (ImGui::MenuItem("Convert Lossless")) header_actions::vpxtoolRun(ui, "images webp");
            // if (ImGui::MenuItem("Apply Patch")) header_actions::vpxtoolRun(ui, "patch");
            if (ImGui::MenuItem("Show Rom Name")) header_actions::vpxtoolRun(ui, "romname");
            if (ImGui::MenuItem("List Contents")) header_actions::vpxtoolRun(ui, "ls");

            ImGui::EndMenu();
        }

        // --- Compress submenu
        if (ImGui::Selectable("Backup/Archive")) {
            ui.deferredModal_ = [&ui]() {
                header_actions::requestCompressTableFolder(ui);
            };
        }

        // --- Edit Metadata (TableOverride Editor)
        if (ImGui::Selectable("Edit Metadata", false)) {
            int idx = ui.selectedIndex();
            auto& filtered = ui.filteredTables();

            if (idx >= 0 && idx < static_cast<int>(filtered.size())) {
                ui.setShowMetadataEditor(true);
                LOG_INFO("Edit Table Metadata requested");
            } else {
                LOG_INFO("Edit Metadata pressed but no table selected");
                ui.modal().openInfo(
                    "No Table Selected",
                    "Please select a table first and try again."
                );
            }
        }

        // --- Delete submenu
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.24f, 0.24f, 1.0f));
        if (ImGui::BeginMenu("Delete")) {
            if (ImGui::MenuItem("Table Folder")) header_actions::requestDeleteTableFolder(ui);
            if (ImGui::MenuItem("Table INI")) header_actions::requestDeleteTableFile(ui, "ini");
            if (ImGui::MenuItem("Table VBS")) header_actions::requestDeleteTableFile(ui, "vbs");
            // if (ImGui::MenuItem("Table Metadata")) header_actions::requestDeleteTableFile(ui, "metadata");
            if (ImGui::MenuItem("Table Overrides")) header_actions::requestDeleteTableFile(ui, "json");
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
                header_actions::clearAllCaches(ui);
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

    header_actions::drawModals(ui);
}
}
