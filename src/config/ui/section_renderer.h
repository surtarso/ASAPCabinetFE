#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include <json.hpp>
#include <string>
#include <vector>
#include <imgui.h>
#include "utils/logging.h"

/**
 * @class ISectionRenderer
 * @brief Interface for rendering configuration sections in the UI.
 */
class ISectionRenderer {
public:
    virtual ~ISectionRenderer() = default;
    virtual void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, bool defaultOpen = false) = 0;
};

/**
 * @class BaseSectionRenderer
 * @brief Base class providing common rendering utilities for configuration sections.
 */
class BaseSectionRenderer : public ISectionRenderer {
protected:
    // Render a boolean field
    void renderBool(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        bool val = value.get<bool>();
        if (ImGui::Checkbox(key.c_str(), &val)) {
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    // Render a float field with a slider
    void renderFloat(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                     float minVal = 0.0f, float maxVal = 1.0f, const char* format = "%.2f") {
        float val = value.get<float>();
        if (ImGui::SliderFloat(key.c_str(), &val, minVal, maxVal, format)) {
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    // Render an integer field
    void renderInt(const std::string& key, nlohmann::json& value, const std::string& sectionName,
                   int minVal = 0, int maxVal = 10000) {
        int val = value.get<int>();
        if (ImGui::InputInt(key.c_str(), &val)) {
            val = std::clamp(val, minVal, maxVal);
            value = val;
            LOG_DEBUG("BaseSectionRenderer: Updated " << sectionName << "." << key << " to " << val);
        }
    }

    // Render a string field
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

    // Render a color field (RGBA array)
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

    // Render a rotation field with snapping
    void renderRotation(const std::string& key, nlohmann::json& value, const std::string& sectionName) {
        int val = value.get<int>();
        static std::unordered_map<std::string, int> lastLoggedValues;
        int currentValue = snapToStep(val); // Initialize with snapped value
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
                capturingKeyName = key; // Use the raw key here
                LOG_DEBUG("BaseSectionRenderer: Started capturing key for " << sectionName << "." << key);
            }
        }
        if (isCapturing && capturingKeyName == key) {
            ImGui::Text("Press a key or joystick input to bind... (Esc to cancel)");
        }
    }

    // Helper function to snap value to nearest 90-degree step
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