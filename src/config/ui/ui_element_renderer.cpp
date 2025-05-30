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
        LOG_INFO("UiElementRenderer::renderColorPicker: " << section << "." << key << " = " << value);
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
                LOG_INFO("UiElementRenderer:renderFontPath: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderPathOrExecutable: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderCheckbox: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderDpiScale: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderSliderInt: " << section << "." << key << " = " << value);
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
            LOG_DEBUG("UiElementRenderer::renderRotationSlider: " << section << "." << key << " = " << value);
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
    const char* options[] = {"vlc", "ffmpeg", "gstreamer", "novideo"};
    int videoBackend = 0; // Default to the first option (vlc)

    // Find the index of the current 'value' in the 'options' array
    for (int i = 0; i < IM_ARRAYSIZE(options); ++i) {
        if (value == options[i]) {
            videoBackend = i;
            break; // Found the match, no need to continue
        }
    }

    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##videoBackend", &videoBackend, options, IM_ARRAYSIZE(options))) {
        std::string oldValue = value;
        value = options[videoBackend];
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer::renderVideoBackendDropdown: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderTitleDropdown: " << section << "." << key << " = " << value);
        
        if (value == "metadata" && oldValue != "metadata") {
            LOG_DEBUG("UiElementRenderer: Checking for metadata index file");
            const auto& iniData = configService->getIniData();
            auto vpxIt = iniData.find("VPX");
            if (vpxIt != iniData.end()) {
                LOG_DEBUG("UiElementRenderer: VPX section found");
                auto pathIt = std::find_if(vpxIt->second.keyValues.begin(), vpxIt->second.keyValues.end(),
                                        [](const auto& pair) { return pair.first == "VPXTablesPath"; });
                if (pathIt != vpxIt->second.keyValues.end()) {
                    LOG_DEBUG("UiElementRenderer: VPXTablesPath found: " << pathIt->second);
                    if (!pathIt->second.empty()) {
                        auto internalIt = iniData.find("Internal");
                        if (internalIt != iniData.end()) {
                            auto indexIt = std::find_if(internalIt->second.keyValues.begin(), internalIt->second.keyValues.end(),
                                                        [](const auto& pair) { return pair.first == "vpxtoolIndex"; });
                            if (indexIt != internalIt->second.keyValues.end()) {
                                if (!indexIt->second.empty()) {
                                    std::filesystem::path jsonPath = std::filesystem::path(pathIt->second) / indexIt->second;
                                    LOG_DEBUG("UiElementRenderer: Checking path: " << jsonPath.string());
                                    if (!std::filesystem::exists(jsonPath)) {
                                        LOG_DEBUG("UiElementRenderer: " << indexIt->second << " not found, opening popup");
                                        ImGui::OpenPopup("Metadata Error");
                                    } else {
                                        LOG_DEBUG("UiElementRenderer: " << indexIt->second << " exists");
                                    }
                                } else {
                                    LOG_DEBUG("UiElementRenderer: vpxtoolIndex is empty");
                                }
                            } else {
                                LOG_DEBUG("UiElementRenderer: vpxtoolIndex not found in Internal section");
                            }
                        } else {
                            LOG_DEBUG("UiElementRenderer: Internal section not found in iniData");
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
        LOG_DEBUG("UiElementRenderer::renderMetadataCheckbox: " << section << "." << key << " = " << value);
        
        // Only perform the check and open popup if the checkbox is being set to true
        if (boolValue) { 
            LOG_DEBUG("UiElementRenderer: Checking for metadata index file");
            const auto& iniData = configService->getIniData();
            auto vpxIt = iniData.find("VPX");
            if (vpxIt != iniData.end()) {
                LOG_DEBUG("UiElementRenderer: VPX section found");
                auto pathIt = std::find_if(vpxIt->second.keyValues.begin(), vpxIt->second.keyValues.end(),
                                        [](const auto& pair) { return pair.first == "VPXTablesPath"; });
                if (pathIt != vpxIt->second.keyValues.end()) {
                    LOG_DEBUG("UiElementRenderer: VPXTablesPath found: " << pathIt->second);
                    if (!pathIt->second.empty()) {
                        auto internalIt = iniData.find("Internal");
                        if (internalIt != iniData.end()) {
                            auto indexIt = std::find_if(internalIt->second.keyValues.begin(), internalIt->second.keyValues.end(),
                                                        [](const auto& pair) { return pair.first == "vpxtoolIndex"; });
                            if (indexIt != internalIt->second.keyValues.end()) {
                                if (!indexIt->second.empty()) {
                                    std::filesystem::path jsonPath = std::filesystem::path(pathIt->second) / indexIt->second;
                                    LOG_DEBUG("UiElementRenderer: Checking path: " << jsonPath.string());
                                    if (!std::filesystem::exists(jsonPath)) {
                                        LOG_DEBUG("UiElementRenderer: " << indexIt->second << " not found, opening popup");
                                        ImGui::OpenPopup("Metadata Error");
                                    } else {
                                        LOG_DEBUG("UiElementRenderer: " << indexIt->second << " exists");
                                    }
                                } else {
                                    LOG_DEBUG("UiElementRenderer: vpxtoolIndex is empty");
                                }
                            } else {
                                LOG_DEBUG("UiElementRenderer: vpxtoolIndex not found in Internal section");
                            }
                        } else {
                            LOG_DEBUG("UiElementRenderer: Internal section not found in iniData");
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
        LOG_DEBUG("UiElementRenderer::renderResolution: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderGenericText: " << section << "." << key << " = " << value);
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
        LOG_DEBUG("UiElementRenderer::renderGenericTextShort: " << section << "." << key << " = " << value);
        hasChanges = true;
    }
}

void renderVolumeScale(const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, [[maybe_unused]] SettingsSection& sectionData) {
    using namespace ImGui;

    float volume = 50.0f;
    try {
        volume = std::stof(value);
    } catch (...) {
        LOG_ERROR("UiElementRenderer: Invalid volume value: " << value << ", defaulting to 50.0");
    }

    // --- VERTICAL SLIDER SIZE ---
    ImVec2 sliderSize(30, 150); // Width, Height. This makes it vertical. Adjust `150` for desired height.

    // Determine distinct color based on the key
    ImVec4 baseSliderColor;
    if (key.find("MediaAudio") != std::string::npos) {
        baseSliderColor = (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f);
    } else if (key.find("TableMusic") != std::string::npos) {
        baseSliderColor = (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f);
    } else if (key.find("InterfaceAudio") != std::string::npos) {
        baseSliderColor = (ImVec4)ImColor::HSV(0.6f, 0.6f, 0.6f);
    } else if (key.find("InterfaceAmbience") != std::string::npos) {
        baseSliderColor = (ImVec4)ImColor::HSV(0.9f, 0.6f, 0.6f);
    } else {
        baseSliderColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }

    // --- Check for mute state and prepare for unmuting / greying out ---
    bool isMuted = false;
    std::string prefix = key.substr(0, key.length() - 3); // Get "MediaAudio" from "MediaAudioVol"
    std::string muteKey = prefix + "Mute"; // Construct the corresponding mute key

    auto muteIt = std::find_if(sectionData.keyValues.begin(), sectionData.keyValues.end(),
                               [&muteKey](const auto& pair) { return pair.first == muteKey; });

    if (muteIt != sectionData.keyValues.end()) {
        isMuted = (muteIt->second == "true");
    } else {
        LOG_DEBUG("UIElementRenderer: Mute key '" << muteKey << "' not found for volume slider '" << key << "'.");
    }

    int pushedStyleVars = 0;
    if (isMuted) {
        PushStyleVar(ImGuiStyleVar_Alpha, GetStyle().Alpha * 0.5f);
        pushedStyleVars++;
    }

    // --- PUSHING 5 STYLE COLORS FOR THE SLIDER ---
    PushStyleColor(ImGuiCol_FrameBg, baseSliderColor);
    PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(baseSliderColor.x + 0.1f, baseSliderColor.y + 0.1f, baseSliderColor.z + 0.1f, baseSliderColor.w));
    PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(baseSliderColor.x + 0.2f, baseSliderColor.y + 0.2f, baseSliderColor.z + 0.2f, baseSliderColor.w));
    PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(baseSliderColor.x, 0.9f, 0.9f));
    PushStyleColor(ImGuiCol_SliderGrabActive, (ImVec4)ImColor::HSV(baseSliderColor.x, 1.0f, 1.0f));

    // Render the vertical slider
    if (VSliderFloat(("##volume_" + key).c_str(), sliderSize, &volume, 0.0f, 100.0f, "%.0f")) {
        value = std::to_string(volume);
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer::renderVolumeScale: " << section << "." << key << " = " << value);

        // Auto-unmute when slider is moved
        if (muteIt != sectionData.keyValues.end() && muteIt->second == "true") {
            muteIt->second = "false";
            hasChanges = true;
            LOG_DEBUG("UiElementRenderer: Auto-unmuted '" << muteKey << "' due to volume change.");
        }
    }

    PopStyleColor(5);

    if (pushedStyleVars > 0) {
        PopStyleVar(pushedStyleVars);
    }

    // Tooltip for the slider
    if (IsItemHovered() || IsItemActive()) {
        static const std::map<std::string, std::string> friendlyNames = {
            {"MediaAudioVol", "Videos Audio"},
            {"TableMusicVol", "Table Music"},
            {"InterfaceAudioVol", "UI Sounds"},
            {"InterfaceAmbienceVol", "Ambient Sound"}
        };

        auto it = friendlyNames.find(key);
        if (it != friendlyNames.end()) {
            SetTooltip("%s", it->second.c_str());
        } else {
            SetTooltip("%s", key.c_str());
        }
    }
}


void renderAudioMuteButton(const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section) {
    using namespace ImGui;

    bool isMuted = (value == "true");
    ImVec2 buttonSize(35, 25); // Adjust size for better visual

    // Determine color based on mute state (Red for Muted, Green for Unmuted)
    ImVec4 baseColor = isMuted ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    
    // --- PUSHING 3 STYLE COLORS FOR THE BUTTON ---
    PushStyleColor(ImGuiCol_Button, baseColor);
    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(baseColor.x + 0.1f, baseColor.y + 0.1f, baseColor.z + 0.1f, baseColor.w));
    PushStyleColor(ImGuiCol_ButtonActive, ImVec4(baseColor.x + 0.2f, baseColor.y + 0.2f, baseColor.z + 0.2f, baseColor.w));

    // Create a unique ID for the button to prevent clashes
    PushID(key.c_str());

    std::string buttonLabel = isMuted ? "MUTE" : "ON";
    if (Button(buttonLabel.c_str(), buttonSize)) {
        isMuted = !isMuted;
        value = isMuted ? "true" : "false";
        hasChanges = true;
        LOG_DEBUG("UiElementRenderer::renderAudioMuteButton: " << section << "." << key << " = " << value);
    }

    PopID(); // Pop the unique ID
    
    // --- POPPING 3 STYLE COLORS ---
    PopStyleColor(3); // This must match the number of PushStyleColor calls above.

    // // Optional: Add a tooltip
    // if (IsItemHovered()) {
    //     SetTooltip("%s", isMuted ? "Click to Unmute" : "Click to Mute");
    // }
}

void renderAudioSettingsMixer([[maybe_unused]] const std::string& key, [[maybe_unused]] std::string& value, bool& hasChanges, const std::string& section, SettingsSection& sectionData) {
    using namespace ImGui;

    std::vector<std::string> audioKeyPrefixes = {
        "Master",
        "MediaAudio",
        "TableMusic",
        "InterfaceAudio",
        "InterfaceAmbience"
    };

    static const std::map<std::string, std::string> channelDisplayNames = {
        {"Master", "Master"},
        {"MediaAudio", "Media"},
        {"TableMusic", "Music"},
        {"InterfaceAudio", "UI FX"},
        {"InterfaceAmbience", "Ambience"}
    };

    float sliderWidth = 30.0f;
    float buttonWidth = 35.0f;
    float desiredColumnContentWidth = 60.0f;
    float gapBetweenColumns = 30.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 5)); 

    // Calculate the total expected height of a fully rendered column
    float textLineHeight = ImGui::GetTextLineHeight();
    float itemSpacingY = ImGui::GetStyle().ItemSpacing.y; 
    float totalColumnHeight = textLineHeight + itemSpacingY + 150.0f + itemSpacingY + 25.0f;

    // Determine the target absolute screen Y for the BOTTOM of all columns
    float rowBottomScreenY = ImGui::GetCursorScreenPos().y + totalColumnHeight;

    // Render the columns
    for (size_t i = 0; i < audioKeyPrefixes.size(); ++i) {
        const std::string& prefix = audioKeyPrefixes[i];
        std::string volKey = prefix + "Vol";
        std::string muteKey = prefix + "Mute";

        ImGui::PushID(prefix.c_str());

        // --- Calculate target Y for this column's top, aiming for bottom alignment ---
        float currentColumnTargetTopY = rowBottomScreenY - totalColumnHeight;
        
        // --- HACK: Adjust Y for the first column if it's consistently too high ---
        if (i == 0) {
            currentColumnTargetTopY += 5.0f; // Add 5 pixels to push the first column down
        }

        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, currentColumnTargetTopY));
        
        ImGui::BeginGroup(); 
        float currentGroupStartX = ImGui::GetCursorPosX(); 

        // --- 1. Render Label (centered within its column) ---
        std::string displayName = channelDisplayNames.at(prefix); 
        float textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
        float textOffsetX = (desiredColumnContentWidth - textWidth) / 2.0f;
        
        ImGui::SetCursorPosX(currentGroupStartX + textOffsetX);
        ImGui::TextUnformatted(displayName.c_str());
        ImGui::NewLine(); 

        // --- 2. Render Slider (centered within its column) ---
        float sliderOffsetX = (desiredColumnContentWidth - sliderWidth) / 2.0f;
        ImGui::SetCursorPosX(currentGroupStartX + sliderOffsetX);
        
        auto volIt = std::find_if(sectionData.keyValues.begin(), sectionData.keyValues.end(),
                                  [&volKey](const auto& pair) { return pair.first == volKey; });
        if (volIt != sectionData.keyValues.end()) {
            renderVolumeScale(volKey, volIt->second, hasChanges, section, sectionData);
        } else {
            LOG_ERROR("UIElementRenderer::renderAudioSettingsMixer: Missing volume key for prefix: " << prefix << " (Key: " << volKey << ")");
            ImGui::Dummy(ImVec2(sliderWidth, 150));
        }
        ImGui::NewLine(); 

        // --- 3. Render Mute Button (centered within its column) ---
        float buttonOffsetX = (desiredColumnContentWidth - buttonWidth) / 2.0f;
        ImGui::SetCursorPosX(currentGroupStartX + buttonOffsetX);

        auto muteIt = std::find_if(sectionData.keyValues.begin(), sectionData.keyValues.end(),
                                   [&muteKey](const auto& pair) { return pair.first == muteKey; });
        if (muteIt != sectionData.keyValues.end()) {
            renderAudioMuteButton(muteKey, muteIt->second, hasChanges, section);
        } else {
            LOG_ERROR("UIElementRenderer::renderAudioSettingsMixer: Missing mute key for prefix: " << prefix << " (Key: " << muteKey << ")");
            ImGui::Dummy(ImVec2(buttonWidth, 25));
        }

        ImGui::EndGroup(); 

        if (i < audioKeyPrefixes.size() - 1) {
            ImGui::SameLine(0.0f, gapBetweenColumns);
        }
        ImGui::PopID(); 
    }

    ImGui::PopStyleVar(); 
    ImGui::NewLine();     
}

} // namespace UIElementRenderer