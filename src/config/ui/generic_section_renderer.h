#ifndef GENERIC_SECTION_RENDERER_H
#define GENERIC_SECTION_RENDERER_H

#include "section_renderer.h"
#include <vector>
#include <string>

/**
 * @class GenericSectionRenderer
 * @brief Renderer for generic configuration sections.
 */
class GenericSectionRenderer : public BaseSectionRenderer {
public:
    GenericSectionRenderer(const std::vector<std::string>& orderedKeys)
        : orderedKeys_(orderedKeys) {}

    void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, bool defaultOpen = false, bool& isDialogOpen = *(new bool(false)), std::string& dialogKey = *(new std::string()));

private:
    std::vector<std::string> orderedKeys_;
};

#endif // GENERIC_SECTION_RENDERER_H