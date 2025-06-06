/**
 * @file ui_element_renderer.h
 * @brief Defines the UIElementRenderer namespace for rendering ImGui UI elements in ASAPCabinetFE.
 *
 * This header provides the UIElementRenderer namespace, which contains functions for
 * rendering specific ImGui UI elements (e.g., keybinds, checkboxes, sliders) used in
 * the configuration UI. These functions handle user input and update configuration
 * values, integrating with InputHandler and IConfigService.
 */

#ifndef UI_ELEMENT_RENDERER_H
#define UI_ELEMENT_RENDERER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>

/**
 * @class SectionRenderer
 * @brief Class for rendering configuration sections (forward declaration).
 */
class SectionRenderer;

/**
 * @namespace UIElementRenderer
 * @brief Provides functions for rendering ImGui UI elements for configuration settings.
 *
 * This namespace contains static functions that render specific ImGui UI elements for
 * editing configuration key-value pairs, such as keybinds, color pickers, and sliders.
 * Each function updates the provided value and tracks changes, interacting with
 * InputHandler for input events and IConfigService for settings.
 */
namespace UIElementRenderer {
    /**
     * @brief Renders a keybind input field.
     *
     * Displays an ImGui input field for configuring a keybind, updating the value
     * based on user input via InputHandler.
     *
     * @param key The configuration key (e.g., "NextTable").
     * @param value The current keybind value (e.g., "Right Shift").
     * @param inputHandler The input handler for capturing key events.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name (e.g., "Keybinds").
     */
    void renderKeybind(const std::string& key, std::string& value, InputHandler& inputHandler, bool& hasChanges, const std::string& section);

    /**
     * @brief Renders a color picker for SDL_Color settings.
     *
     * Displays an ImGui color picker for editing color values, updating the value
     * if changed.
     *
     * @param key The configuration key.
     * @param value The current color value (e.g., "255,255,255,255").
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderColorPicker([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);

    /**
     * @brief Renders a font path dropdown.
     *
     * Displays an ImGui dropdown for selecting a font path from available fonts,
     * updating the value if changed.
     *
     * @param key The configuration key.
     * @param value The current font path.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param availableFonts List of available font paths.
     */
    void renderFontPath([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, const std::vector<std::string>& availableFonts);

    /**
     * @brief Renders a path or executable input field.
     *
     * Displays an ImGui input field for editing a file path or executable, updating
     * the value if changed.
     *
     * @param key The configuration key.
     * @param value The current path or executable.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderPathOrExecutable([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);

    /**
     * @brief Renders a checkbox for boolean settings.
     *
     * Displays an ImGui checkbox for toggling a boolean value, updating the value
     * if changed.
     *
     * @param key The configuration key.
     * @param value The current boolean value (e.g., "true").
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderCheckbox([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);

    /**
     * @brief Renders a checkbox for metadata settings.
     *
     * Displays an ImGui checkbox for toggling metadata-related settings, updating
     * the value if changed, using IConfigService for context.
     *
     * @param key The configuration key.
     * @param value The current boolean value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param configService The configuration service for settings access.
     */
    void renderMetadataCheckbox([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, IConfigService* configService);

    /**
     * @brief Renders a DPI scale slider.
     *
     * Displays an ImGui slider for adjusting DPI scaling, updating the value in the
     * SettingsSection if changed.
     *
     * @param key The configuration key.
     * @param value The current DPI scale value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param sectionData The SettingsSection containing the key-value pair.
     */
    void renderDpiScale([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, SettingsSection& sectionData);

    /**
     * @brief Renders a volume scale slider.
     *
     * Displays an ImGui slider for adjusting volume scaling, updating the value in
     * the SettingsSection if changed.
     *
     * @param key The configuration key.
     * @param value The current volume scale value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param sectionData The SettingsSection containing the key-value pair.
     */
    void renderVolumeScale([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, SettingsSection& sectionData);

    /**
     * @brief Renders an integer slider.
     *
     * Displays an ImGui slider for adjusting an integer value within a range,
     * updating the value if changed.
     *
     * @param key The configuration key.
     * @param value The current integer value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param min The minimum value of the slider.
     * @param max The maximum value of the slider.
     */
    void renderSliderInt([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, int min, int max);

    /**
     * @brief Renders a rotation slider.
     *
     * Displays an ImGui slider for adjusting a rotation angle, updating the value
     * if changed.
     *
     * @param key The configuration key.
     * @param value The current rotation angle.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param min The minimum angle (default: -360).
     * @param max The maximum angle (default: 360).
     */
    void renderRotationSlider([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, int min = -360, int max = 360);

    /**
     * @brief Renders a title dropdown.
     *
     * Displays an ImGui dropdown for selecting a title, updating the value if
     * changed, using IConfigService for context.
     *
     * @param key The configuration key.
     * @param value The current title value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     * @param configService The configuration service for settings access.
     */
    void renderTitleDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, IConfigService* configService);

    /**
     * @brief Renders a video backend dropdown.
     *
     * Displays an ImGui dropdown for selecting a video backend, updating the value
     * if changed.
     *
     * @param key The configuration key.
     * @param value The current video backend value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderVideoBackendDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);

    /**
     * @brief Renders a resolution input field.
     *
     * Displays an ImGui input field for editing a resolution (e.g., "1920x1080"),
     * updating the value if changed.
     *
     * @param key The configuration key.
     * @param value The current resolution value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderResolution([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);

    /**
     * @brief Renders a generic text input field.
     *
     * Displays an ImGui input field for editing a generic text value, updating the
     * value if changed.
     *
     * @param key The configuration key.
     * @param value The current text value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderGenericText([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);

    /**
     * @brief Renders a short generic text input field.
     *
     * Displays a compact ImGui input field for editing a short text value, updating
     * the value if changed.
     *
     * @param key The configuration key.
     * @param value The current text value.
     * @param hasChanges Reference to flag indicating if changes were made.
     * @param section The INI section name.
     */
    void renderGenericTextShort([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section);
    void renderAudioSettingsMixer([[maybe_unused]] const std::string& key, [[maybe_unused]] std::string& value, bool& hasChanges, const std::string& section, SettingsSection& sectionData);
    void renderAudioMuteButton(const std::string& key, std::string& value, bool& hasChanges, const std::string& section);
    void renderWheelTitleWindowDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges,[[maybe_unused]] const std::string& section);
    void renderTitleSortDropdown([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges,[[maybe_unused]] const std::string& section);
    void renderSliderFloat([[maybe_unused]] const std::string& key, std::string& value, bool& hasChanges, [[maybe_unused]] const std::string& section, float min, float max);
}

#endif // UI_ELEMENT_RENDERER_H