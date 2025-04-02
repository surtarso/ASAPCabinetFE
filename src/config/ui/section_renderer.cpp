#include "config/ui/section_renderer.h"
#include "utils/tooltips.h"
#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "utils/logging.h"
#include <sstream>
#include <filesystem>
#include <algorithm>

SectionRenderer::SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler)
    : configService_(configService), currentSection_(currentSection), inputHandler_(inputHandler) {}

void SectionRenderer::renderSectionsPane(const std::vector<std::string>& sectionOrder) {
    ImGui::BeginChild("SectionsPane", ImVec2(250, 0), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Configuration Sections");
    ImGui::Separator();

    for (const auto& section : sectionOrder) {
        if (configService_->getIniData().count(section)) {
            bool selected = currentSection_ == section;
            if (ImGui::Selectable(section.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                currentSection_ = section;
                LOG_DEBUG("Selected section: " << section);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void SectionRenderer::renderKeyValuesPane(std::map<std::string, SettingsSection>& iniData) {
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, 0), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Settings - %s", currentSection_.c_str());
    ImGui::Separator();

    if (!currentSection_.empty() && iniData.count(currentSection_)) {
        auto& section = iniData[currentSection_];
        float maxKeyWidth = 150.0f;
        for (const auto& [key, _] : section.keyValues) {
            float keyWidth = ImGui::CalcTextSize(key.c_str()).x + 10.0f;
            maxKeyWidth = std::max(maxKeyWidth, std::min(keyWidth, 250.0f));
        }

        // Dialog sizing—big enough to browse, small enough to fit
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 maxSize = ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f);
        ImVec2 minSize = ImVec2(600, 400);

        for (auto& [key, value] : section.keyValues) {
            ImGui::PushID(key.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s:", key.c_str());
            ImGui::SameLine(maxKeyWidth);

            // Tooltip "?"—green and handy
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            if (ImGui::Button("?", ImVec2(16, 0))) {}
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            renderTooltip(key);
            ImGui::SameLine();

            if (currentSection_ == "Keybinds") {
                std::string label = value.empty() ? "None" : value;
                ImGui::Text("%s", label.c_str());
                ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
                if (ImGui::Button("Set", ImVec2(60, 0))) {
                    inputHandler_.startCapturing(key);
                }
            } else if (key == "FontColor" || key == "FontBgColor") {
                std::vector<int> rgba(4);
                std::stringstream ss(value);
                std::string token;
                for (int i = 0; i < 4 && std::getline(ss, token, ','); ++i) {
                    rgba[i] = std::stoi(token);
                }
                float color[4] = {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f};
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                if (ImGui::ColorButton("##color", ImVec4(color[0], color[1], color[2], color[3]), ImGuiColorEditFlags_AlphaPreview, ImVec2(20, 20))) {
                    ImGui::OpenPopup("ColorPicker");
                }
                if (ImGui::BeginPopup("ColorPicker")) {
                    ImGui::ColorPicker4("##picker", color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
                    value = std::to_string(int(color[0] * 255)) + "," + 
                            std::to_string(int(color[1] * 255)) + "," + 
                            std::to_string(int(color[2] * 255)) + "," + 
                            std::to_string(int(color[3] * 255));
                    LOG_DEBUG("Color updated: " << currentSection_ << "." << key << " = " << value);
                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();
            } else if (key.find("Path") != std::string::npos || key.find("ExecutableCmd") != std::string::npos || key == "FontPath") {
                // Text field—where the magic lands
                char buffer[1024];
                strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::SetNextItemWidth(-60);
                if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                    value = std::string(buffer);
                    LOG_DEBUG("Text updated: " << currentSection_ << "." << key << " = " << value);
                }
                ImGui::SameLine();
                if (ImGui::Button("Browse", ImVec2(50, 0))) {
                    IGFD::FileDialogConfig config;
                    // Safe path—fallback if empty or invalid
                    if (!value.empty() && std::filesystem::exists(value)) {
                        config.path = value;
                    } else if (key == "FontPath") {
                        config.path = "/usr/share/fonts/";
                    } else {
                        config.path = std::string(getenv("HOME"));
                    }
                    config.flags = ImGuiFileDialogFlags_Modal; // Lock ConfigUI
                    if (key == "FontPath") {
                        ImGuiFileDialog::Instance()->OpenDialog("FontDlg", "Select Font", ".ttf", config);
                    } else if (key.find("Path") != std::string::npos) {
                        ImGuiFileDialog::Instance()->OpenDialog("FolderDlg_" + key, "Select Folder", nullptr, config);
                    } else if (key.find("ExecutableCmd") != std::string::npos) {
                        ImGuiFileDialog::Instance()->OpenDialog("FileDlg_" + key, "Select Executable", "((.*))", config);
                    }
                }
                // Dialogs—sync value and text field
                if (key == "FontPath" && ImGuiFileDialog::Instance()->Display("FontDlg", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = std::filesystem::path(ImGuiFileDialog::Instance()->GetFilePathName()).filename().string();
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        LOG_DEBUG("Font picked: " << currentSection_ << "." << key << " = " << value);
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
                if (key.find("Path") != std::string::npos && 
                    ImGuiFileDialog::Instance()->Display("FolderDlg_" + key, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = ImGuiFileDialog::Instance()->GetCurrentPath(); // Folder fix!
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        LOG_DEBUG("Folder picked: " << currentSection_ << "." << key << " = " << value);
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
                if (key.find("ExecutableCmd") != std::string::npos && 
                    ImGuiFileDialog::Instance()->Display("FileDlg_" + key, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = ImGuiFileDialog::Instance()->GetFilePathName();
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        LOG_DEBUG("Executable picked: " << currentSection_ << "." << key << " = " << value);
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            } else {
                // Generic text input—no fuss
                char buffer[1024];
                strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                    value = std::string(buffer);
                    LOG_DEBUG("Updated: " << currentSection_ << "." << key << " = " << value);
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void SectionRenderer::renderTooltip(const std::string& key) {
    auto tooltips = Tooltips::getTooltips();
    if (tooltips.count(key) && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tooltips[key].c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}