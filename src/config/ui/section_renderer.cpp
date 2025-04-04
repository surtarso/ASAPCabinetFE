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

// **Constructor**: Initializes the SectionRenderer, scans for available fonts in the system font directory
SectionRenderer::SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler)
    : configService_(configService), currentSection_(currentSection), inputHandler_(inputHandler) {
    std::string fontDir = "/usr/share/fonts/"; // Default font directory for Linux systems
    if (std::filesystem::exists(fontDir)) {
        // Recursively scan for .ttf font files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(fontDir)) {
            if (entry.path().extension() == ".ttf") {
                availableFonts_.push_back(entry.path().string()); // Store full paths of found fonts
            }
        }
        std::sort(availableFonts_.begin(), availableFonts_.end()); // Sort alphabetically for consistent UI display
        LOG_DEBUG("Found " << availableFonts_.size() << " .ttf fonts in " << fontDir);
    } else {
        LOG_ERROR("Font directory " << fontDir << " not found! Font selection will be limited.");
    }
}

// **Render Sections Pane**: Displays a selectable list of configuration sections on the left side
void SectionRenderer::renderSectionsPane(const std::vector<std::string>& sectionOrder) {
    ImGui::BeginChild("SectionsPane", ImVec2(250, 0), false); // Fixed width child window for sections
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8)); // Consistent spacing between items
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Configuration Sections"); // Light blue header
    ImGui::Separator(); // Visual divider

    // Loop through provided section order and render each as a selectable item
    for (const auto& section : sectionOrder) {
        if (configService_->getIniData().count(section)) { // Ensure section exists in config data
            bool selected = (currentSection_ == section); // Highlight current section
            if (ImGui::Selectable(section.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                currentSection_ = section; // Update current section on selection
                LOG_DEBUG("Selected section: " << section);
            }
            if (selected) ImGui::SetItemDefaultFocus(); // Keep focus on selected item
        }
    }
    ImGui::PopStyleVar(); // Restore default spacing
    ImGui::EndChild(); // End sections pane
}

