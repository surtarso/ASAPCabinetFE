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
    void renderKeybind(const std::string& key, std::string& value, InputHandler& inputHandler);
    void renderColorPicker(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
    void renderFontPath(const std::string& key, std::string& value, bool& hasChanges, const std::string& section, const std::vector<std::string>& availableFonts);
    void renderPathOrExecutable(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
    void renderCheckbox(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
    void renderDpiScale(const std::string& key, std::string& value, bool& hasChanges, const std::string& section, SettingsSection& sectionData);
    void renderSliderInt(const std::string& key, std::string& value, bool& hasChanges, const std::string& section, int min, int max);
    void renderMonitorCombo(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
    void renderResolution(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
    void renderGenericText(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
}

#endif // UI_ELEMENT_RENDERER_H