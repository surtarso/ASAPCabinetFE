/**
 * @file input_handler.h
 * @brief Defines the InputHandler class for managing input events in ASAPCabinetFE's configuration UI.
 *
 * This header provides the InputHandler class, which processes SDL input events to
 * capture and update keybindings in the configuration UI. It interacts with an
 * IKeybindProvider to manage keybind data and updates INI configuration data.
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "keybinds/ikeybind_provider.h"
#include "config/config_service.h"
#include <SDL2/SDL.h>
#include <string>
#include <map>

/**
 * @class InputHandler
 * @brief Handles SDL input events for keybind configuration.
 *
 * This class processes SDL events to capture key presses for configuring keybindings,
 * updating the INI configuration data in the specified section. It uses an
 * IKeybindProvider to access keybind definitions and tracks the capturing state.
 */
class InputHandler {
public:
    /**
     * @brief Constructs an InputHandler instance.
     *
     * Initializes the handler with a keybind provider for accessing keybind data.
     *
     * @param keybindProvider The keybind provider for keybind definitions.
     */
    InputHandler(IKeybindProvider* keybindProvider);

    /**
     * @brief Handles an SDL input event.
     *
     * Processes the SDL event to capture key presses for keybind configuration,
     * updating the INI data in the specified section if a keybind is captured.
     *
     * @param event The SDL event to process.
     * @param iniData The map of section names to SettingsSection objects for updates.
     * @param currentSection The current INI section being edited (e.g., "Keybinds").
     */
    void handleEvent(const SDL_Event& event, std::map<std::string, SettingsSection>& iniData, const std::string& currentSection);

    /**
     * @brief Checks if the handler is capturing a key.
     *
     * @return True if currently capturing a key, false otherwise.
     */
    bool isCapturingKey() const { return isCapturingKey_; }

    /**
     * @brief Starts capturing a key for a keybind.
     *
     * Initiates key capture for the specified keybind name.
     *
     * @param keyName The name of the keybind to capture (e.g., "NextTable").
     */
    void startCapturing(const std::string& keyName);

private:
    [[maybe_unused]] IKeybindProvider* keybindProvider_; ///< Keybind provider for keybind definitions.
    bool isCapturingKey_ = false;                               ///< Flag indicating if key capture is active.
    std::string capturingKeyName_;                      ///< Name of the keybind being captured.
    std::string capturedKeyName_;                       ///< Name of the captured key.

    /**
     * @brief Updates a keybind in the INI data.
     *
     * Applies the captured key to the INI data in the specified section.
     *
     * @param iniData The map of section names to SettingsSection objects for updates.
     * @param currentSection The current INI section being edited.
     */
    void updateKeybind(std::map<std::string, SettingsSection>& iniData, const std::string& currentSection);
};

#endif // INPUT_HANDLER_H