// **Render Key-Values Pane**: Displays and edits key-value pairs for the selected section with custom UI elements
void SectionRenderer::renderKeyValuesPane(std::map<std::string, SettingsSection>& iniData) {
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, 0), false); // Full-width child window for key-values
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8)); // Consistent item spacing
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Settings - %s", currentSection_.c_str()); // Section title
    ImGui::Separator(); // Divider line

    if (!currentSection_.empty() && iniData.count(currentSection_)) { // Check if section is valid
        auto& section = iniData[currentSection_];
        float maxKeyWidth = 150.0f; // Fixed width for key labels to align UI elements

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 maxSize = ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f); // Max size for file dialogs
        ImVec2 minSize = ImVec2(600, 400); // Min size for file dialogs

        // Create a modifiable copy of key-value pairs for real-time editing
        std::vector<std::pair<std::string, std::string>> keyValuesCopy = section.keyValues;
        for (auto& [key, value] : keyValuesCopy) {
            std::string keyCopy = key; // Local copy of key for unique ID generation
            ImGui::PushID(keyCopy.c_str()); // Ensure unique IDs for ImGui widgets per key
            ImGui::AlignTextToFramePadding(); // Align text with input fields
            ImGui::Text("%s:", keyCopy.c_str()); // Display key name
            ImGui::SameLine(maxKeyWidth); // Align subsequent widgets

            // **Tooltip Button**: Small "?" button for hover tooltips
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f)); // Green text
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent button
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // No hover effect
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); // Minimal padding
            if (ImGui::Button("?", ImVec2(16, 0))) {} // Dummy button for tooltip trigger
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            renderTooltip(keyCopy); // Render tooltip if available
            ImGui::SameLine(); // Keep next widget on same line

            // **Custom UI Elements Based on Section and Key**
            if (currentSection_ == "Keybinds") {
                // Display keybind value and "Set" button for reassignment
                std::string label = value.empty() ? "None" : value;
                ImGui::Text("%s", label.c_str());
                ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f); // Position button at right
                if (ImGui::Button("Set", ImVec2(60, 0))) {
                    inputHandler_.startCapturing(keyCopy); // Start capturing new keybind
                }
            } else if (keyCopy == "FontColor" || keyCopy == "FontBgColor") {
                // Color picker for font foreground/background colors
                std::vector<int> rgba(4, 0); // Default to black with full opacity
                std::stringstream ss(value);
                std::string token;
                for (int i = 0; i < 4 && std::getline(ss, token, ','); ++i) {
                    rgba[i] = std::stoi(token); // Parse RGBA values
                }
                float color[4] = {rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f};
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
                if (ImGui::ColorButton("##color", ImVec4(color[0], color[1], color[2], color[3]), ImGuiColorEditFlags_AlphaPreview, ImVec2(20, 20))) {
                    ImGui::OpenPopup("ColorPicker"); // Open color picker on click
                }
                if (ImGui::BeginPopup("ColorPicker")) {
                    ImGui::ColorPicker4("##picker", color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
                    value = std::to_string(int(color[0] * 255)) + "," +
                            std::to_string(int(color[1] * 255)) + "," +
                            std::to_string(int(color[2] * 255)) + "," +
                            std::to_string(int(color[3] * 255)); // Update value as comma-separated RGBA
                    hasChanges_ = true;
                    LOG_INFO("Color updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();
            } else if (keyCopy == "FontPath") {
                // Combo box for selecting font files
                ImGui::SetNextItemWidth(-1); // Full width
                std::string displayValue = value.empty() ? "None" : std::filesystem::path(value).filename().string();
                static std::string preview = displayValue; // Retain preview across frames
                if (ImGui::BeginCombo("##fontCombo", preview.c_str())) {
                    for (size_t i = 0; i < availableFonts_.size(); ++i) {
                        std::string fontName = std::filesystem::path(availableFonts_[i]).filename().string();
                        bool isSelected = (availableFonts_[i] == value);
                        if (ImGui::Selectable(fontName.c_str(), isSelected)) {
                            value = availableFonts_[i]; // Set full path as value
                            preview = fontName; // Update preview to filename
                            LOG_INFO("Font selected: " << currentSection_ << "." << keyCopy << " = " << value);
                            hasChanges_ = true;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            } else if (keyCopy.find("Path") != std::string::npos || keyCopy.find("ExecutableCmd") != std::string::npos) {
                // Text input with "Browse" button for paths and executables
                char buffer[1024];
                strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::SetNextItemWidth(-60); // Leave space for button
                if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                    value = std::string(buffer);
                    LOG_DEBUG("Text updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    hasChanges_ = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Browse", ImVec2(50, 0))) {
                    IGFD::FileDialogConfig config;
                    config.path = (!value.empty() && std::filesystem::exists(value)) ? value : std::string(getenv("HOME"));
                    config.flags = ImGuiFileDialogFlags_Modal;
                    if (keyCopy.find("Path") != std::string::npos) {
                        ImGuiFileDialog::Instance()->OpenDialog("FolderDlg_" + keyCopy, "Select Folder", nullptr, config);
                    } else if (keyCopy.find("ExecutableCmd") != std::string::npos) {
                        ImGuiFileDialog::Instance()->OpenDialog("FileDlg_" + keyCopy, "Select Executable", "((.*))", config);
                    }
                }
                // Folder picker dialog
                if (keyCopy.find("Path") != std::string::npos &&
                    ImGuiFileDialog::Instance()->Display("FolderDlg_" + keyCopy, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = ImGuiFileDialog::Instance()->GetCurrentPath();
                        LOG_INFO("Folder picked: " << currentSection_ << "." << keyCopy << " = " << value);
                        hasChanges_ = true;
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
                // File picker dialog for executables
                if (keyCopy.find("ExecutableCmd") != std::string::npos &&
                    ImGuiFileDialog::Instance()->Display("FileDlg_" + keyCopy, ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        value = ImGuiFileDialog::Instance()->GetFilePathName();
                        LOG_INFO("Executable picked: " << currentSection_ << "." << keyCopy << " = " << value);
                        hasChanges_ = true;
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            } else if (currentSection_ == "DPISettings") {
                if (keyCopy == "EnableDpiScaling") {
                    // Checkbox to enable/disable DPI scaling
                    bool enableDpi = (value == "true");
                    if (ImGui::Checkbox("##enableDpi", &enableDpi)) {
                        value = enableDpi ? "true" : "false";
                        hasChanges_ = true;
                        LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    }
                } else if (keyCopy == "DpiScale") {
                    // Slider for DPI scale, grayed out if scaling is disabled
                    float dpiScale = 1.0f;
                    try {
                        dpiScale = std::stof(value);
                    } catch (...) {
                        LOG_ERROR("Invalid DpiScale value: " << value << ", defaulting to 1.0");
                        dpiScale = 1.0f;
                    }
                    bool isEnabled = false;
                    for (const auto& kv : section.keyValues) {
                        if (kv.first == "EnableDpiScaling") {
                            isEnabled = (kv.second == "true");
                            break;
                        }
                    }
                    if (!isEnabled) {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // Gray out if disabled
                    }
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::SliderFloat("##dpiScale", &dpiScale, 0.5f, 3.0f, "%.1f")) {
                        value = std::to_string(dpiScale);
                        hasChanges_ = true;
                        LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    }
                    if (!isEnabled) {
                        ImGui::PopStyleVar();
                    }
                }
            } else if (currentSection_ == "TitleDisplay") {
                if (keyCopy == "ShowWheel" || keyCopy == "ShowTitle") {
                    // Checkboxes for showing wheel and title
                    bool showValue = (value == "true");
                    if (ImGui::Checkbox("##showValue", &showValue)) {
                        value = showValue ? "true" : "false";
                        hasChanges_ = true;
                        LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    }
                } else if (keyCopy == "FontSize") {
                    // Slider for font size (10–100)
                    int fontSize = 28; // Default value
                    try {
                        fontSize = std::stoi(value);
                    } catch (...) {
                        LOG_ERROR("Invalid FontSize value: " << value << ", defaulting to 28");
                        fontSize = 28;
                    }
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::SliderInt("##fontSize", &fontSize, 10, 100, "%d")) {
                        value = std::to_string(fontSize);
                        hasChanges_ = true;
                        LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    }
                }
            } else if (currentSection_ == "WindowSettings") {
                if (keyCopy == "MainMonitor" || keyCopy == "SecondMonitor") {
                    // Combo box for selecting monitor index
                    int monitorIndex = 0;
                    try {
                        monitorIndex = std::stoi(value);
                    } catch (...) {
                        LOG_ERROR("Invalid monitor index: " << value << ", defaulting to 0");
                        monitorIndex = 0;
                    }
                    const char* monitors[] = {"0", "1", "2"}; // Simple monitor options
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::Combo("##monitor", &monitorIndex, monitors, IM_ARRAYSIZE(monitors))) {
                        value = std::to_string(monitorIndex);
                        hasChanges_ = true;
                        LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                    }
                } else if (currentSection_ == "WindowSettings") {
                    if (keyCopy == "MainMonitor" || keyCopy == "SecondMonitor") {
                        // Combo box for selecting monitor index with visible dropdown arrow
                        int monitorIndex = 0;
                        try {
                            monitorIndex = std::stoi(value);
                        } catch (...) {
                            LOG_ERROR("Invalid monitor index: " << value << ", defaulting to 0");
                            monitorIndex = 0;
                        }
                        const char* monitors[] = {"0", "1", "2"}; // Simple monitor options
                        ImGui::SetNextItemWidth(-1); // Full width to match other dropdowns
                        if (ImGui::Combo("##monitor", &monitorIndex, monitors, IM_ARRAYSIZE(monitors))) {
                            value = std::to_string(monitorIndex);
                            hasChanges_ = true;
                            LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                        }
                    } else if (keyCopy == "MainWidth" || keyCopy == "SecondWidth") {
                        // Textbox + dropdown for width settings with explicit arrow
                        std::string widthStr = value;
                        char buffer[16];
                        strncpy(buffer, widthStr.c_str(), sizeof(buffer) - 1);
                        buffer[sizeof(buffer) - 1] = '\0';
                        ImGui::SetNextItemWidth(100); // Fixed width for textbox
                        if (ImGui::InputText("##widthInput", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsDecimal)) {
                            value = std::string(buffer);
                            hasChanges_ = true;
                            LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                        }
                        ImGui::SameLine();
                        // Dropdown with common widths, ensuring arrow is visible
                        const char* commonWidths[] = {"600", "720", "768", "800", "900", "1024", "1080", "1200", "1280", "1366", "1440", "1600", "1920", "2160", "2560", "3840"};
                        int currentIndex = -1;
                        for (int i = 0; i < IM_ARRAYSIZE(commonWidths); ++i) {
                            if (value == commonWidths[i]) {
                                currentIndex = i; // Match current value to dropdown
                                break;
                            }
                        }
                        ImGui::SetNextItemWidth(100); // Match textbox width for consistency
                        if (ImGui::Combo("##commonWidth", &currentIndex, commonWidths, IM_ARRAYSIZE(commonWidths))) {
                            if (currentIndex >= 0) {
                                value = commonWidths[currentIndex];
                                strncpy(buffer, value.c_str(), sizeof(buffer) - 1); // Sync textbox
                                hasChanges_ = true;
                                LOG_DEBUG("Selected common width: " << currentSection_ << "." << keyCopy << " = " << value);
                            }
                        }
                    } else if (keyCopy == "MainHeight" || keyCopy == "SecondHeight") {
                        // Textbox + dropdown for height settings with explicit arrow
                        std::string heightStr = value;
                        char buffer[16];
                        strncpy(buffer, heightStr.c_str(), sizeof(buffer) - 1);
                        buffer[sizeof(buffer) - 1] = '\0';
                        ImGui::SetNextItemWidth(100);
                        if (ImGui::InputText("##heightInput", buffer, sizeof(buffer), ImGuiInputTextFlags_CharsDecimal)) {
                            value = std::string(buffer);
                            hasChanges_ = true;
                            LOG_DEBUG("Updated: " << currentSection_ << "." << keyCopy << " = " << value);
                        }
                        ImGui::SameLine();
                        // Dropdown with common heights, ensuring arrow is visible
                        const char* commonHeights[] = {"600", "720", "768", "800", "900", "1024", "1080", "1200", "1280", "1366", "1440", "1600", "1920", "2160", "2560", "3840"};
                        int currentIndex = -1;
                        for (int i = 0; i < IM_ARRAYSIZE(commonHeights); ++i) {
                            if (value == commonHeights[i]) {
                                currentIndex = i; // Match current value to dropdown
                                break;
                            }
                        }
                        ImGui::SetNextItemWidth(100); // Match textbox width for consistency
                        if (ImGui::Combo("##commonHeight", &currentIndex, commonHeights, IM_ARRAYSIZE(commonHeights))) {
                            if (currentIndex >= 0) {
                                value = commonHeights[currentIndex];
                                strncpy(buffer, value.c_str(), sizeof(buffer) - 1); // Sync textbox
                                hasChanges_ = true;
                                LOG_DEBUG("Selected common height: " << currentSection_ << "." << keyCopy << " = " << value);
                            }
                        }
                    }
                }
            } else {
                // **Fallback**: Generic text input for unhandled keys
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
            ImGui::PopID(); // End unique ID scope for this key
        }
        section.keyValues = keyValuesCopy; // Apply changes back to the section
    }
    ImGui::PopStyleVar();
    ImGui::EndChild(); // End key-values pane
}

// **Render Tooltip**: Displays a tooltip for a key when the "?" button is hovered
void SectionRenderer::renderTooltip(const std::string& key) {
    auto tooltips = Tooltips::getTooltips(); // Retrieve tooltip map
    if (tooltips.count(key) && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f); // Wrap text for readability
        ImGui::TextUnformatted(tooltips[key].c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}