/**
 * @file isection_renderer.h
 * @brief Defines the ISectionRenderer interface and BaseSectionRenderer for rendering configuration sections in ASAPCabinetFE.
 *
 * This header provides the ISectionRenderer interface and the BaseSectionRenderer base class,
 * which specify methods and utilities for rendering configuration sections in the UI using ImGui.
 * It is implemented by classes like SectionRenderer to handle section-specific rendering logic.
 */

#ifndef ISECTION_RENDERER_H
#define ISECTION_RENDERER_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <imgui.h>
#include <filesystem>
#include "ImGuiFileDialog.h"
#include "log/logging.h"

class ISectionRenderer {
public:
    virtual ~ISectionRenderer() = default;

    virtual void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing,
                        std::string& capturingKeyName, ImGuiFileDialog* fileDialog, bool defaultOpen = false,
                        bool& isDialogOpen = *(new bool(false)), std::string& dialogKey = *(new std::string())) = 0;
};

class BaseSectionRenderer : public ISectionRenderer {
protected:
    void renderBool(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        bool val = value.get<bool>();
        if (ImGui::Checkbox(key.c_str(), &val)) {
            value = val;
            LOG_INFO("Updated " + sectionName + "." + key + " to " + std::to_string(val));
        }
    }

    void renderFloat(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                     float minVal = 0.0f, float maxVal = 1.0f, const char* format = "%.2f") {
        float val = value.get<float>();
        if (key == "titleWeight") {
            minVal = 0.2f; maxVal = 0.8f;
        } else if (key == "yearWeight") {
            minVal = 0.0f; maxVal = 0.4f;
        } else if (key == "manufacturerWeight") {
            minVal = 0.0f; maxVal = 0.3f;
        } else if (key == "romWeight") {
            minVal = 0.0f; maxVal = 0.5f;
        } else if (key == "titleThreshold") {
            minVal = 0.3f; maxVal = 0.8f;
        } else if (key == "confidenceThreshold") {
            minVal = 0.4f; maxVal = 0.9f;
        } else if (key == "DPI Scale") {
            minVal = 0.5f; maxVal = 3.0f;
        } else if (key.find("Alpha") != std::string::npos || key == "scrollbarLength" ||
                   key == "metadataPanelWidth" || key == "metadataPanelHeight") {
            minVal = 0.0f; maxVal = 1.0f;
        } else if (key == "arrowHintWidth" || key == "arrowHintHeight") {
            minVal = 0.0f; maxVal = 200.0f;
        } else if (key == "arrowThickness" || key == "arrowGlow") {
            minVal = 0.0f; maxVal = 10.0f;
        } else if (key == "scrollbarWidth" || key == "thumbWidth") {
            minVal = 0.0f; maxVal = 50.0f;
        } else if (key == "masterVol" || key == "mediaAudioVol" || key == "tableMusicVol" ||
                   key == "interfaceAudioVol" || key == "interfaceAmbienceVol") {
            minVal = 0.0f; maxVal = 100.0f;
        } else if (key == "configUIWidth" || key == "configUIHeight") {
            minVal = 0.1f; maxVal = 1.0f;
        }
        if (ImGui::SliderFloat(key.c_str(), &val, minVal, maxVal, format)) {
            value = val;
            LOG_INFO("Updated " + sectionName + "." + key + " to " + std::to_string(val));
        }
    }

    void renderInt(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                   int minVal = 0, int maxVal = 10000) {
        int val = value.get<int>();
        if (key.find("WindowWidth") != std::string::npos || key.find("WindowHeight") != std::string::npos ||
            key.find("MediaWidth") != std::string::npos || key.find("MediaHeight") != std::string::npos) {
            minVal = 0; maxVal = 3840;
        } else if (key == "fontSize") {
            minVal = 10; maxVal = 60;
        } else if (key == "screenshotWait") {
            minVal = 0; maxVal = 60;
        }
        if (ImGui::InputInt(key.c_str(), &val)) {
            val = std::clamp(val, minVal, maxVal);
            value = val;
            LOG_INFO("Updated " + sectionName + "." + key + " to " + std::to_string(val));
        }
    }

