#include "config/ui/ui_element_renderer.h"
#include "ImGuiFileDialog.h"
#include "utils/logging.h"
#include <sstream>
#include <filesystem>

namespace UIElementRenderer {

void renderKeybind(const std::string& key, std::string& value, InputHandler& inputHandler) {
    std::string label = value.empty() ? "None" : value;
    ImGui::Text("%s", label.c_str());
    
    // Position the button closer to the key definition
    const float buttonOffset = 300.0f; // Manual tweak point: adjust this value
    ImGui::SameLine(buttonOffset); // Position after the label, tweakable
    
    if (ImGui::Button("Set", ImVec2(60, 0))) {
        inputHandler.startCapturing(key);
    }
}

void renderColorPicker(const std::string& key, std::string& value, bool& hasChanges, const std::string& section) {
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
        LOG_INFO("Color updated: " << section << "." << key << " = " << value);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void renderFontPath(const std::string& key, std::string& value, bool& hasChanges, const std::string& section, const std::vector<std::string>& availableFonts) {
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
                LOG_INFO("Font selected: " << section << "." << key << " = " << value);
                hasChanges = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void renderPathOrExecutable(const std::string& key, std::string& value, bool& hasChanges, const std::string& section) {
    char buffer[1024];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(-60);
    if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
        value = std::string(buffer);
        LOG_DEBUG("Text updated: " << section << "." << key << " = " << value);
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
            LOG_INFO("Folder picked: " << section << "." << key << " = " << value);
            hasChanges = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
    if (key.find("ExecutableCmd") != std::string::npos &&
        ImGuiFileDialog::Instance()->Display("FileDlg_" + key, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            value = ImGuiFileDialog::Instance()->GetFilePathName();
            LOG_INFO("Executable picked: " << section << "." << key << " = " << value);
            hasChanges = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void renderCheckbox(const std::string& key, std::string& value, bool& hasChanges, const std::string& section) {
    bool boolValue = (value == "true");
    if (ImGui::Checkbox("##checkbox", &boolValue)) {
        value = boolValue ? "true" : "false";
        hasChanges = true;
        LOG_DEBUG("Updated: " << section << "." << key << " = " << value);
    }
}

void renderDpiScale(const std::string& key, std::string& value, bool& hasChanges, const std::string& section, SettingsSection& sectionData) {
    float dpiScale = 1.0f;
    try {
        dpiScale = std::stof(value);
    } catch (...) {
        LOG_ERROR("Invalid DpiScale value: " << value << ", defaulting to 1.0");
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
        LOG_DEBUG("Updated: " << section << "." << key << " = " << value);
    }
    if (!isEnabled) {
        ImGui::PopStyleVar();
    }
}

void renderSliderInt(const std::string& key, std::string& value, bool& hasChanges, const std::string& section, int min, int max) {
    int intValue = 0;
    try {
        intValue = std::stoi(value);
    } catch (...) {
        LOG_ERROR("Invalid int value: " << value << ", defaulting to " << (min + (max - min) / 2));
        intValue = min + (max - min) / 2;
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##sliderInt", &intValue, min, max, "%d")) {
        value = std::to_string(intValue);
        hasChanges = true;
        LOG_DEBUG("Updated: " << section << "." << key << " = " << value);
    }
}

void renderMonitorCombo(const std::string& key, std::string& value, bool& hasChanges, const std::string& section) {
    int monitorIndex = 0;
    try {
        monitorIndex = std::stoi(value);
    } catch (...) {
        LOG_ERROR("Invalid monitor index: " << value << ", defaulting to 0");
    }
    const char* monitors[] = {"0", "1", "2"};
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##monitor", &monitorIndex, monitors, IM_ARRAYSIZE(monitors))) {
        value = std::to_string(monitorIndex);
        hasChanges = true;
        LOG_DEBUG("Updated: " << section << "." << key << " = " << value);
    }
}

void renderResolution(const std::string& key, std::string& value, bool& hasChanges, const std::string& section) {
    char buffer[16];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputText("##res", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsDecimal)) {
        value = std::string(buffer);
        hasChanges = true;
        LOG_DEBUG("Updated: " << section << "." << key << " = " << value);
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
            LOG_DEBUG("Selected resolution: " << section << "." << key << " = " << value);
        }
    }
}

void renderGenericText(const std::string& key, std::string& value, bool& hasChanges, const std::string& section) {
    char buffer[1024];
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
        value = std::string(buffer);
        LOG_DEBUG("Updated: " << section << "." << key << " = " << value);
        hasChanges = true;
    }
}

} // namespace UIElementRenderer