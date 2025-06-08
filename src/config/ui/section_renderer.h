#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include "isection_renderer.h"
#include <vector>
#include <string>

/**
 * @class SectionRenderer
 * @brief Renderer for generic configuration sections.
 */
class SectionRenderer : public BaseSectionRenderer {
public:
    SectionRenderer(const std::vector<std::string>& orderedKeys)
        : orderedKeys_(orderedKeys) {}

    void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, bool defaultOpen = false, bool& isDialogOpen = *(new bool(false)), std::string& dialogKey = *(new std::string()));

private:
    std::vector<std::string> orderedKeys_;
};

#endif // SECTION_RENDERER_H