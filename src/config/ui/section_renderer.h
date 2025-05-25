/**
 * @file section_renderer.h
 * @brief Defines the SectionRenderer class for rendering configuration sections in ASAPCabinetFE.
 *
 * This header provides the SectionRenderer class, which manages the ImGui rendering of
 * configuration sections and key-value pairs. It integrates with IConfigService for
 * settings, InputHandler for input events, and UIElementRenderer for specific UI elements.
 */

#ifndef SECTION_RENDERER_H
#define SECTION_RENDERER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

/**
 * @class SectionRenderer
 * @brief Manages ImGui rendering of configuration sections and key-value pairs.
 *
 * This class renders a pane for selecting configuration sections and a pane for editing
 * key-value pairs within the selected section. It uses UIElementRenderer to render
 * specific UI elements and tracks changes to INI data, integrating with IConfigService
 * and InputHandler.
 */
class SectionRenderer {
public:
    /**
     * @brief Constructs a SectionRenderer instance.
     *
     * Initializes the renderer with a configuration service, current section reference,
     * and input handler, and sets up font lists and key renderers.
     *
     * @param configService The configuration service for accessing settings.
     * @param currentSection Reference to the currently selected section name.
     * @param inputHandler The input handler for capturing user input.
     */
    SectionRenderer(IConfigService* configService, std::string& currentSection, InputHandler& inputHandler);

    /**
     * @brief Renders the sections selection pane.
     *
     * Displays an ImGui pane listing available configuration sections, allowing the
     * user to select a section.
     *
     * @param sectionOrder The ordered list of section names to display.
     */
    void renderSectionsPane(const std::vector<std::string>& sectionOrder);

    /**
     * @brief Renders the key-value editing pane.
     *
     * Displays an ImGui pane for editing key-value pairs in the current section,
     * using UIElementRenderer for specific UI elements and tracking changes.
     *
     * @param iniData The map of section names to SettingsSection objects.
     * @param hasChanges Reference to flag indicating if changes were made.
     */
    void renderKeyValuesPane(std::map<std::string, SettingsSection>& iniData, bool& hasChanges);

private:
    IConfigService* configService_; ///< Configuration service for accessing settings.
    std::string& currentSection_;   ///< Reference to the currently selected section name.
    InputHandler& inputHandler_;    ///< Input handler for capturing user input.
    std::vector<std::string> availableFonts_; ///< List of available font paths.
    bool hasChanges_;               ///< Flag indicating if changes were made.
    std::map<std::string, std::function<void(const std::string&, std::string&, SettingsSection&)>> keyRenderers_; ///< Dispatcher table for key-specific rendering.

    /**
     * @brief Renders a tooltip for a configuration key.
     *
     * Displays an ImGui tooltip with information about the specified key.
     *
     * @param key The configuration key.
     */
    void renderTooltip(const std::string& key);

    /**
     * @brief Initializes the list of available fonts.
     *
     * Populates the availableFonts_ vector with valid font paths.
     */
    void initializeFontList();

    /**
     * @brief Initializes the key renderers dispatcher table.
     *
     * Sets up the keyRenderers_ map with functions for rendering specific keys,
     * using UIElementRenderer.
     */
    void initializeKeyRenderers();

    /**
     * @brief Renders a single key-value pair.
     *
     * Displays the UI element for the specified key-value pair using the appropriate
     * renderer from keyRenderers_, updating the value if changed.
     *
     * @param key The configuration key.
     * @param value The current value.
     * @param section The SettingsSection containing the key-value pair.
     */
    void renderKeyValue(const std::string& key, std::string& value, SettingsSection& section);
};

#endif // SECTION_RENDERER_H