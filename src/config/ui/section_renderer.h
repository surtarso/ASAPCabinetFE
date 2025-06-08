#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include <json.hpp>
#include <string>
#include <vector>
#include <imgui.h>
#include "ImGuiFileDialog.h"
#include "utils/logging.h"

/**
 * @class ISectionRenderer
 * @brief Interface for rendering configuration sections in the UI.
 */
class ISectionRenderer {
public:
    virtual ~ISectionRenderer() = default;
    virtual void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, bool defaultOpen = false, bool& isDialogOpen = *(new bool(false)), std::string& dialogKey = *(new std::string())) = 0;
};

/**
 * @class BaseSectionRenderer
 * @brief Base class providing common rendering utilities for configuration sections.
 */
class BaseSectionRenderer : public ISectionRenderer {
protected:
    void renderBool(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        bool val = value.get<bool>();
        if (ImGui::Checkbox(key.c_str(), &val)) {
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    void renderFloat(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                     float minVal = 0.0f, float maxVal = 1.0f, const char* format = "%.2f") {
        float val = value.get<float>();
        if (ImGui::SliderFloat(key.c_str(), &val, minVal, maxVal, format)) {
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    void renderInt(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                   int minVal = 0, int maxVal = 10000) {
        int val = value.get<int>();
        if (ImGui::InputInt(key.c_str(), &val)) {
            val = std::clamp(val, minVal, maxVal);
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    void renderString(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        std::string val = value.get<std::string>();
        char buffer[256];
        strncpy(buffer, val.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
            value = std::string(buffer);
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << buffer);
        }
    }

    void renderColor(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        float color[4] = {
            value[0].get<float>() / 255.0f,
            value[1].get<float>() / 255.0f,
            value[2].get<float>() / 255.0f,
            value[3].get<float>() / 255.0f
        };
        if (ImGui::ColorEdit4(key.c_str(), color)) {
            value = nlohmann::json{
                static_cast<int>(color[0] * 255.0f),
                static_cast<int>(color[1] * 255.0f),
                static_cast<int>(color[2] * 255.0f),
                static_cast<int>(color[3] * 255.0f)
            };
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key
                      << " to [" << value[0] << "," << value[1] << "," << value[2] << "," << value[3] << "]");
        }
    }

    void renderRotation(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        int val = value.get<int>();
        static std::unordered_map<std::string, int> lastLoggedValues;
        int currentValue = snapToStep(val);
        LOG_DEBUG("BaseSectionRenderer: Rendering " << key << " with currentValue " << currentValue);
        if (ImGui::SliderInt(key.c_str(), &currentValue, 0, 360, "%d°")) {
            int snappedValue = snapToStep(currentValue);
            if (snappedValue != lastLoggedValues[key]) {
                value = snappedValue;
                lastLoggedValues[key] = snappedValue;
                LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << snappedValue << "°");
            }
        }
    }

    void renderKeybind(const std::string& key, nlohmann::json& value, const std::string& sectionName, bool& isCapturing, std::string& capturingKeyName) {
        if (!value.is_string()) {
            LOG_DEBUG("BaseSectionRenderer: Invalid type for keybind " << key << ", expected string, got " << value.type_name());
            return;
        }

        std::string currentBind = value.get<std::string>();
        std::string buttonLabel = "[" + key + ": " + (currentBind.empty() ? "Unbound" : currentBind) + "]";
        if (ImGui::Button(buttonLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            if (!isCapturing) {
                isCapturing = true;
                capturingKeyName = key;
                LOG_DEBUG("BaseSectionRenderer: Started capturing key for " << sectionName << "." << key);
            }
        }
        if (isCapturing && capturingKeyName == key) {
            ImGui::Text("Press a key or joystick input to bind... (Esc to cancel)");
        }
    }

    void renderPathOrExecutable(const std::string& key, nlohmann::json& value, const std::string& sectionName, bool& isDialogOpen, std::string& dialogKey) {
        std::string val = value.get<std::string>();
        char buffer[1024];
        strncpy(buffer, val.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
        if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
            value = std::string(buffer);
            LOG_DEBUG("BaseSectionRenderer::renderPathOrExecutable: " << sectionName << "." << key << " = " << value.get<std::string>());
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(50, 0))) {
            LOG_DEBUG("BaseSectionRenderer: Browse button clicked for " << key);
            IGFD::FileDialogConfig config;
            config.path = (!val.empty() && std::filesystem::exists(val)) ? val : std::string(getenv("HOME"));
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir, nullptr, ImVec4(0.5f, 1.0f, 0.9f, 0.9f));

            if (key == "VPXTablesPath") {
                ImGuiFileDialog::Instance()->OpenDialog("FolderDlg_VPXTablesPath", "Select VPX Tables Folder", nullptr, config);
            } else if (key == "VPinballXPath") {
                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByFullName, "((VPinballX))", ImVec4(0.0f, 1.0f, 0.0f, 0.9f));
                ImGuiFileDialog::Instance()->OpenDialog("FileDlg_VPinballXPath", "Select VPinballX Executable", "((VPinballX))", config);
            } else if (key == "vpxIniPath") {
                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".ini", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
                ImGuiFileDialog::Instance()->OpenDialog("FileDlg_vpxIniPath", "Select VPinballX Config File", ".ini", config);
            }
            isDialogOpen = true;
            dialogKey = key;
            LOG_DEBUG("Dialog opened with key: " << dialogKey << ", isDialogOpen: " << isDialogOpen);
        }
    }

    int snapToStep(int value) {
        const int steps[] = {0, 90, 180, 270, 360};
        int nearestStep = 0;
        int minDiff = abs(value - steps[0]);
        for (int i = 1; i < 5; ++i) {
            int diff = abs(value - steps[i]);
            if (diff < minDiff) {
                minDiff = diff;
                nearestStep = steps[i];
            }
        }
        return nearestStep;
    }
};

#endif // SECTION_RENDERER_H