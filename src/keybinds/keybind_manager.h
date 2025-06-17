/**
 * @file keybind_manager.h
 * @brief Defines the KeybindManager class for managing input bindings in ASAPCabinetFE.
 *
 * This header provides the KeybindManager class, which implements the IKeybindProvider
 * interface to manage keyboard and joystick input bindings for application actions.
 * It supports keycodes, joystick buttons, hats, and axes, and handles loading/saving
 * keybind configurations.
 */

#ifndef KEYBIND_MANAGER_H
#define KEYBIND_MANAGER_H

#include "keybinds/ikeybind_provider.h"
#include <map>
#include <string>
#include <variant>

/**
 * @class KeybindManager
 * @brief Manages keyboard and joystick input bindings for application actions.
 *
 * This class implements the IKeybindProvider interface to store, retrieve, and validate
 * input bindings (keyboard keys, joystick buttons, hats, and axes) for user-defined actions.
 * It supports loading and saving keybind configurations and provides tooltip descriptions.
 */
class KeybindManager : public IKeybindProvider {
public:
    /**
     * @brief Constructs a KeybindManager instance.
     *
     * Initializes the keybind manager with default bindings for application actions.
     */
    KeybindManager();

    /**
     * @brief Destroys the KeybindManager instance.
     *
     * Default destructor, no special cleanup required.
     */
    ~KeybindManager() override = default;

    std::string getActionForKey(const std::string& key) const override;

    /**
     * @brief Gets the keyboard keycode for an action.
     *
     * Retrieves the SDL keycode associated with the specified action.
     *
     * @param action The action identifier (e.g., "LaunchTable").
     * @return The SDL keycode for the action, or SDLK_UNKNOWN if not found.
     */
    SDL_Keycode getKey(const std::string& action) const override;

    /**
     * @brief Sets the keyboard keycode for an action.
     *
     * Assigns a new SDL keycode to the specified action.
     *
     * @param action The action identifier.
     * @param key The SDL keycode to assign.
     */
    void setKey(const std::string& action, SDL_Keycode key) override;

    /**
     * @brief Gets the list of supported actions.
     *
     * Returns a vector of action identifiers that can be bound to inputs.
     *
     * @return A vector of action identifier strings.
     */
    std::vector<std::string> getActions() const override;

    /**
     * @brief Sets a joystick button binding for an action.
     *
     * Assigns a joystick button to the specified action.
     *
     * @param action The action identifier.
     * @param joystickId The ID of the joystick.
     * @param button The joystick button index.
     */
    void setJoystickButton(const std::string& action, int joystickId, uint8_t button) override;

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
    void setJoystickHat(const std::string& action, int joystickId, uint8_t hat, uint8_t direction) override;

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
    void setJoystickAxis(const std::string& action, int joystickId, uint8_t axis, bool positiveDirection) override;

    /**
     * @brief Converts an SDL event to a string representation.
     *
     * Generates a human-readable string describing the input event (key, button, hat, or axis).
     *
     * @param event The SDL event to convert.
     * @return A string describing the event.
     */
    std::string eventToString(const SDL_Event& event) const override;

    /**
     * @brief Checks if a keyboard event matches an action.
     *
     * Determines if the keyboard event corresponds to the specified action's keybinding.
     *
     * @param event The SDL keyboard event.
     * @param action The action identifier.
     * @return True if the event matches the action's keybinding, false otherwise.
     */
    bool isAction(const SDL_KeyboardEvent& event, const std::string& action) const override;

    /**
     * @brief Checks if a joystick button event matches an action.
     *
     * Determines if the joystick button event corresponds to the specified action's binding.
     *
     * @param event The SDL joystick button event.
     * @param action The action identifier.
     * @return True if the event matches the action's binding, false otherwise.
     */
    bool isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const override;

    /**
     * @brief Checks if a joystick hat event matches an action.
     *
     * Determines if the joystick hat event corresponds to the specified action's binding.
     *
     * @param event The SDL joystick hat event.
     * @param action The action identifier.
     * @return True if the event matches the action's binding, false otherwise.
     */
    bool isJoystickHatAction(const SDL_JoyHatEvent& event, const std::string& action) const override;

    /**
     * @brief Checks if a joystick axis event matches an action.
     *
     * Determines if the joystick axis event corresponds to the specified action's binding.
     *
     * @param event The SDL joystick axis event.
     * @param action The action identifier.
     * @return True if the event matches the action's binding, false otherwise.
     */
    bool isJoystickAxisAction(const SDL_JoyAxisEvent& event, const std::string& action) const override;

    /**
     * @brief Loads keybind configurations from a data map.
     *
     * Populates the keybind mappings from a provided map of action-to-input strings.
     *
     * @param keybindData A map of action identifiers to input configuration strings.
     */
    void loadKeybinds(const std::map<std::string, std::string>& keybindData) override;

    /**
     * @brief Saves keybind configurations to a map.
     *
     * Populates a map with the current keybind mappings for JSON serialization.
     *
     * @param keybinds The map to populate with action-to-input string pairs.
     */
    void saveKeybinds(std::map<std::string, std::string>& keybinds) const override;

private:
    /**
     * @struct JoystickInput
     * @brief Represents a joystick button input binding.
     */
    struct JoystickInput {
        int joystickId;     ///< The ID of the joystick.
        uint8_t button;     ///< The joystick button index.
    };

    /**
     * @struct JoystickHatInput
     * @brief Represents a joystick hat input binding.
     */
    struct JoystickHatInput {
        int joystickId;     ///< The ID of the joystick.
        uint8_t hat;        ///< The joystick hat index.
        uint8_t direction;  ///< The hat direction (e.g., SDL_HAT_UP).
    };

    /**
     * @struct JoystickAxisInput
     * @brief Represents a joystick axis input binding.
     */
    struct JoystickAxisInput {
        int joystickId;         ///< The ID of the joystick.
        uint8_t axis;           ///< The joystick axis index.
        bool positiveDirection; ///< True for positive axis direction, false for negative.
    };

    /**
     * @struct Keybind
     * @brief Stores an input binding for an action.
     *
     * Holds a variant that can represent a keyboard keycode, joystick button, hat, or axis input.
     */
    struct Keybind {
        std::variant<SDL_Keycode, JoystickInput, JoystickHatInput, JoystickAxisInput> input; ///< The input binding.
    };

    std::map<std::string, Keybind> keybinds_; ///< Map of action identifiers to keybind structures.

    /**
     * @brief Initializes default keybind mappings.
     *
     * Sets up default keyboard and joystick bindings for application actions.
     */
    void initializeDefaults();

    /**
     * @brief Normalizes an action name for consistent storage.
     *
     * Removes spaces and applies camelCase (e.g., "Previous Table" -> "PreviousTable").
     *
     * @param action The action name to normalize.
     * @return The normalized action name.
     */
};

#endif // KEYBIND_MANAGER_H