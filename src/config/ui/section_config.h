#ifndef SECTION_CONFIG_H
#define SECTION_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @class SectionConfig
 * @brief Manages section and key order for configuration UI rendering.
 */
class SectionConfig {
public:
    SectionConfig() {
        sectionOrder_ = {
            "VPX",
            "TableMetadata",
            "TitleDisplay",
            "DPISettings",
            "UIWidgets",
            "WindowSettings",
            "MediaDimensions",
            "AudioSettings",
            "CustomMedia",
            "DefaultMedia",
            "UISounds",
            "Keybinds",
            "Internal"
        };
        keyOrders_["TableMetadata"] = {
            "showMetadata",
            "fetchVPSdb",
            "forceRebuildMetadata",
            "metadataPanelWidth",
            "metadataPanelHeight",
            "metadataPanelAlpha",
            "titleSource",
            "titleSortBy"
        };
    }

    const std::vector<std::string>& getSectionOrder() const { return sectionOrder_; }
    const std::vector<std::string>& getKeyOrder(const std::string& sectionName) const {
        static const std::vector<std::string> empty;
        auto it = keyOrders_.find(sectionName);
        return it != keyOrders_.end() ? it->second : empty;
    }

private:
    std::vector<std::string> sectionOrder_;
    std::unordered_map<std::string, std::vector<std::string>> keyOrders_;
};

#endif // SECTION_CONFIG_H