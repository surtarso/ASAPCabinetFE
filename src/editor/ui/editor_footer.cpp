#include "editor/ui/editor_footer.h"
#include "utils/editor_tooltips.h"
#include "tables/table_patcher.h"
#include "log/logging.h"
#include <imgui.h>
#include <filesystem>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace editor_footer {

void drawFooter(EditorUI& ui) {
    if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
        // ---------- Footer Selected Upper Info ----------
        const auto& t = ui.filteredTables()[ui.selectedIndex()];
        // --- Last Scanner Owner ---
        if (!t.jsonOwner.empty()) {
            ImGui::TextDisabled("Last table scanner: %s", t.jsonOwner.c_str()); //string
            ImGui::SameLine();
        }

        // --- Patched Status ---
        ImGui::TextDisabled(" | Patched: ");
        ImGui::SameLine();

        if (t.isPatched) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.54f, 0.74f, 0.24f, 1.0f)); //green
            ImGui::TextUnformatted("Yes");
            ImGui::PopStyleColor();
        } else {
            ImGui::TextDisabled("No");
        }
        ImGui::SameLine();

        // --- Broken Status ---
        ImGui::TextDisabled(" | Broken: ");
        ImGui::SameLine();

        if (t.isBroken) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.24f, 0.24f, 1.0f));  //red
            ImGui::TextUnformatted("Yes");
            ImGui::PopStyleColor();
        } else {
            ImGui::TextDisabled("No");
        }
        ImGui::SameLine();

        ImGui::TextDisabled(" | Play Count: %d", t.playCount); //int
        ImGui::SameLine();

        ImGui::TextDisabled(" | Last Play Time: %.2f mins", t.playTimeLast); //float
        ImGui::SameLine();

        ImGui::TextDisabled(" | Total Play Time: %.2f mins", t.playTimeTotal); //float
    }

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing()*2.0f);

    ImGui::BeginGroup();

    {
        const char* modeLabel = (ui.scannerMode() == ScannerMode::File) ? "File" :
                               (ui.scannerMode() == ScannerMode::VPin) ? "VPin" : "VPSDb";
        std::string comboLabel = std::string("Rescan (") + modeLabel + ")";

        ImVec4 customColor  = ImVec4(0.35f,0.20f,0.55f,1.0f);
        ImVec4 hoveredColor = ImVec4(0.45f,0.30f,0.65f,1.0f);
        ImVec4 activeColor  = ImVec4(0.25f,0.15f,0.40f,1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button,        customColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  activeColor);

        // ---------- Rescan Options Combo ----------
        if (ImGui::BeginCombo("##rescan_combo", comboLabel.c_str(), ImGuiComboFlags_NoPreview | ImGuiComboFlags_HeightLargest)) {
            // ImGui::TextDisabled("Scanner Mode");
            // if (ImGui::Selectable("File Scanner", ui.scannerMode() == ScannerMode::File))
            //     ui.setScannerMode(ScannerMode::File);
            // if (ImGui::Selectable("VPin Scanner", ui.scannerMode() == ScannerMode::VPin))
            //     ui.setScannerMode(ScannerMode::VPin);
            // if (ImGui::Selectable("VPSDb Scanner", ui.scannerMode() == ScannerMode::VPSDb))
            //     ui.setScannerMode(ScannerMode::VPSDb);

            // Flag to keep the popup open after clicking a selectable item
            const ImGuiSelectableFlags keepOpenFlags = ImGuiSelectableFlags_DontClosePopups;

            ImGui::TextDisabled("Scanner Mode");

            // --- File Scanner ---
            bool isFileSelected = ui.scannerMode() == ScannerMode::File;
            const char* fileLabel = isFileSelected ? "File Scanner \t\t\t\t\t\t «" : "File Scanner";

            if (ImGui::Selectable(fileLabel, isFileSelected, keepOpenFlags)) {
                ui.setScannerMode(ScannerMode::File);
            }

            // --- VPin Scanner ---
            bool isVPinSelected = ui.scannerMode() == ScannerMode::VPin;
            const char* vpinLabel = isVPinSelected ? "VPin Scanner \t\t\t\t\t\t «" : "VPin Scanner";

            if (ImGui::Selectable(vpinLabel, isVPinSelected, keepOpenFlags)) {
                ui.setScannerMode(ScannerMode::VPin);
            }

            // --- VPSDb Scanner ---
            bool isVPSDbSelected = ui.scannerMode() == ScannerMode::VPSDb;
            const char* vpsDbLabel = isVPSDbSelected ? "VPSDb Scanner \t\t\t\t\t «" : "VPSDb Scanner";

            if (ImGui::Selectable(vpsDbLabel, isVPSDbSelected, keepOpenFlags)) {
                ui.setScannerMode(ScannerMode::VPSDb);
            }

            ImGui::TextDisabled("Options");
            Settings& settings = ui.configService()->getMutableSettings();

            // Use external VPXTOOLS
            bool wantsVpxtool = settings.useVpxtool;
            if (ImGui::Checkbox("Use External VPXTool", &wantsVpxtool)) {
                settings.useVpxtool = wantsVpxtool;
                ui.configService()->saveConfig();
            }

            // Auto patch tables
            bool autoPatch = settings.autoPatchTables;
            if (ImGui::Checkbox("Patch All Tables", &autoPatch)) {
                LOG_INFO(std::string("Auto-Patch tables on Rescan toggled: ") + (autoPatch ? "ON" : "OFF"));
                settings.autoPatchTables = autoPatch;  // invert
                ui.configService()->saveConfig();
            }

            // // Auto media download
            // bool autoMedia = settings.fetchMediaOnline;
            // bool autoLogo = settings.downloadTopperLogoImage;   // Launchbox (for Topper Logos)
            // bool autoFlyer = settings.downloadFlyersImage;       // Launchbox Flyers (front/back)
            // bool autoPlayf = settings.downloadPlayfieldImage;    // Vpin Playfield
            // bool autoBackg = settings.downloadBackglassImage;    // Vpin Backglass
            // bool autoDmd = settings.downloadDmdImage;          // Vpin DMD
            // bool autoWheel = settings.downloadWheelImage;        // VPin Wheel
            // if (ImGui::Checkbox("Download All Media", &autoMedia)) {
            //     LOG_INFO(std::string("Download All Media on Rescan toggled: ") + (autoMedia ? "ON" : "OFF"));
            //     settings.fetchMediaOnline = autoMedia;  // invert
            //     settings.downloadTopperLogoImage = autoLogo;   // Launchbox (for Topper Logos)
            //     settings.downloadFlyersImage = autoFlyer;       // Launchbox Flyers (front/back)
            //     settings.downloadPlayfieldImage = autoPlayf;    // Vpin Playfield
            //     settings.downloadBackglassImage = autoBackg;    // Vpin Backglass
            //     settings.downloadDmdImage = autoDmd;          // Vpin DMD
            //     settings.downloadWheelImage = autoWheel;        // VPin Wheel
            //     // ui.configService()->saveConfig();
            // }

            // Rebuild Metadata
            bool cleanIndex = settings.forceRebuildMetadata;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            if (ImGui::Checkbox("Rebuild Metadata", &cleanIndex)) {
                settings.forceRebuildMetadata = cleanIndex;
                if (settings.forceRebuildMetadata){
                    settings.ignoreScanners = false;
                }
                ui.configService()->saveConfig();
            }
            ImGui::PopStyleColor(1);
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        // ---------- Rescan Tables Button ----------
        if (ImGui::Button("Rescan Tables")) {
            ui.rescanAsyncPublic(ui.scannerMode());
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
            ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Rescan Tables").c_str());
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    // ---------- Refresh Button ----------
    // blue
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.35f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f, 0.3f, 0.6f, 1.0f));

    if (ImGui::Button("Refresh")) {
        LOG_DEBUG("Refresh pressed");
        ui.setScannerMode(ScannerMode::HasIndex);
        ui.rescanAsyncPublic(ui.scannerMode());
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Refresh").c_str());
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    // ---------- Open Folder Button ----------
    if (ImGui::Button("Open Folder")) {
        std::string path;
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size()))
            path = ui.filteredTables()[ui.selectedIndex()].vpxFile;
        ui.actions().openFolder(path);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Open Folder").c_str());
    }
    ImGui::SameLine();

    ImVec4 yellow      = ImVec4(0.8f,0.7f,0.3f,0.85f);
    ImVec4 yellowHover = ImVec4(0.9f,0.8f,0.4f,0.85f);
    ImVec4 yellowActive= ImVec4(0.7f,0.6f,0.2f,0.85f);

    ImGui::PushStyleColor(ImGuiCol_Button,        yellow);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, yellowHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  yellowActive);

    // ---------- Extract or Open VBS Button ----------
    if (ImGui::Button("Extract VBS")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];

            ui.modal().openCommandOutput("Processing VBS...");

            ui.actions().extractOrOpenVbs(
                t.vpxFile,

                // onOutput
                [&ui](const std::string& line) {
                    ui.modal().appendCommandOutput(line);
                },

                // onFinished
                [&ui]() {
                    ui.modal().appendCommandOutput("Done.");
                }
            );
        } else {
            ui.modal().openInfo(
                "No Table Selected",
                "Please select a table first and try again."
            );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Extract VBS").c_str());
    }
    ImGui::SameLine();

    // ---------- Apply Patch Button ----------
    if (ImGui::Button("Apply Patch")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {

            // 1. Get the settings and dependencies
            const Settings& settings = ui.configService()->getSettings();
            TablePatcher* patcher = ui.tablePatcher();
            TableData* tableToPatch = nullptr;

            // 2. Map the filtered index back to the master table list
            //    Access to tables_ and filteredTables_ must be thread-safe.
            {
                std::lock_guard<std::mutex> lock(ui.tableMutex());

                // Get the unique file path of the currently selected (filtered) table
                const std::string& selectedPath = ui.filteredTables()[ui.selectedIndex()].vpxFile;

                // Find the matching TableData object in the master list
                for (auto& t : ui.tables()) {
                    if (t.vpxFile == selectedPath) {
                        tableToPatch = &t;
                        break;
                    }
                }
            }
            // 3. Call the single patch function on the found TableData
            if (patcher && tableToPatch) {
                LOG_INFO("Attempting single patch for: " + tableToPatch->title);

                if (patcher->patchSingleTable(settings, *tableToPatch)) {
                    LOG_DEBUG("Successfully patched table: " + tableToPatch->title);
                    ui.modal().openInfo("Patch Complete", tableToPatch->title + " was successfully patched.");
                } else {
                    LOG_DEBUG("Patch not applied or failed for: " + tableToPatch->title);
                    ui.modal().openWarning("Patch Status", tableToPatch->title + " did not require or failed to apply a patch.");
                }

                // Since we modified the table data (isPatched, hashFromVbs),
                // we should re-filter/re-sort to update the display if necessary.
                ui.filterAndSortTablesPublic();}
        } else {
            // Open confirm dialog before patching all tables
            ui.modal().openConfirm(
                "Confirm Patch All?",
                "This will apply patches to all tables in need.\nAre you sure you want to continue?",
                {"No", "Yes"},
                [&ui](const std::string& choice) {
                    if (choice == "Yes") {
                        LOG_DEBUG("Confirmed: Applying Patch to all tables in need");
                        ui.setScannerMode(ScannerMode::Patch);
                        ui.rescanAsyncPublic(ui.scannerMode());
                    } else {
                        LOG_INFO("Patch all canceled by user.");
                    }
                }
            );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Apply Patch").c_str());
    }
    ImGui::SameLine();

    ImGui::PopStyleColor(3);

    // ---------- Download Media Button ----------
    if (ImGui::Button("Download Media")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            LOG_DEBUG("[Placeholder] downloading media for table: " + t.vpxFile);
            ui.modal().openWarning(
                "A Table is Selected",
                "Please unselect a table first and try again."
                "Single table media downloading is not yet implemented."
            );
        } else {
            LOG_INFO("Opening Download Media Panel");
            ui.setShowDownloadMediaPanel(true);
            // Open confirm dialog before media downloading for all tables
            // ui.modal().openConfirm(
            //     "Confirm Download All?",
            //     "This will download images for all tables it finds.\nAre you sure you want to continue?",
            //     {"No", "Yes"},
            //     [&ui](const std::string& choice) {
            //         if (choice == "Yes") {
            //             LOG_DEBUG("Confirmed: Downloading tables to all tables found (hashed and matched)");
            //             ui.setScannerMode(ScannerMode::MediaDb);
            //             ui.rescanAsyncPublic(ui.scannerMode());
            //         } else {
            //             LOG_INFO("Download Media (all) canceled by user.");

            //         }
            //     }
            // );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Download Media").c_str());
    }
    ImGui::SameLine();

    // ---------- Screenshot Button ----------
    if (ImGui::Button("Screenshot")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            Uint32 currentTime = SDL_GetTicks();
            if (ui.inExternalAppMode_ || (currentTime - ui.lastExternalAppReturnTime_) < EditorUI::EXTERNAL_APP_DEBOUNCE_TIME_MS) {
                LOG_DEBUG("Screenshot mode skipped due to external app mode or debounce.");
            } else {
                LOG_DEBUG("Screenshot mode triggered from editor button");
                if (!ui.screenshotModeActive_ && ui.screenshotManager()) {

                    // Set flags *before* the thread
                    ui.screenshotModeActive_ = true;
                    ui.inExternalAppMode_ = true;

                    const auto& t = ui.filteredTables()[ui.selectedIndex()];
                    fs::path p(t.vpxFile);
                    std::string vpxFile_copy = t.vpxFile; // Copy for thread

                    // 1. Open modal *before* starting work
                    ui.modal().openProgress(
                        "Screenshot Mode",
                        "Launching " + p.filename().string() + "..."
                    );

                    // 2. Start thread for blocking work
                    std::thread([&ui, vpxFile_copy]() {
                        // --- Worker Thread ---
                        LOG_DEBUG("Worker Thread: Launching screenshot mode...");

                        // 3. This is the blocking call
                        ui.screenshotManager()->launchScreenshotMode(vpxFile_copy);

                        // 4. Update modal (this is thread-safe)
                        ui.modal().requestFinishProgress("");

                        // 5. Update flags *after* work is done
                        // NOTE: This is not strictly thread-safe, but will
                        // be picked up on the next frame after modal is closed.
                        // For simple debounce, it's fine.
                        ui.inExternalAppMode_ = false;
                        ui.screenshotModeActive_ = false;
                        ui.lastExternalAppReturnTime_ = SDL_GetTicks();
                        LOG_DEBUG("Worker Thread: Exited screenshot mode");
                        // --- End Worker Thread ---
                    }).detach();
                }
            }

        } else {
            LOG_WARN("Screenshot pressed but no table selected");
            ui.modal().openWarning(
                "No Table Selected",
                "Please select a table first and try again."
                "Bulk table screenshot is not yet implemented."
            );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Screenshot").c_str());
    }
    ImGui::SameLine();

    // blue
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.35f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f, 0.3f, 0.6f, 1.0f));

    // ---------- View Metadata Button ----------
    if (ImGui::Button("View Metadata")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            fs::path p(t.vpxFile);

            // Open the existing progress modal while loading
            ui.modal().openProgress("Loading Metadata", "Opening " + p.filename().string() + "...");

            // Start a thread to toggle metadata view
            std::thread([&ui]() {
                // Optional small delay for visual effect
                std::this_thread::sleep_for(std::chrono::milliseconds(150));

                // Show metadata view
                ui.setShowMetadataView(true);

                // Close modal silently
                ui.modal().requestFinishProgress("");  // empty message, no "Done!"
            }).detach();

            LOG_DEBUG("Toggling metadata view ON");
        } else {
            LOG_INFO("View Metadata pressed but no table selected");
            ui.modal().openInfo(
                "No Table Selected",
                "Please select a table first and try again."
            );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("View Metadata").c_str());
    }
    ImGui::SameLine();

    // ---------- Browse Tables Button ----------
    if (ImGui::Button("Browse Tables")) {
        ui.setShowVpsdbBrowser(true);
        LOG_DEBUG("Browse Tables pressed");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Browse Tables").c_str());
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImVec4 green      = ImVec4(0.24f, 0.74f, 0.24f, 1.0f);
    ImVec4 greenHover = ImVec4(0.20f, 0.55f, 0.20f, 1.0f);
    ImVec4 greenActive= ImVec4(0.12f, 0.35f, 0.12f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button,        green);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, greenHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  greenActive);

    // ---------- Play Selected Button ----------
    if (ImGui::Button("Play Selected")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {

            const auto& t_filtered = ui.filteredTables()[ui.selectedIndex()];
            fs::path p(t_filtered.vpxFile);

            // --- Implement External App Mode Flags (for proper editor handling) ---
            ui.inExternalAppMode_ = true;

            // 1. Open modal *before* starting work
            ui.modal().openProgress("Launching Game", "Starting " + p.filename().string() + "...");

            // Make copies of data needed for the thread
            auto t_filtered_copy = t_filtered;
            auto& tables = ui.tables();
            auto* launcher = ui.tableLauncher();

            // 2. Call the launch logic directly. This starts the asynchronous game thread internally (T3)
            // and returns immediately. The modal stays open until the final callback runs.
            ui.actions().launchTableWithStats(
                t_filtered_copy,
                tables, // The mutable master list
                launcher,
                // This is the refreshUICallback that runs AFTER the game exits and stats are saved.
                [&ui]() {
                    ui.requestPostLaunchCleanup();
                });

        } else {
            LOG_INFO("Play pressed but no table selected");
            ui.modal().openInfo(
                "No Table Selected",
                "You pressed 'Play' but no table was selected."
                "Please select a table first and try again."
            );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Play Selected").c_str());
    }
    ImGui::PopStyleColor(3);

    float exitWidth     = ImGui::CalcTextSize("Settings  Exit Editor").x + ImGui::GetStyle().FramePadding.x * 2.3f;
    float rightAlignPos = ImGui::GetContentRegionAvail().x - exitWidth;
    ImGui::SameLine(rightAlignPos);

    // ---------- Settings Button ----------
    if (ImGui::Button("Settings")) {
        ui.setShowEditorSettings(true);
        LOG_DEBUG("Settings pressed");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 buttonMin = ImGui::GetItemRectMin(); // Top-left corner of the button
        ImVec2 buttonMax = ImGui::GetItemRectMax(); // Bottom-right corner of the button
        ImVec2 pos = ImVec2(buttonMax.x, buttonMin.y);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f)); // bottom-right corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Settings").c_str());
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f,0.15f,0.15f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f,0.25f,0.25f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.6f,0.1f,0.1f,1.0f));

    // ---------- Exit Editor Button ----------
    if (ImGui::Button("Exit Editor")) {
        ui.requestExit();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 buttonMin = ImGui::GetItemRectMin(); // Top-left corner of the button
        ImVec2 buttonMax = ImGui::GetItemRectMax(); // Bottom-right corner of the button
        ImVec2 pos = ImVec2(buttonMax.x, buttonMin.y);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f)); // bottom-right corner of the tooltip
        ImGui::SetTooltip("%s", Tooltips::BUTTON_TOOLTIPS.at("Exit Editor").c_str());
    }

    ImGui::PopStyleColor(3);

    ImGui::EndGroup();

    // ---------- Footer Text ----------
    std::ostringstream ss;
    ss << ui.filteredTables().size() << " tables found";
    if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
        const auto& t = ui.filteredTables()[ui.selectedIndex()];
        fs::path p(t.vpxFile);
        ss << "  |  Selected: /" << p.parent_path().filename().string()
           << "/" << p.filename().string();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.94f, 0.94f, 1.0f)); // off-white (whitesmoke)
    ImGui::Text("%s", ss.str().c_str());
    ImGui::PopStyleColor();
}
}
