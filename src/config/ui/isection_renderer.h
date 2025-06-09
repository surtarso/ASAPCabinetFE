/**
 * @file isection_renderer.h
 * @brief Defines the ISectionRenderer interface for rendering configuration sections in ASAPCabinetFE.
 *
 * This header provides the ISectionRenderer interface and the BaseSectionRenderer base class,
 * which specify methods and utilities for rendering configuration sections in the UI using ImGui.
 * It is implemented by classes like SectionRenderer to handle section-specific rendering logic.
 */

#ifndef ISECTION_RENDERER_H
#define ISECTION_RENDERER_H

#include <json.hpp>
#include <string>
#include <vector>
#include <imgui.h>
#include "ImGuiFileDialog.h"
#include "utils/logging.h"

/**
 * @class ISectionRenderer
 * @brief Interface for rendering configuration sections in the UI.
 *
 * This pure virtual class defines a method for rendering configuration sections, including
 * handling JSON data, capturing keybindings, and managing file dialogs. Implementers provide
 * specific rendering logic for different configuration sections in ASAPCabinetFE.
 */
class ISectionRenderer {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     *
     * Ensures proper cleanup of any derived classes implementing this interface.
     */
    virtual ~ISectionRenderer() = default;

    /**
     * @brief Renders a configuration section in the UI.
     *
     * Renders the specified section with its JSON data, handling key capture, file dialogs,
     * and optional default open state or dialog management.
     *
     * @param sectionName The name of the configuration section to render.
     * @param sectionData Reference to the JSON data for the section.
     * @param isCapturing Reference to a flag indicating if a key is being captured.
     * @param capturingKeyName Reference to the name of the key being captured.
     * @param fileDialog Pointer to the ImGuiFileDialog instance for file selection.
     * @param defaultOpen Optional flag to open the section by default (default: false).
     * @param isDialogOpen Reference to a flag indicating if a file dialog is open (default: new bool(false)).
     * @param dialogKey Reference to the key associated with the open dialog (default: new std::string()).
     */
    virtual void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, ImGuiFileDialog* fileDialog, bool defaultOpen = false, bool& isDialogOpen = *(new bool(false)), std::string& dialogKey = *(new std::string())) = 0;
};

/**
 * @class BaseSectionRenderer
 * @brief Base class providing common rendering utilities for configuration sections.
 *
 * This class extends ISectionRenderer and provides protected methods for rendering
 * common data types (bool, float, int, string, color, rotation, keybind, path) using ImGui.
 * Derived classes can reuse these utilities for section-specific rendering.
 */
