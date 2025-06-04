#include "config/ui/section_renderer.h"
#include "config/ui/ui_element_renderer.h"
#include "utils/tooltips.h"
#include "utils/logging.h"
#include <filesystem>
#include <algorithm>

SectionRenderer::SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler)
    : configService_(configService), currentSection_(currentSection), inputHandler_(inputHandler) {
    initializeFontList();
    initializeKeyRenderers();
}

void SectionRenderer::initializeKeyRenderers() {
    using namespace UIElementRenderer;
    keyRenderers_["FontColor"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderColorPicker(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["FontBgColor"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderColorPicker(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["FontPath"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderFontPath(key, value, hasChanges_, currentSection_, availableFonts_);
    };
    keyRenderers_["EnableDpiScaling"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["DpiScale"] = [this](const std::string& key, std::string& value, SettingsSection& section) {
        renderDpiScale(key, value, hasChanges_, currentSection_, section);
    };
    keyRenderers_["ShowWheel"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ShowTitle"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ShowBackglass"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ShowDMD"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ShowTopper"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ShowMetadata"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderMetadataCheckbox(key, value, hasChanges_, currentSection_, configService_);
    };
    keyRenderers_["FetchVPSdb"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ForceRebuild"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["ForceImagesOnly"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["FontSize"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderSliderInt(key, value, hasChanges_, currentSection_, 10, 100);
    };
    keyRenderers_["ScreenshotWait"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderSliderInt(key, value, hasChanges_, currentSection_, 1, 40);
    };
    keyRenderers_["TitleSource"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderTitleDropdown(key, value, hasChanges_, currentSection_, configService_);
    };
    keyRenderers_["UseVPinballXIni"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderCheckbox(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["PlayfieldWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["PlayfieldHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["PlayfieldMediaWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["PlayfieldMediaHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["PlayfieldRotation"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderRotationSlider(key, value, hasChanges_, currentSection_, -360, 360);
    };
    keyRenderers_["BackglassWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["BackglassHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["BackglassMediaWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["BackglassMediaHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["BackglassRotation"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderRotationSlider(key, value, hasChanges_, currentSection_, -360, 360);
    };
    keyRenderers_["DMDWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["DMDHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["DMDMediaWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["DMDMediaHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["DMDRotation"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderRotationSlider(key, value, hasChanges_, currentSection_, -360, 360);
    };
    keyRenderers_["TopperWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["TopperHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["TopperMediaWidth"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["TopperMediaHeight"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderResolution(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["TopperRotation"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderRotationSlider(key, value, hasChanges_, currentSection_, -360, 360);
    };
    keyRenderers_["VideoBackend"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderVideoBackendDropdown(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["MediaAudioMute"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderAudioMuteButton(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["TableMusicMute"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderAudioMuteButton(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["InterfaceAudioMute"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderAudioMuteButton(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["InterfaceAmbienceMute"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderAudioMuteButton(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["MasterMute"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderAudioMuteButton(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["MediaAudioVol"] = [this](const std::string& key, std::string& value, SettingsSection& section) {
        renderVolumeScale(key, value, hasChanges_, currentSection_, section);
    };
    keyRenderers_["TableMusicVol"] = [this](const std::string& key, std::string& value, SettingsSection& section) {
        renderVolumeScale(key, value, hasChanges_, currentSection_, section);
    };
    keyRenderers_["InterfaceAudioVol"] = [this](const std::string& key, std::string& value, SettingsSection& section) {
        renderVolumeScale(key, value, hasChanges_, currentSection_, section);
    };
    keyRenderers_["InterfaceAmbienceVol"] = [this](const std::string& key, std::string& value, SettingsSection& section) {
        renderVolumeScale(key, value, hasChanges_, currentSection_, section);
    };
    keyRenderers_["MasterVol"] = [this](const std::string& key, std::string& value, SettingsSection& section) {
        renderVolumeScale(key, value, hasChanges_, currentSection_, section);
    };
    keyRenderers_["TitleWindow"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderWheelTitleWindowDropdown(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["WheelWindow"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderWheelTitleWindowDropdown(key, value, hasChanges_, currentSection_);
    };
    keyRenderers_["TitleSortBy"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderTitleSortDropdown(key, value, hasChanges_, currentSection_);
    };
}

void SectionRenderer::renderKeyValue(const std::string& key, std::string& value, SettingsSection& section) {
    using namespace UIElementRenderer;
    if (currentSection_ == "Keybinds") {
        renderKeybind(key, value, inputHandler_, hasChanges_, currentSection_);
    } else if (keyRenderers_.count(key)) {
        keyRenderers_[key](key, value, section); // Check dispatcher first
    } else if (key.find("VPXTablesPath") != std::string::npos || 
                key.find("VPinballXPath") != std::string::npos ||
                 key.find("VPXIniPath") != std::string::npos) {
        renderPathOrExecutable(key, value, hasChanges_, currentSection_); // Only for non-FontPath "Path" keys
    } else if (key.back() == 'X' || key.back() == 'Y' ||
               key.find("WheelMediaWidth") != std::string::npos || key.find("WheelMediaHeight") != std::string::npos) {
        renderGenericTextShort(key, value, hasChanges_, currentSection_);
    } else {
        renderGenericText(key, value, hasChanges_, currentSection_);
    }
}

void SectionRenderer::initializeFontList() {
    std::string fontDir = "/usr/share/fonts/";
    if (std::filesystem::exists(fontDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(fontDir)) {
            if (entry.path().extension() == ".ttf") {
                availableFonts_.push_back(entry.path().string());
            }
        }
        std::sort(availableFonts_.begin(), availableFonts_.end());
        //LOG_DEBUG("SectionRenderer: Found " << availableFonts_.size() << " .ttf fonts in " << fontDir);
    } else {
        LOG_ERROR("SectionRenderer: Font directory " << fontDir << " not found! Font selection will be limited.");
    }
}

void SectionRenderer::renderSectionsPane(const std::vector<std::string>& sectionOrder) {
    ImGui::BeginChild("SectionsPane", ImVec2(250, 0), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Configuration Sections");
    ImGui::Separator();

    for (const auto& section : sectionOrder) {
        if (configService_->getIniData().count(section)) {
            bool selected = (currentSection_ == section);
            if (ImGui::Selectable(section.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                currentSection_ = section;
                LOG_DEBUG("SectionRenderer: Selected section: " << section);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void SectionRenderer::renderKeyValuesPane(std::map<std::string, SettingsSection>& iniData, bool& hasChanges) {
    ImGui::BeginChild("KeyValuesPane", ImVec2(0, 0), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Settings - %s", currentSection_.c_str());
    ImGui::Separator();

    if (!currentSection_.empty() && iniData.count(currentSection_)) {
        auto& section = iniData[currentSection_];
        hasChanges_ = false; // Reset changes flag for this section

        // --- IMPORTANT CHANGE STARTS HERE ---
        if (currentSection_ == "AudioSettings") {
            // If the current section is "AudioSettings", render the custom mixer
            // The dummy key/value are needed because renderAudioSettingsMixer has a signature
            // that expects them, even if it doesn't strictly use them internally for its core logic.
            // It gets the actual audio settings from 'sectionData' (which is 'section' here).
            std::string dummyKey = "";
            std::string dummyValue = "";
            UIElementRenderer::renderAudioSettingsMixer(dummyKey, dummyValue, hasChanges_, currentSection_, section);

            // Propagate changes from renderAudioSettingsMixer to the main 'hasChanges' flag
            if (hasChanges_) {
                hasChanges = true;
            }
        } else {
            // For all other sections, proceed with the generic key-value rendering loop
            float maxKeyWidth = 150.0f; // This variable is only relevant for generic rendering

            // Iterate and render each key-value pair
            for (auto& [key, value] : section.keyValues) {
                ImGui::PushID(key.c_str());
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s:", key.c_str());
                ImGui::SameLine(maxKeyWidth);

                // Render the '?' button for tooltips
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                if (ImGui::Button("?", ImVec2(16, 0))) {} // Button for tooltip trigger
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
                renderTooltip(key); // Assuming renderTooltip is in UIElementRenderer namespace or globally accessible
                ImGui::SameLine();

                // Call renderKeyValue for individual elements
                bool oldHasChanges = hasChanges; // Capture global hasChanges state before renderKeyValue
                renderKeyValue(key, value, section); // Call into UIElementRenderer namespace
                if (hasChanges_ && !oldHasChanges) { // Check if renderKeyValue set hasChanges_
                    hasChanges = true; // Propagate to ConfigUI if a change was made
                    LOG_DEBUG("SectionRenderer: Change detected in " << currentSection_ << "." << key << " = " << value);
                }
                ImGui::PopID();
            }
        }
        // --- IMPORTANT CHANGE ENDS HERE ---
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void SectionRenderer::renderTooltip(const std::string& key) {
    //LOG_DEBUG("SectionRenderer::renderTooltip called for key: " << key);
    const auto& tooltips = Tooltips::getTooltips(); // Use reference to avoid copy
    if (tooltips.count(key) && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tooltips.at(key).c_str()); // Use .at() for safety
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    //LOG_DEBUG("SectionRenderer::renderTooltip completed for key: " << key);
}