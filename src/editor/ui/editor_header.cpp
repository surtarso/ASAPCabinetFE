#include "editor/ui/editor_header.h"
#include "editor/header_actions.h"
#include "data/lbdb/lbdb_builder.h"
#include "data/asapcab/asapcab_database_manager.h"
#include <thread>
#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace editor_header {

void drawHeader(EditorUI& ui) {

    // ---------------------------------------------
    // Arrow-key navigation for rows (FULLY SEPARATE)
    // ---------------------------------------------
    ui.actions().handleRowNavigation(
        ui.selectedIndex(),
        static_cast<int>(ui.filteredTables().size())
    );

    // ------------------------------ FUZZY SEARCH BAR ------------------------------
    if (!ImGui::IsItemActive()) {
        ui.actions().handleKeyboardSearchFocus(
            ui.searchBuffer(),
            ui.searchQuery(),
            [&ui]() { ui.filterAndSortTablesPublic(); },
            [&ui]() {
                if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
                    const auto& t_filtered = ui.filteredTables()[ui.selectedIndex()];

                    fs::path p(t_filtered.vpxFile);
                    ui.inExternalAppMode_ = true;
                    ui.modal().openProgress(
                        "Launching Game",
                        "Starting " + p.filename().string() + "..."
                    );

                    ui.actions().launchTableWithStats(
                        t_filtered,
                        ui.tables(),
                        ui.tableLauncher(),
                        [&ui]() { ui.requestPostLaunchCleanup(); }
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
    if (ImGui::InputTextWithHint("##SearchInputTop",
                                 "Search by Year, Name, Author, Manufacturer, File, or ROM",
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
        Settings& settings = ui.configService()->getMutableSettings();

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

        ImGui::Separator();

        // --- Delete submenu
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.24f, 0.24f, 1.0f));
        if (ImGui::BeginMenu("Delete")) {
            if (ImGui::MenuItem("Table Folder")) header_actions::requestDeleteTableFolder(ui);
            if (ImGui::MenuItem("Table INI")) header_actions::requestDeleteTableFile(ui, "ini");
            if (ImGui::MenuItem("Table VBS")) header_actions::requestDeleteTableFile(ui, "vbs");
            if (ImGui::MenuItem("Table Overrides")) header_actions::requestDeleteTableFile(ui, "json");
            ImGui::EndMenu();
        }
        ImGui::PopStyleColor();

        // --- Maintenance submenu (colored title)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.7f, 0.3f, 0.85f));
        if (ImGui::BeginMenu("Maintenance")) {

            // ----------------------------
            // CACHE SUBMENU
            // ----------------------------
            if (ImGui::BeginMenu("Cache")) {

                // CLEAR ALL CACHES
                if (ImGui::MenuItem("Clear All Caches")) {
                    header_actions::clearAllCaches(ui);
                }
                // CLEAR PREVIEW CACHE
                if (ImGui::MenuItem("Clear Metadata Preview Cache")) {
                    header_actions::clearPreviewCache(ui);
                }
                // CLEAR VPSDB CACHE
                if (ImGui::MenuItem("Clear VPSDB Image Cache")) {
                    header_actions::clearVpsDbImageCache(ui);
                }

                ImGui::EndMenu(); // Cache
            }

            // ----------------------------
            // DATABASE SUBMENU
            // ----------------------------
            if (ImGui::BeginMenu("Database")) {

                // REBUILD MAIN DATABASE
                if (ImGui::MenuItem("Rebuild AsapCab's Main Database")) {
                    // Immediately shows modal so the user knows it's working
                    ui.modal().openProgress(
                        "Building AsapCab's DB",
                        "Working...\nThis may take several minutes."
                    );
                    // Background thread to avoid blocking ImGui
                    std::thread([settings, &ui] {

                        // Create manager instance
                        data::asapcabdb::AsapCabDatabaseManager dbManager(settings);

                        bool success = dbManager.ensureAvailable();
                        // Close modal when finished
                        ui.modal().finishProgress("AsapCab's Database is now available!"); // TODO: not showing up...

                        if (!success) {
                            LOG_ERROR("AsapCab's DB rebuild failed");
                        } else {
                            LOG_INFO("AsapCab's DB rebuild complete");
                        }
                    }).detach();
                }

                // REBUILD LAUNCHBOX DB
                if (ImGui::MenuItem("Rebuild Launchbox DB")) {
                    // Immediately shows modal so the user knows it's working
                    ui.modal().openProgress(
                        "Building LaunchBox DB",
                        "Working...\nThis may take a few minutes."
                    );
                    // Background thread to avoid blocking ImGui
                    std::thread([settings, &ui] {

                        bool success = launchbox::build_pinball_database(
                            settings,
                            nullptr // no progress callback
                        );
                        // Close modal when finished
                        ui.modal().finishProgress("Launchbox Database is now available!");

                        if (!success) {
                            LOG_ERROR("LaunchBox DB rebuild failed");
                        } else {
                            LOG_INFO("LaunchBox DB rebuild complete");
                        }
                    }).detach();
                }
                // REBUILD IPDB
                if (ImGui::MenuItem("Update Internet Pinball DB")) {
                    ui.modal().openProgress(
                        "Updating IPDB",
                        "Working...\nDownloading data..."
                    );

                    std::thread([settings, &ui] {
                        // Create updater with progress disabled (modal already handles UI)
                        data::ipdb::IpdbUpdater updater(settings, nullptr);

                        bool success = updater.forceUpdate();
                        ui.modal().finishProgress("IPDB is now updated!");

                        if (!success) LOG_ERROR("IPDB update failed");
                        else          LOG_INFO("IPDB update complete");
                    }).detach();
                }
                // REBUILD VPSDB
                if (ImGui::MenuItem("Update Virtual Pinball Spreadsheet DB")) {

                    ui.modal().openProgress(
                        "Updating VPS Database",
                        "Working...\nChecking for updates..."
                    );

                    std::thread([settings, &ui] {

                        // Build local paths
                        std::string vpsDbPath      = settings.vpsDbPath;
                        std::string lastUpdatedPath = settings.vpsDbLastUpdated;

                        // Hard assumption: update on user action → "startup" is always valid.
                        // If you prefer another rule, change the string.
                        std::string updateFrequency = "startup";

                        // Instantiate updater
                        VpsDatabaseUpdater updater(vpsDbPath);

                        bool success = updater.fetchIfNeeded(
                            lastUpdatedPath,
                            updateFrequency,
                            nullptr // no LoadingProgress → modal gives user feedback
                        );
                        LOG_INFO("finishProgress called");
                        ui.modal().finishProgress("VPS Database is now updated!");

                        if (!success) LOG_ERROR("VPS Database update failed");
                        else          LOG_INFO("VPS Database update complete");
                    }).detach();
                }
                // REBUILD VPINMDB
                if (ImGui::MenuItem("Update VPin Media DB")) {

                    ui.modal().openProgress(
                        "Updating VPin Media DB",
                        "Working...\nChecking or downloading file..."
                    );

                    std::thread([settings, &ui] {

                        // Create updater with null progress (modal is enough)
                        data::vpinmdb::VpinMdbUpdater updater(settings, nullptr);

                        bool success = updater.ensureAvailable();
                        LOG_INFO("finishProgress called");
                        ui.modal().finishProgress("VPin Media DB is ready!");

                        if (!success) LOG_ERROR("VPin Media DB update failed");
                        else          LOG_INFO("VPin Media DB update complete");
                    }).detach();
                }
                ImGui::EndMenu(); // Database
            }
            ImGui::EndMenu(); // Maintenance
        }
        ImGui::PopStyleColor();
        ImGui::Separator();

        // ------------------------------
        ImGui::TextDisabled("System");

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

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.65f, 0.30f, 1.0f));  //green
        if (ImGui::Selectable("Switch to Frontend")){
            LOG_INFO("Frontend requested from Advanced Menu");
            ui.setHotReloadStatus(true);
        }
        ImGui::PopStyleColor();

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
