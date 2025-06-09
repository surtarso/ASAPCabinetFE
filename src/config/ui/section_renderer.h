/**
 * @file section_renderer.h
 * @brief Defines the SectionRenderer class for rendering configuration sections in ASAPCabinetFE.
 *
 * This header provides the SectionRenderer class, which implements the ISectionRenderer
 * interface to render generic configuration sections using ImGui. It uses a base class
 * for common rendering utilities and is tailored for section-specific rendering logic.
 */

#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include "isection_renderer.h"
#include "config_ui.h"
#include <vector>
#include <string>

/**
 * @class SectionRenderer
 * @brief Renderer for generic configuration sections.
 *
 * This class implements the ISectionRenderer interface to render configuration sections
 * with ordered keys, grouped fields, and reset-to-default functionality. It leverages
 * BaseSectionRenderer utilities and integrates with ConfigUI for section management.
 */
class SectionRenderer : public BaseSectionRenderer {
public:
    /**
     * @brief Constructs a SectionRenderer instance.
     *
     * Initializes the renderer with a ConfigUI instance and a vector of ordered keys.
     *
     * @param configUI Pointer to the ConfigUI instance for section management.
     * @param orderedKeys Vector of keys defining the rendering order.
     */
    SectionRenderer(ConfigUI* configUI, const std::vector<std::string>& orderedKeys)
        : configUI_(configUI), orderedKeys_(orderedKeys) {}

    /**
     * @brief Renders a configuration section in the UI.
     *
     * Renders the specified section with its JSON data, handling key capture, file dialogs,
     * grouped fields, and reset functionality.
     *
     * @param sectionName The name of the configuration section to render.
     * @param sectionData Reference to the JSON data for the section.
     * @param isCapturing Reference to a flag indicating if a key is being captured.
     * @param capturingKeyName Reference to the name of the key being captured.
     * @param fileDialog Pointer to the ImGuiFileDialog instance for file selection.
     * @param defaultOpen Optional flag to open the section by default (default: false).
     * @param isDialogOpen Reference to a flag indicating if a file dialog is open (default: new bool(false)).
     * @param dialogKey Reference to the key associated with the open dialog (default: new std::string()).
     */
    void render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, ImGuiFileDialog* fileDialog, bool defaultOpen = false, bool& isDialogOpen = *(new bool(false)), std::string& dialogKey = *(new std::string()));

private:
    ConfigUI* configUI_;          ///< Pointer to the ConfigUI instance for section management.
    std::vector<std::string> orderedKeys_; ///< Vector of keys defining the rendering order.
};

#endif // SECTION_RENDERER_H