#ifndef UI_ELEMENT_RENDERER_H
#define UI_ELEMENT_RENDERER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>

class SectionRenderer; // Forward declaration

namespace UIElementRenderer {
    void renderKeybind(const std::string& key, std::string& value, InputHandler& inputHandler, bool& hasChanges, const std::string& section);
    void renderColorPicker([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderFontPath([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, const std::vector<std::string>& availableFonts);
    void renderPathOrExecutable([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderCheckbox([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderDpiScale([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, SettingsSection& sectionData);
    void renderSliderInt([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]]const std::string& section, int min, int max);
    void renderRotationSlider([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, int min = -360, int max = 360);
    void renderTitleDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, IConfigService* configService);
    void renderVideoBackendDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderResolution([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderGenericText([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderGenericTextShort([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
}

#endif // UI_ELEMENT_RENDERER_H