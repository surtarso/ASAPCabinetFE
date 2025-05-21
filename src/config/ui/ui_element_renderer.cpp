#include "config/ui/ui_element_renderer.h"
#include "ImGuiFileDialog.h"
#include "utils/logging.h"
#include <sstream>
#include <filesystem>
#include <algorithm>

#ifdef __linux__
#include <cstdlib>
#endif

namespace UIElementRenderer {

void renderKeybind(const std::string& key, std::string& value, InputHandler& inputHandler, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    std::string label = value.empty() ? "None" : value;
    ImGui::Text("%s", label.c_str());
    
    // Position the button closer to the key definition
    const float buttonOffset = 300.0f;
    ImGui::SameLine(buttonOffset); // Position after the label
    
    if (ImGui::Button("Set", ImVec2(60, 0))) {
        inputHandler.startCapturing(key);
        hasChanges = true;
    }
}

void renderColorPicker([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    std::vector<int> rgba(4, 0);
    std::stringstream ss(value);
    std::string token;
    for (int i = 0; i < 4 && std::getline(ss, token, ','); ++i) {
        rgba[i] = std::stoi(token);
    }
    float color[4] = {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f};
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    if (ImGui::ColorButton("##color", ImVec4(color[0], color[1], color[2], color[3]), ImGuiColorEditFlags_AlphaPreview, ImVec2(20, 20))) {
        ImGui::OpenPopup("ColorPicker");
    }
    if (ImGui::BeginPopup("ColorPicker")) {
        ImGui::ColorPicker4("##picker", color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
        value = std::to_string(int(color[0] * 255)) + "," +
                std::to_string(int(color[1] * 255)) + "," +
                std::to_string(int(color[2] * 255)) + "," +
                std::to_string(int(color[3] * 255));
        hasChanges = true;
        LOG_INFO("UiElementRenderer: Color updated: " << section << "." << key << " = " << value);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void renderFontPath([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, const std::vector<std::string>& availableFonts) {
    ImGui::SetNextItemWidth(-1); // Full width
    std::string displayValue = value.empty() ? "None" : std::filesystem::path(value).filename().string();
    static std::string preview = displayValue; // Retain preview across frames
    if (ImGui::BeginCombo("##fontCombo", preview.c_str())) {
        for (size_t i = 0; i < availableFonts.size(); ++i) {
            std::string fontName = std::filesystem::path(availableFonts[i]).filename().string();
            bool isSelected = (availableFonts[i] == value);
            if (ImGui::Selectable(fontName.c_str(), isSelected)) {
                value = availableFonts[i]; // Set full path as value
                preview = fontName; // Update preview to filename
                LOG_INFO("UiElementRenderer: Font selected: " << section << "." << key << " = " << value);
                hasChanges = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void renderPathOrExecutable([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    char buffer[1024];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(-60);
    if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
        value = std::string(buffer);
        LOG_DEBUG("UiElementRenderer: Text updated: " << section << "." << key << " = " << value);
        hasChanges = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse", ImVec2(50, 0))) {
        IGFD::FileDialogConfig config;
        config.path = (!value.empty() && std::filesystem::exists(value)) ? value : std::string(getenv("HOME"));
        config.flags = ImGuiFileDialogFlags_Modal;
        if (key.find("Path") != std::string::npos) {
            ImGuiFileDialog::Instance()->OpenDialog("FolderDlg_" + key, "Select Folder", nullptr, config);
        } else {
            ImGuiFileDialog::Instance()->OpenDialog("FileDlg_" + key, "Select Executable", "((.*))", config);
        }
    }
    ImVec2 maxSize = ImVec2(ImGui::GetIO().DisplaySize.x * 0.8f, ImGui::GetIO().DisplaySize.y * 0.8f);
    ImVec2 minSize = ImVec2(600, 400);
    if (key.find("Path") != std::string::npos &&
        ImGuiFileDialog::Instance()->Display("FolderDlg_" + key, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            value = ImGuiFileDialog::Instance()->GetCurrentPath();
            LOG_INFO("UiElementRenderer: Folder picked: " << section << "." << key << " = " << value);
            hasChanges = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
    if (key.find("VPinballXPath") != std::string::npos &&
        ImGuiFileDialog::Instance()->Display("FileDlg_" + key, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            value = ImGuiFileDialog::Instance()->GetFilePathName();
            LOG_INFO("UiElementRenderer: Executable picked: " << section << "." << key << " = " << value);
            hasChanges = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void renderCheckbox([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    bool boolValue = (value == "true");
    if (ImGui::Checkbox("##checkbox", &boolValue)) {
        value = boolValue ? "true" : "false";
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
    }
}

void renderDpiScale([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, SettingsSection& sectionData) {
    float dpiScale = 1.0f;
    try {
        dpiScale = std::stof(value);
    } catch (...) {
        LOG_ERROR("UiElementRenderer: Invalid DpiScale value: " << value << ", defaulting to 1.0");
    }
    bool isEnabled = false;
    for (const auto& kv : sectionData.keyValues) {
        if (kv.first == "EnableDpiScaling") {
            isEnabled = (kv.second == "true");
            break;
        }
    }
    if (!isEnabled) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##dpiScale", &dpiScale, 0.5f, 3.0f, "%.1f")) {
        value = std::to_string(dpiScale);
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
    }
    if (!isEnabled) {
        ImGui::PopStyleVar();
    }
}

void renderSliderInt([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, int min, int max) {
    int intValue = 0;
    try {
        intValue = std::stoi(value);
    } catch (...) {
        LOG_ERROR("UiElementRenderer: Invalid int value: " << value << ", defaulting to " << (min + (max - min) / 2));
        intValue = min + (max - min) / 2;
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##sliderInt", &intValue, min, max, "%d")) {
        value = std::to_string(intValue);
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
    }
}

// Helper function to snap rotation to valid values (0, ±90, ±180, ±270, ±360)
static int snapRotation(int value) {
    // Valid rotation values
    static const std::vector<int> validRotations = {-360, -270, -180, -90, 0, 90, 180, 270, 360};
    
    // Find the closest valid rotation
    int closest = 0;
    int minDiff = std::abs(value - closest);
    for (int rot : validRotations) {
        int diff = std::abs(value - rot);
        if (diff < minDiff) {
            minDiff = diff;
            closest = rot;
        }
    }
    return closest;
}

void renderRotationSlider([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, 
                         [[maybe_unused]] const std::string& section, int min, int max) {
    int intValue = 0;
    try {
        intValue = std::stoi(value);
        // Ensure initial value is a valid rotation
        intValue = snapRotation(intValue);
    } catch (...) {
        LOG_ERROR("UiElementRenderer: Invalid rotation value: " << value << ", defaulting to 0");
        intValue = 0; // Default to 0 for invalid inputs
    }
    
    ImGui::SetNextItemWidth(-1);
    int tempValue = intValue; // Use a temporary value for the slider
    if (ImGui::SliderInt(("##rotationSlider_" + key).c_str(), &tempValue, min, max, "%d°")) {
        // Snap the value when the slider is released or changed
        int snappedValue = snapRotation(tempValue);
        if (snappedValue != intValue) {
            value = std::to_string(snappedValue);
            hasChanges = true;
            LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
        }
    }
}

static void openUrl(const std::string& url) {
    #ifdef __linux__
    std::string command = "xdg-open " + url;
    if (system(command.c_str()) != 0) {
        LOG_ERROR("UiElementRenderer: Failed to open URL: " << url);
    }
    #else
    LOG_ERROR("UiElementRenderer: URL opening not implemented for this platform");
    #endif
}

void renderVideoBackendDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges,[[maybe_unused]] const std::string& section) {
    const char* options[] = {"vlc", "ffmpeg"};
    int videoBackend = (value == "ffmpeg") ? 1 : 0;

    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##videoBackend", &videoBackend, options, IM_ARRAYSIZE(options))) {
        std::string oldValue = value;
        value = options[videoBackend];
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
    }
}

void renderTitleDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges,[[maybe_unused]] const std::string& section, IConfigService* configService) {
    const char* options[] = {"filename", "metadata"};
    int titleSource = (value == "metadata") ? 1 : 0;

    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##titleSource", &titleSource, options, IM_ARRAYSIZE(options))) {
        std::string oldValue = value;
        value = options[titleSource];
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
        
        if (value == "metadata" && oldValue != "metadata") {
            LOG_DEBUG("UiElementRenderer: Checking for vpxtool_index.json");
            const auto& iniData = configService->getIniData();
            auto vpxIt = iniData.find("VPX");
            if (vpxIt != iniData.end()) {
                LOG_DEBUG("UiElementRenderer: VPX section found");
                auto pathIt = std::find_if(vpxIt->second.keyValues.begin(), vpxIt->second.keyValues.end(),
                                           [](const auto& pair) { return pair.first == "VPXTablesPath"; });
                if (pathIt != vpxIt->second.keyValues.end()) {
                    LOG_DEBUG("UiElementRenderer: VPXTablesPath found: " << pathIt->second);
                    if (!pathIt->second.empty()) {
                        std::filesystem::path jsonPath = std::filesystem::path(pathIt->second) / "vpxtool_index.json";
                        LOG_DEBUG("UiElementRenderer: Checking path: " << jsonPath.string());
                        if (!std::filesystem::exists(jsonPath)) {
                            LOG_DEBUG("UiElementRenderer: vpxtool_index.json not found, opening popup");
                            ImGui::OpenPopup("Metadata Error");
                        } else {
                            LOG_DEBUG("UiElementRenderer: vpxtool_index.json exists");
                        }
                    } else {
                        LOG_DEBUG("UiElementRenderer: VPXTablesPath is empty");
                    }
                } else {
                    LOG_DEBUG("UiElementRenderer: VPXTablesPath not found in VPX section");
                }
            } else {
                LOG_DEBUG("UiElementRenderer: VPX section not found in iniData");
            }
        }
    }
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Metadata Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        ImGui::TextWrapped("Error: 'vpxtool_index.json' not found in the configured tables path.");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::TextWrapped("Please ensure 'vpxtool' is installed and rescan the tables path.");

        ImGui::Spacing();

        const char* url = "https://github.com/francisdb/vpxtool/";
        ImGui::Indent();
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "%s", url);
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("Open in browser");
        }
        if (ImGui::IsItemClicked()) {
            LOG_DEBUG("UiElementRenderer: Opening URL: " << url);
            openUrl(url);
        }
        ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
            LOG_DEBUG("UiElementRenderer: Closing Metadata Error");
            value = "filename";
            titleSource = 0;
            hasChanges = true;
            LOG_DEBUG("UiElementRenderer: Reverted: " << section << "." << key << " = " << value);
            ImGui::CloseCurrentPopup();
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        ImGui::EndPopup();
    }
}

void renderMetadataCheckbox([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, IConfigService* configService) {
    bool boolValue = (value == "true");
    if (ImGui::Checkbox("##checkbox", &boolValue)) {
        value = boolValue ? "true" : "false";
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
        
        // Only perform the check and open popup if the checkbox is being set to true
        if (boolValue) { 
            LOG_DEBUG("UiElementRenderer: Checking for vpxtool_index.json");
            const auto& iniData = configService->getIniData();
            auto vpxIt = iniData.find("VPX");
            if (vpxIt != iniData.end()) {
                LOG_DEBUG("UiElementRenderer: VPX section found");
                auto pathIt = std::find_if(vpxIt->second.keyValues.begin(), vpxIt->second.keyValues.end(),
                                        [](const auto& pair) { return pair.first == "VPXTablesPath"; });
                if (pathIt != vpxIt->second.keyValues.end()) {
                    LOG_DEBUG("UiElementRenderer: VPXTablesPath found: " << pathIt->second);
                    if (!pathIt->second.empty()) {
                        std::filesystem::path jsonPath = std::filesystem::path(pathIt->second) / "vpxtool_index.json";
                        LOG_DEBUG("UiElementRenderer: Checking path: " << jsonPath.string());
                        if (!std::filesystem::exists(jsonPath)) {
                            LOG_DEBUG("UiElementRenderer: vpxtool_index.json not found, opening popup");
                            ImGui::OpenPopup("Metadata Error");
                        } else {
                            LOG_DEBUG("UiElementRenderer: vpxtool_index.json exists");
                        }
                    } else {
                        LOG_DEBUG("UiElementRenderer: VPXTablesPath is empty");
                    }
                } else {
                    LOG_DEBUG("UiElementRenderer: VPXTablesPath not found in VPX section");
                }
            }
        }
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Metadata Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        ImGui::TextWrapped("Error: 'vpxtool_index.json' not found in the configured tables path.");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::TextWrapped("Please ensure 'vpxtool' is installed and rescan the tables path.");

        ImGui::Spacing();

        const char* url = "https://github.com/francisdb/vpxtool/";
        ImGui::Indent();
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "%s", url);
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("Open in browser");
        }
        if (ImGui::IsItemClicked()) {
            LOG_DEBUG("UiElementRenderer: Opening URL: " << url);
            openUrl(url);
        }
        ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
            LOG_DEBUG("UiElementRenderer: Closing Metadata Error");
            // Set value back to false when "OK" is clicked in the error popup
            value = "false"; 
            hasChanges = true;
            LOG_DEBUG("UiElementRenderer: Reverted: " << section << "." << key << " = " << value);
            ImGui::CloseCurrentPopup();
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        ImGui::EndPopup();
    }
}

void renderResolution([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    char buffer[16];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputText("##res", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsDecimal)) {
        value = std::string(buffer);
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
    }
    ImGui::SameLine();
    const char* commonRes[] = {"128", "256", "512", "600", "720", "768", "800", "900", "1024", "1080", "1200", "1280", "1366", "1440", "1600", "1920", "2160", "2560", "3840"};
    int currentIndex = -1;
    for (int i = 0; i < IM_ARRAYSIZE(commonRes); ++i) {
        if (value == commonRes[i]) {
            currentIndex = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(100);
    if (ImGui::Combo("##commonRes", &currentIndex, commonRes, IM_ARRAYSIZE(commonRes))) {
        if (currentIndex >= 0) {
            value = commonRes[currentIndex];
            hasChanges = true;
            LOG_DEBUG("UiElementRenderer: Selected resolution: " << section << "." << key << " = " << value);
        }
    }
}

void renderGenericText([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    char buffer[1024];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
        value = std::string(buffer);
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
        hasChanges = true;
    }
}

void renderGenericTextShort([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    char buffer[16];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputText("##value", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsDecimal)) {
        value = std::string(buffer);
        LOG_DEBUG("UiElementRenderer: Updated: " << section << "." << key << " = " << value);
        hasChanges = true;
    }
}

} // namespace UIElementRenderer