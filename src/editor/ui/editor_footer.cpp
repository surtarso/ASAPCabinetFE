#include "editor/ui/editor_footer.h"
#include "utils/editor_tooltips.h"
#include "log/logging.h"
#include <imgui.h>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace editor_footer {

void drawFooter(EditorUI& ui) {
    // Last scan info
    if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
        const auto& t = ui.filteredTables()[ui.selectedIndex()];
        if (!t.jsonOwner.empty()) {
            ImGui::TextDisabled("Last table scanner: %s", t.jsonOwner.c_str());
        }
    }

    ImGui::Separator();
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

        if (ImGui::BeginCombo("##rescan_combo", comboLabel.c_str(), ImGuiComboFlags_NoPreview)) {
            ImGui::TextDisabled("Scanner Mode");
            if (ImGui::Selectable("File Scanner", ui.scannerMode() == ScannerMode::File))
                ui.setScannerMode(ScannerMode::File);
            if (ImGui::Selectable("VPin Scanner", ui.scannerMode() == ScannerMode::VPin))
                ui.setScannerMode(ScannerMode::VPin);
            if (ImGui::Selectable("VPSDb Scanner", ui.scannerMode() == ScannerMode::VPSDb))
                ui.setScannerMode(ScannerMode::VPSDb);

            ImGui::TextDisabled("Options");
            // ImGui::Checkbox("Force Rebuild Metadata", &ui.forceRebuildMetadata());
            bool flag = ui.forceRebuildMetadata();
            if (ImGui::Checkbox("Force Rebuild Metadata", &flag)) {
                ui.setForceRebuildMetadata(flag);
            }
            // ImGui::Checkbox("Use External VPXTool",        &ui.useVpxtool());
            bool flagUse = ui.useVpxtool();
            if (ImGui::Checkbox("Use External VPXTool", &flagUse)) {
                ui.setUseVpxtool(flagUse);
            }

            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Rescan Tables")) {
            ui.rescanAsyncPublic(ui.scannerMode());
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Rescan Tables").c_str());
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.35f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f, 0.3f, 0.6f, 1.0f));

    if (ImGui::Button("Refresh")) {
        LOG_DEBUG("Refresh pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Refresh").c_str());
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    if (ImGui::Button("Open Folder")) {
        std::string path;
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size()))
            path = ui.filteredTables()[ui.selectedIndex()].vpxFile;
        ui.actions().openFolder(path);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Open Folder").c_str());
    }
    ImGui::SameLine();

    ImVec4 yellow      = ImVec4(0.8f,0.7f,0.3f,0.85f);
    ImVec4 yellowHover = ImVec4(0.9f,0.8f,0.4f,0.85f);
    ImVec4 yellowActive= ImVec4(0.7f,0.6f,0.2f,0.85f);

    ImGui::PushStyleColor(ImGuiCol_Button,        yellow);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, yellowHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  yellowActive);

    if (ImGui::Button("Extract VBS")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            ui.actions().extractVBS(t.vpxFile);
            fs::path vpxPath(t.vpxFile);
            std::string vbsPath = (vpxPath.parent_path() / (vpxPath.stem().string() + ".vbs")).string();
            if (fs::exists(vbsPath)) {
                ui.actions().openInExternalEditor(vbsPath);
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

    if (ImGui::Button("Apply Patch")) {
        LOG_DEBUG("Apply Patch pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Apply Patch").c_str());
    }
    ImGui::SameLine();

    ImGui::PopStyleColor(3);

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
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            ui.showMetadataEditor_ = true;
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
        ui.showVpsdbBrowser_ = true;
        LOG_DEBUG("Browse Tables pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Browse Tables").c_str());
    }
    ImGui::SameLine();

    ImVec4 green      = ImVec4(0.2f,0.7f,0.2f,1.0f);
    ImVec4 greenHover = ImVec4(0.3f,0.8f,0.3f,1.0f);
    ImVec4 greenActive= ImVec4(0.1f,0.6f,0.1f,1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button,        green);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, greenHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  greenActive);

    if (ImGui::Button("Play Selected")) {
        if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
            const auto& t = ui.filteredTables()[ui.selectedIndex()];
            ui.tableLauncher()->launchTable(t);
        } else {
            LOG_DEBUG("Play pressed but no table selected");
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Play Selected").c_str());
    }
    ImGui::PopStyleColor(3);

    float exitWidth     = ImGui::CalcTextSize("Settings  Exit Editor").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float rightAlignPos = ImGui::GetContentRegionAvail().x - exitWidth;
    ImGui::SameLine(rightAlignPos);

    if (ImGui::Button("Settings")) {
        LOG_DEBUG("Settings pressed (placeholder)");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Settings").c_str());
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f,0.15f,0.15f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f,0.25f,0.25f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.6f,0.1f,0.1f,1.0f));

    if (ImGui::Button("Exit Editor")) {
        ui.requestExit();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip(Tooltips::BUTTON_TOOLTIPS.at("Exit Editor").c_str());
    }

    ImGui::PopStyleColor(3);

    ImGui::EndGroup();

    std::ostringstream ss;
    ss << ui.filteredTables().size() << " tables found";
    if (ui.selectedIndex() >= 0 && ui.selectedIndex() < static_cast<int>(ui.filteredTables().size())) {
        const auto& t = ui.filteredTables()[ui.selectedIndex()];
        fs::path p(t.vpxFile);
        ss << "  |  Selected: /" << p.parent_path().filename().string()
           << "/" << p.filename().string();
    }
    ImGui::TextDisabled("%s", ss.str().c_str());
}
}
