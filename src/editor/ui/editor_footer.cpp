#include "editor/ui/editor_footer.h"
#include "utils/editor_tooltips.h"
#include "log/logging.h"
#include <imgui.h>
#include <filesystem>
#include <sstream>

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
            ImGui::TextDisabled("Scanner Mode");
            if (ImGui::Selectable("File Scanner", ui.scannerMode() == ScannerMode::File))
                ui.setScannerMode(ScannerMode::File);
            if (ImGui::Selectable("VPin Scanner", ui.scannerMode() == ScannerMode::VPin))
                ui.setScannerMode(ScannerMode::VPin);
            if (ImGui::Selectable("VPSDb Scanner", ui.scannerMode() == ScannerMode::VPSDb))
                ui.setScannerMode(ScannerMode::VPSDb);

            ImGui::TextDisabled("Options");
            Settings& settings = ui.configService()->getMutableSettings();

            bool cleanIndex = settings.forceRebuildMetadata;
            if (ImGui::Checkbox("Rebuild Metadata", &cleanIndex)) {
                settings.forceRebuildMetadata = cleanIndex;
                if (settings.forceRebuildMetadata){
                    settings.ignoreScanners = false;
                }
                ui.configService()->saveConfig();
            }

            // Use external VPXTOOLS
            bool wantsVpxtool = settings.useVpxtool;
            if (ImGui::Checkbox("Use External VPXTool", &wantsVpxtool)) {
                settings.useVpxtool = wantsVpxtool;
                ui.configService()->saveConfig();
            }

            // Auto patch tables
            bool autoPatch = settings.autoPatchTables;
            if (ImGui::Checkbox("Auto-Patch tables", &autoPatch)) {
                LOG_INFO(std::string("Auto-Patch tables on Rescan toggled: ") + (autoPatch ? "ON" : "OFF"));
                settings.autoPatchTables = autoPatch;  // invert
                ui.configService()->saveConfig();
            }

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
            ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Rescan Tables").c_str());
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    // ---------- Refresh Button ----------
    // blue
    // ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.35f, 0.7f, 1.0f));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f, 0.3f, 0.6f, 1.0f));

    // if (ImGui::Button("Refresh")) {
    //     LOG_DEBUG("Refresh pressed");
    //     ui.setScannerMode(ScannerMode::HasIndex);
    //     ui.rescanAsyncPublic(ui.scannerMode());
    // }
    // if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    //     ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
    //     ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
    //     ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Refresh").c_str());
    // }
    // ImGui::PopStyleColor(3);
    // ImGui::SameLine();

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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Open Folder").c_str());
    }
    ImGui::SameLine();

    ImVec4 yellow      = ImVec4(0.8f,0.7f,0.3f,0.85f);
    ImVec4 yellowHover = ImVec4(0.9f,0.8f,0.4f,0.85f);
    ImVec4 yellowActive= ImVec4(0.7f,0.6f,0.2f,0.85f);

    ImGui::PushStyleColor(ImGuiCol_Button,        yellow);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, yellowHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  yellowActive);

    // ---------- Extract VBS Button ----------
    if (ImGui::Button("Extract VBS")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            ui.actions().extractVBS(t.vpxFile);
            fs::path vpxPath(t.vpxFile);
            std::string vbsPath = (vpxPath.parent_path() / (vpxPath.stem().string() + ".vbs")).string();
            if (fs::exists(vbsPath)) {
                ui.actions().openInExternalEditor(vbsPath);
            } else {
                LOG_WARN("Tried to open VBS, but extraction failed or file not found at: " + vbsPath);
                ui.modal().openWarning(
                    "File Not Found",
                    "The specified file " + vbsPath + " could not be found or deleted."
                );
            }
        } else {
            LOG_INFO("Extract VBS pressed but no table selected");
            ui.modal().openInfo(
                "No Table Selected",
                "Please select a table first and try again."
            );
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImVec2 pos = ImGui::GetItemRectMin(); // top-left corner of the button
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // bottom-left corner of the tooltip
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Extract VBS").c_str());
    }
    ImGui::SameLine();

    // ---------- Apply Patch Button ----------
    if (ImGui::Button("Apply Patch")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            LOG_WARN("[Placeholder] Patching table: " + t.vpxFile);
            ui.modal().openWarning(
                "A Table is Selected",
                "Please unselect a table first and try again."
                "Single table patching is not yet implemented."
            );
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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Apply Patch").c_str());
    }
    ImGui::SameLine();

    ImGui::PopStyleColor(3);

    // ---------- Download Media Button ----------
    if (ImGui::Button("Download Media")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            LOG_WARN("[Placeholder] downloading media for table: " + t.vpxFile);
            ui.modal().openWarning(
                "A Table is Selected",
                "Please unselect a table first and try again."
                "Single table media downloading is not yet implemented."
            );
        } else {
            // Open confirm dialog before media downloading for all tables
            ui.modal().openConfirm(
                "Confirm Download All?",
                "This will download images for all tables it finds.\nAre you sure you want to continue?",
                {"No", "Yes"},
                [&ui](const std::string& choice) {
                    if (choice == "Yes") {
                        LOG_DEBUG("Confirmed: Downloading tables to all tables found (hashed and matched)");
                        ui.setScannerMode(ScannerMode::MediaDb);
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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Download Media").c_str());
    }
    ImGui::SameLine();

    // ---------- Screenshot Button ----------
    if (ImGui::Button("Screenshot")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& ta = ui.filteredTables()[ui.selectedIndex()];

            LOG_INFO("Screenshot pressed for table: " + ta.vpxFile);
            Uint32 currentTime = SDL_GetTicks();
            if (ui.inExternalAppMode_ || (currentTime - ui.lastExternalAppReturnTime_) < EditorUI::EXTERNAL_APP_DEBOUNCE_TIME_MS) {
                LOG_DEBUG("Screenshot mode skipped due to external app mode or debounce.");
            } else {
                LOG_DEBUG("Screenshot mode triggered from editor button");
                if (!ui.screenshotModeActive_ && ui.screenshotManager()) {
                    // optional UI sound
                    // soundManager_->playUISound("launch_screenshot");

                    ui.screenshotModeActive_ = true;
                    ui.inExternalAppMode_ = true;

                    const auto& t = ui.filteredTables()[ui.selectedIndex()];
                    ui.screenshotManager()->launchScreenshotMode(t.vpxFile);

                    ui.inExternalAppMode_ = false;
                    ui.screenshotModeActive_ = false;
                    ui.lastExternalAppReturnTime_ = SDL_GetTicks();
                    LOG_DEBUG("Exited screenshot mode");
                }
            }
            // ui.screenshotManager()->launchScreenshotMode(t.vpxFile);

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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Screenshot").c_str());
    }
    ImGui::SameLine();

    // blue
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.35f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f, 0.3f, 0.6f, 1.0f));

    // ---------- View Metadata Button ----------
    if (ImGui::Button("View Metadata")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            ui.setShowMetadataView(true);
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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("View Metadata").c_str());
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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Browse Tables").c_str());
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

            // Call the launch logic in ButtonActions
            ui.actions().launchTableWithStats(
                t_filtered,
                ui.tables(), // The mutable master list
                ui.tableLauncher(),
                [&ui](){ ui.filterAndSortTablesPublic(); } // Callback to refresh UI
            );

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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Play Selected").c_str());
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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Settings").c_str());
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
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Exit Editor").c_str());
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
