#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include <string>
#include <vector>
#include <map>

class SectionRenderer {
public:
    SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler);
    void renderSectionsPane(const std::vector<std::string>& sectionOrder);
    void renderKeyValuesPane(std::map<std::string, SettingsSection>& iniData);

private:
    IConfigService* configService_;
    std::string& currentSection_;
    InputHandler& inputHandler_;
    bool showPicker_ = false;
    std::string currentKey_;
    std::string currentSectionForPicker_;
    void renderTooltip(const std::string& key);
};

#endif // SECTION_RENDERER_H