#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

class SectionRenderer {
public:
    SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler);
    void renderSectionsPane(const std::vector<std::string>& sectionOrder);
    void renderKeyValuesPane(std::map<std::string, SettingsSection>& iniData, bool& hasChanges);

private:
    IConfigService* configService_;
    std::string& currentSection_;
    InputHandler& inputHandler_;
    std::vector<std::string> availableFonts_;
    bool hasChanges_ = false;

    // Dispatcher table for key-specific rendering
    std::map<std::string, std::function<void(const std::string&, std::string&, SettingsSection&)>> keyRenderers_;

    void renderTooltip(const std::string& key);
    void initializeFontList();
    void initializeKeyRenderers();
    void renderKeyValue(const std::string& key, std::string& value, SettingsSection& section);
};

#endif // SECTION_RENDERER_H