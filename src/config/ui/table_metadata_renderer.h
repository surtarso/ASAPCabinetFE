#ifndef TABLE_METADATA_RENDERER_H
#define TABLE_METADATA_RENDERER_H

#include "section_renderer.h"
#include <vector>
#include <string>

/**
 * @class TableMetadataSectionRenderer
 * @brief Renderer for the TableMetadata section in the configuration UI.
 */
class TableMetadataSectionRenderer : public BaseSectionRenderer {
public:
    TableMetadataSectionRenderer(const std::vector<std::string>& orderedKeys)
        : orderedKeys_(orderedKeys) {}

    void render(const std::string& sectionName, nlohmann::json& sectionData) override;

private:
    std::vector<std::string> orderedKeys_;
};

#endif // TABLE_METADATA_RENDERER_H