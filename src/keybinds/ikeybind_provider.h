/**
 * @file ikeybind_provider.h
 * @brief Defines the IKeybindProvider interface for managing input bindings in ASAPCabinetFE.
 *
 * This header provides the IKeybindProvider interface, which specifies methods for
 * managing keyboard and joystick input bindings for application actions. It supports
 * keycodes, joystick buttons, hats, and axes, and is implemented by classes like
 * KeybindManager to handle user input configurations.
 */

#ifndef IKEYBIND_PROVIDER_H
#define IKEYBIND_PROVIDER_H

#include <string>
#include <vector>
#include <map>
#include <SDL_keycode.h>
#include <SDL_joystick.h>
#include <SDL_events.h>

/**
 * @class IKeybindProvider
 * @brief Interface for managing keyboard and joystick input bindings.
 *
 * This pure virtual class defines methods for retrieving, setting, and validating
 * input bindings for application actions, including keyboard keys, joystick buttons,
 * hats, and axes. It also provides utilities for converting events to strings and
 * managing keybind persistence.
 */
class IKeybindProvider {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IKeybindProvider() = default;

    // virtual std::string normalizeAction(const std::string& action) const = 0;

    virtual std::string getActionForKey(const std::string& key) const = 0;
    /**
     * @brief Gets the keyboard keycode for an action.
     *
     * Retrieves the SDL keycode associated with the specified action.
     *
     * @param action The action identifier (e.g., "LaunchTable").
     * @return The SDL keycode for the action, or SDLK_UNKNOWN if not found.
     */
    virtual SDL_Keycode getKey(const std::string& action) const = 0;

    /**
     * @brief Sets the keyboard keycode for an action.
     *
     * Assigns a new SDL keycode to the specified action.
     *
     * @param action The action identifier.
     * @param key The SDL keycode to assign.
     */
    virtual void setKey(const std::string& action, SDL_Keycode key) = 0;

    /**
     * @brief Gets the list of supported actions.
     *
     * Returns a vector of action identifiers that can be bound to inputs.
     *
     * @return A vector of action identifier strings.
     */
    virtual std::vector<std::string> getActions() const = 0;

    /**
     * @brief Sets a joystick button binding for an action.
     *
     * Assigns a joystick button to the specified action.
     *
     * @param action The action identifier.
     * @param joystickId The ID of the joystick.
     * @param button The joystick button index.
     */
    virtual void setJoystickButton(const std::string& action, int joystickId, uint8_t button) = 0;

    /**
     * @brief Sets a joystick hat binding for an action.
     *
     * Assigns a joystick hat direction to the specified action.
     *
     * @param action The action identifier.
     * @param joystickId The ID of the joystick.
     * @param hat The joystick hat index.
     * @param direction The hat direction (e.g., SDL_HAT_UP).
     */
    virtual void setJoystickHat(const std::string& action, int joystickId, uint8_t hat, uint8_t direction) = 0;

    /**
     * @brief Sets a joystick axis binding for an action.
     *
     * Assigns a joystick axis direction to the specified action.
     *
     * @param action The action identifier.
     * @param joystickId The ID of the joystick.
     * @param axis The joystick axis index.
     * @param positiveDirection True for positive axis direction, false for negative.
     */
    virtual void setJoystickAxis(const std::string& action, int joystickId, uint8_t axis, bool positiveDirection) = 0;

    /**
     * @brief Converts an SDL event to a string representation.
     *
     * Generates a human-readable string describing the input event (key, button, hat, or axis).
     *
     * @param event The SDL event to convert.
     * @return A string describing the event.
     */
    virtual std::string eventToString(const SDL_Event& event) const = 0;

    /**
     * @brief Checks if a keyboard event matches an action.
     *
     * Determines if the keyboard event corresponds to the specified action's keybinding.
     *
     * @param event The SDL keyboard event.
     * @param action The action identifier.
     * @return True if the event matches the action's keybinding, false otherwise.
     */
    virtual bool isAction(const SDL_KeyboardEvent& event, const std::string& action) const = 0;

    /**
     * @brief Checks if a joystick button event matches an action.
     *
     * Determines if the joystick button event corresponds to the specified action's binding.
     *
     * @param event The SDL joystick button event.
     * @param action The action identifier.
     * @return True if the event matches the action's binding, false otherwise.
     */
    virtual bool isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const = 0;

    /**
     * @brief Checks if a joystick hat event matches an action.
     *
     * Determines if the joystick hat event corresponds to the specified action's binding.
     *
     * @param event The SDL joystick hat event.
     * @param action The action identifier.
     * @return True if the event matches the action's binding, false otherwise.
     */
    virtual bool isJoystickHatAction(const SDL_JoyHatEvent& event, const std::string& action) const = 0;

    /**
     * @brief Checks if a joystick axis event matches an action.
     *
     * Determines if the joystick axis event corresponds to the specified action's binding.
     *
     * @param event The SDL joystick axis event.
     * @param action The action identifier.
     * @return True if the event matches the action's binding, false otherwise.
     */
    virtual bool isJoystickAxisAction(const SDL_JoyAxisEvent& event, const std::string& action) const = 0;

    /**
     * @brief Loads keybind configurations from a data map.
     *
     * Populates the keybind mappings from a provided map of action-to-input strings.
     *
     * @param keybindData A map of action identifiers to input configuration strings.
     */
    virtual void loadKeybinds(const std::map<std::string, std::string>& keybindData) = 0;

    /**
     * @brief Saves keybind configurations to a map.
     *
     * Populates a map with the current keybind mappings for JSON serialization.
     *
     * @param keybinds The map to populate with action-to-input string pairs.
     */
    virtual void saveKeybinds(std::map<std::string, std::string>& keybinds) const = 0;
};

#endif // IKEYBIND_PROVIDER_H