    void renderString(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        std::string val = value.get<std::string>();
        char buffer[256];
        strncpy(buffer, val.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
            value = std::string(buffer);
            LOG_INFO("Updated " + sectionName + "." + key + " to " + buffer);
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
            LOG_INFO("Updated " + sectionName + "." + key + " to [" +
                     std::to_string(value[0].get<int>()) + "," +
                     std::to_string(value[1].get<int>()) + "," +
                     std::to_string(value[2].get<int>()) + "," +
                     std::to_string(value[3].get<int>()) + "]");
        }
    }

    void renderRotation(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        int val = value.get<int>();
        static std::unordered_map<std::string, int> lastLoggedValues;
        int currentValue = snapToStep(val);
        if (ImGui::SliderInt(key.c_str(), &currentValue, 0, 360, "%d°")) {
            int snappedValue = snapToStep(currentValue);
            if (snappedValue != lastLoggedValues[key]) {
                value = snappedValue;
                lastLoggedValues[key] = snappedValue;
                LOG_INFO("Updated " + sectionName + "." + key + " to " + std::to_string(snappedValue) + "°");
            }
        }
    }

    void renderKeybind(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                       bool& isCapturing, std::string& capturingKeyName) {
        if (!value.is_string()) {
            LOG_DEBUG("Invalid type for keybind " + key + ", expected string, got " + value.type_name());
            return;
        }
        std::string currentBind = value.get<std::string>();
        std::string buttonLabel = "[" + key + ": " + (currentBind.empty() ? "Unbound" : currentBind) + "]";
        if (ImGui::Button(buttonLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            if (!isCapturing) {
                isCapturing = true;
                capturingKeyName = key;
                LOG_DEBUG("Started capturing key for " + sectionName + "." + key);
            }
        }
        if (isCapturing && capturingKeyName == key) {
            ImGui::Text("Press a key or joystick input to bind... (Esc to cancel)");
        }
    }

    void renderPathOrExecutable(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                                ImGuiFileDialog* fileDialog, bool& isDialogOpen, std::string& dialogKey) {
        std::string val = value.get<std::string>();
        char buffer[1024];
        strncpy(buffer, val.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
        if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
            value = std::string(buffer);
            LOG_INFO("Updated " + sectionName + "." + key + " to " + buffer);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(50, 0))) {
            LOG_DEBUG("Browse button clicked for " + key);
            IGFD::FileDialogConfig config;
            config.path = (!val.empty() && std::filesystem::exists(val)) ? std::filesystem::path(val).parent_path().string() : std::string(getenv("HOME"));
            config.flags = ImGuiFileDialogFlags_Modal;
            fileDialog->SetFileStyle(IGFD_FileStyleByTypeDir, nullptr, ImVec4(0.5f, 1.0f, 0.9f, 0.9f));

            if (key == "VPXTablesPath") {
                fileDialog->OpenDialog("FolderDlg_VPXTablesPath", "Select VPX Tables Folder", nullptr, config);
            } else if (key == "VPinballXPath") {
                fileDialog->SetFileStyle(IGFD_FileStyleByFullName, "VPinballX*", ImVec4(0.0f, 1.0f, 0.0f, 0.9f));
                fileDialog->OpenDialog("FileDlg_VPinballXPath", "Select VPinballX Executable", "VPinballX*", config);
            } else if (key == "vpxIniPath") {
                fileDialog->SetFileStyle(IGFD_FileStyleByExtention, ".ini", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
                fileDialog->OpenDialog("FileDlg_vpxIniPath", "Select VPinballX Config File", ".ini", config);
            }
            isDialogOpen = true;
            dialogKey = key;
            LOG_DEBUG("Dialog opened with key: " + dialogKey + ", isDialogOpen: " + std::to_string(isDialogOpen));
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

#endif // ISECTION_RENDERER_H