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
    keyRenderers_["FontSize"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderSliderInt(key, value, hasChanges_, currentSection_, 10, 100);
    };
    keyRenderers_["TitleSource"] = [this](const std::string& key, std::string& value, SettingsSection&) {
        renderTitleDropdown(key, value, hasChanges_, currentSection_, configService_);
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
}

void SectionRenderer::renderKeyValue(const std::string& key, std::string& value, SettingsSection& section) {
    using namespace UIElementRenderer;
    if (currentSection_ == "Keybinds") {
        renderKeybind(key, value, inputHandler_, hasChanges_, currentSection_);
    } else if (keyRenderers_.count(key)) {
        keyRenderers_[key](key, value, section); // Check dispatcher first
    } else if (key.find("Path") != std::string::npos || key.find("VPinballXPath") != std::string::npos) {
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
        float maxKeyWidth = 150.0f;
        hasChanges_ = false; 

        // Update keyValues directly instead of using a copy
        for (auto& [key, value] : section.keyValues) {
            ImGui::PushID(key.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s:", key.c_str());
            ImGui::SameLine(maxKeyWidth);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            if (ImGui::Button("?", ImVec2(16, 0))) {}
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            renderTooltip(key);
            ImGui::SameLine();

            bool oldHasChanges = hasChanges;
            renderKeyValue(key, value, section);
            if (hasChanges_ && !oldHasChanges) {
                hasChanges = true; // Propagate to ConfigUI
                LOG_DEBUG("SectionRenderer: Change detected in " << currentSection_ << "." << key << " = " << value);
            }
            ImGui::PopID();
        }
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