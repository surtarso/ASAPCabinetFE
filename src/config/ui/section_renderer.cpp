#include "config/ui/section_renderer.h"
#include "utils/tooltips.h"
#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "utils/logging.h"
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <iomanip>

// Constructor: Scan fonts with full paths
SectionRenderer::SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler)
    : configService_(configService), currentSection_(currentSection), inputHandler_(inputHandler) {
    std::string fontDir = "/usr/share/fonts/";
    if (std::filesystem::exists(fontDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(fontDir)) {
            if (entry.path().extension() == ".ttf") {
                availableFonts_.push_back(entry.path().string()); // Full paths
            }
        }
        std::sort(availableFonts_.begin(), availableFonts_.end());
        LOG_DEBUG("Found " << availableFonts_.size() << " .ttf fonts in " << fontDir);
    } else {
        LOG_DEBUG("Font directory " << fontDir << " not found!");
    }
}

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

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 maxSize = ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f);
        ImVec2 minSize = ImVec2(600, 400);

        std::vector<std::pair<std::string, std::string>> keyValuesCopy = section.keyValues;
        for (auto& [key, value] : keyValuesCopy) {
            std::string keyCopy = key;
            ImGui::PushID(keyCopy.c_str());
            //LOG_DEBUG("Rendering key: " << keyCopy);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s:", keyCopy.c_str());
            ImGui::SameLine(maxKeyWidth);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            if (ImGui::Button("?", ImVec2(16, 0))) {}
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            renderTooltip(keyCopy);
            ImGui::SameLine();

            if (currentSection_ == "Keybinds") {
                std::string label = value.empty() ? "None" : value;
                ImGui::Text("%s", label.c_str());
                ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
                if (ImGui::Button("Set", ImVec2(60, 0))) {
                    inputHandler_.startCapturing(keyCopy);
                }
            } else if (keyCopy == "FontColor" || keyCopy == "FontBgColor") {
                std::vector<int> rgba(4);
                std::stringstream ss;
                ss << value;  // Initialize stringstream with value
                std::string token;
                for (int i = 0; i < 4 && std::getline(ss, token, ','); ++i) {
                    rgba[i] = std::stoi(token);
                }
                float color[4] = {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f};
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
                if (ImGui::ColorButton("##color", ImVec4(color[0], color[1], color[2], color[3]), ImGuiColorEditFlags_AlphaPreview, ImVec2(20, 20))) {
                    ImGui::OpenPopup("ColorPicker");
                }
                bool popupOpen = ImGui::BeginPopup("ColorPicker");
                if (popupOpen) {
                    ImGui::ColorPicker4("##picker", color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
                    value = std::to_string(int(color[0] * 255)) + "," +
                            std::to_string(int(color[1] * 255)) + "," +
                            std::to_string(int(color[2] * 255)) + "," +
                            std::to_string(int(color[3] * 255));
                    hasChanges_ = true;
                    LOG_DEBUG("Color updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();
            } else if (keyCopy == "FontPath") {
                ImGui::SetNextItemWidth(-1);
                std::string displayValue = value.empty() ? "None" : std::filesystem::path(value).filename().string();
                static std::string preview = displayValue;
                if (ImGui::BeginCombo("##fontCombo", preview.c_str())) {
                    for (size_t i = 0; i < availableFonts_.size(); ++i) {
                        std::string fontName = std::filesystem::path(availableFonts_[i]).filename().string();
                        bool isSelected = (availableFonts_[i] == value);
                        if (ImGui::Selectable(fontName.c_str(), isSelected)) {
                            value = availableFonts_[i];
                            preview = fontName;
                            LOG_DEBUG("Font selected: " << currentSection_ << "." << keyCopy << " = " << value);
                            hasChanges_ = true;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            } else if (keyCopy.find("Path") != std::string::npos || keyCopy.find("ExecutableCmd") != std::string::npos) {
                char buffer[1024];
                strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::SetNextItemWidth(-60);
                if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                    value = std::string(buffer);
                    LOG_DEBUG("Text updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    hasChanges_ = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Browse", ImVec2(50, 0))) {
                    IGFD::FileDialogConfig config;
                    if (!value.empty() && std::filesystem::exists(value)) {
                        config.path = value;
                    } else {
                        config.path = std::string(getenv("HOME"));
                    }
                    config.flags = ImGuiFileDialogFlags_Modal;
                    if (keyCopy.find("Path") != std::string::npos) {
                        ImGuiFileDialog::Instance()->OpenDialog("FolderDlg_" + keyCopy, "Select Folder", nullptr, config);
                    } else if (keyCopy.find("ExecutableCmd") != std::string::npos) {
                        ImGuiFileDialog::Instance()->OpenDialog("FileDlg_" + keyCopy, "Select Executable", "((.*))", config);
                    }
                }
                if (keyCopy.find("Path") != std::string::npos && 
                    ImGuiFileDialog::Instance()->Display("FolderDlg_" + keyCopy, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = ImGuiFileDialog::Instance()->GetCurrentPath();
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        LOG_DEBUG("Folder picked: " << currentSection_ << "." << keyCopy << " = " << value);
                        hasChanges_ = true;
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
                if (keyCopy.find("ExecutableCmd") != std::string::npos && 
                    ImGuiFileDialog::Instance()->Display("FileDlg_" + keyCopy, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = ImGuiFileDialog::Instance()->GetFilePathName();
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        LOG_DEBUG("Executable picked: " << currentSection_ << "." << keyCopy << " = " << value);
                        hasChanges_ = true;
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            } else {
                char buffer[1024];
                strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                    value = std::string(buffer);
                    LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    hasChanges_ = true;
                }
            }
            ImGui::PopID();
        }
        section.keyValues = keyValuesCopy;
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