class BaseSectionRenderer : public ISectionRenderer {
protected:
    /**
     * @brief Renders a boolean value as a checkbox.
     *
     * Updates the JSON value if the checkbox state changes.
     *
     * @param key The key of the boolean value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     */
    void renderBool(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        bool val = value.get<bool>();
        if (ImGui::Checkbox(key.c_str(), &val)) {
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    /**
     * @brief Renders a float value as a slider with custom ranges.
     *
     * Updates the JSON value if the slider is adjusted, with ranges tailored to specific keys.
     *
     * @param key The key of the float value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     * @param minVal Minimum value for the slider (default: 0.0f).
     * @param maxVal Maximum value for the slider (default: 1.0f).
     * @param format Format string for the slider display (default: "%.2f").
     */
    void renderFloat(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                     float minVal = 0.0f, float maxVal = 1.0f, const char* format = "%.2f") {
        float val = value.get<float>();
        // Custom ranges for specific float fields
        if (key == "DPI Scale") {
            minVal = 0.5f;
            maxVal = 3.0f;
        } else if (key.find("Alpha") != std::string::npos || key == "scrollbarLength" ||
                   key == "metadataPanelWidth" || key == "metadataPanelHeight") {
            minVal = 0.0f;
            maxVal = 1.0f;
        } else if (key == "arrowHintWidth" || key == "arrowHintHeight") {
            minVal = 0.0f;
            maxVal = 200.0f;
        } else if (key == "arrowThickness" || key == "arrowGlow") {
            minVal = 0.0f;
            maxVal = 10.0f;
        } else if (key == "scrollbarWidth" || key == "thumbWidth") {
            minVal = 0.0f;
            maxVal = 50.0f;
        } else if (key == "masterVol" || key == "mediaAudioVol" || key == "tableMusicVol" ||
                   key == "interfaceAudioVol" || key == "interfaceAmbienceVol") {
            minVal = 0.0f;
            maxVal = 100.0f;
        } else if (key == "configUIWidth" || key == "configUIHeight") {
            minVal = 0.1f;
            maxVal = 1.0f;
        }
        if (ImGui::SliderFloat(key.c_str(), &val, minVal, maxVal, format)) {
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    void renderInt(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                   int minVal = 0, int maxVal = 10000) {
        int val = value.get<int>();
        // Custom ranges for specific integer fields
        if (key.find("WindowWidth") != std::string::npos || key.find("WindowHeight") != std::string::npos ||
            key.find("MediaWidth") != std::string::npos || key.find("MediaHeight") != std::string::npos) {
            minVal = 0;
            maxVal = 3840; // Suitable for up to 4K resolutions
        } else if (key == "fontSize") {
            minVal = 10;
            maxVal = 60;
        } else if (key == "screenshotWait") {
            minVal = 0;
            maxVal = 60;
        }
        if (ImGui::InputInt(key.c_str(), &val)) {
            val = std::clamp(val, minVal, maxVal);
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    /**
     * @brief Renders an integer value as an input field with custom ranges.
     *
     * Updates the JSON value if the input changes, clamping to the specified range.
     *
     * @param key The key of the integer value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     * @param minVal Minimum value for the input (default: 0).
     * @param maxVal Maximum value for the input (default: 10000).
     */
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

    /**
     * @brief Renders a color value as a color editor.
     *
     * Updates the JSON array [R,G,B,A] if the color is edited.
     *
     * @param key The key of the color value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     */
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

    /**
     * @brief Renders a rotation value as a slider with 90-degree steps.
     *
     * Updates the JSON value if the slider is adjusted, snapping to 0°, 90°, 180°, 270°, or 360°.
     *
     * @param key The key of the rotation value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     */
    void renderRotation(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        int val = value.get<int>();
        static std::unordered_map<std::string, int> lastLoggedValues;
        int currentValue = snapToStep(val);
        //LOG_DEBUG("BaseSectionRenderer: Rendering " << key << " with currentValue " << currentValue);
        if (ImGui::SliderInt(key.c_str(), &currentValue, 0, 360, "%d°")) {
            int snappedValue = snapToStep(currentValue);
            if (snappedValue != lastLoggedValues[key]) {
                value = snappedValue;
                lastLoggedValues[key] = snappedValue;
                LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << snappedValue << "°");
            }
        }
    }

    /**
     * @brief Renders a keybind value with a capture button.
     *
     * Initiates key capture if the button is clicked and the capture flag is not set.
     *
     * @param key The key of the keybind value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     * @param isCapturing Reference to a flag indicating if a key is being captured.
     * @param capturingKeyName Reference to the name of the key being captured.
     */
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

    /**
     * @brief Renders a path or executable value with a browse button.
     *
     * Opens a file dialog for selecting paths or executables based on the key.
     *
     * @param key The key of the path value in the JSON.
     * @param value Reference to the JSON value to render and update.
     * @param sectionName The name of the section for logging.
     * @param fileDialog Pointer to the ImGuiFileDialog instance.
     * @param isDialogOpen Reference to a flag indicating if a file dialog is open.
     * @param dialogKey Reference to the key associated with the open dialog.
     */
    void renderPathOrExecutable(const std::string& key, nlohmann::json& value, const std::string& sectionName, ImGuiFileDialog* fileDialog, bool& isDialogOpen, std::string& dialogKey) {
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
            fileDialog->SetFileStyle(IGFD_FileStyleByTypeDir, nullptr, ImVec4(0.5f, 1.0f, 0.9f, 0.9f));

            if (key == "VPXTablesPath") {
                fileDialog->OpenDialog("FolderDlg_VPXTablesPath", "Select VPX Tables Folder", nullptr, config);
            } else if (key == "VPinballXPath") {
                fileDialog->SetFileStyle(IGFD_FileStyleByFullName, "((VPinballX))", ImVec4(0.0f, 1.0f, 0.0f, 0.9f));
                fileDialog->OpenDialog("FileDlg_VPinballXPath", "Select VPinballX Executable", "((VPinballX))", config);
            } else if (key == "vpxIniPath") {
                fileDialog->SetFileStyle(IGFD_FileStyleByExtention, ".ini", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
                fileDialog->OpenDialog("FileDlg_vpxIniPath", "Select VPinballX Config File", ".ini", config);
            }
            isDialogOpen = true;
            dialogKey = key;
            LOG_DEBUG("Dialog opened with key: " << dialogKey << ", isDialogOpen: " << isDialogOpen);
        }
    }

    /**
     * @brief Snaps a value to the nearest 90-degree step.
     *
     * Returns the nearest value from {0, 90, 180, 270, 360}.
     *
     * @param value The value to snap.
     * @return The snapped value.
     */
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