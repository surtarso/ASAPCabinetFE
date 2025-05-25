/**
 * @file joystick_manager.h
 * @brief Defines the JoystickManager class for handling SDL joysticks in ASAPCabinetFE.
 *
 * This header provides the JoystickManager class, which initializes and manages SDL
 * joysticks for input handling. It supports adding and removing joysticks dynamically
 * and provides access to the active joystick list for use in input processing.
 */

#ifndef JOYSTICK_MANAGER_H
#define JOYSTICK_MANAGER_H

#include <SDL2/SDL.h>
#include <vector>

/**
 * @class JoystickManager
 * @brief Manages SDL joystick initialization and lifecycle.
 *
 * This class handles the initialization, addition, and removal of SDL joysticks,
 * maintaining a list of active joysticks for input processing. It is used by input
 * handling components, such as InputManager or KeybindManager, to process joystick events.
 */
class JoystickManager {
public:
    /**
     * @brief Constructs a JoystickManager instance.
     *
     * Initializes SDL joystick support and detects connected joysticks.
     */
    JoystickManager();

    /**
     * @brief Destroys the JoystickManager instance.
     *
     * Cleans up all open joysticks and shuts down SDL joystick support.
     */
    ~JoystickManager();

    /**
     * @brief Gets the list of active joysticks.
     *
     * @return A vector of pointers to active SDL joysticks.
     */
    std::vector<SDL_Joystick*> getJoysticks() const { return joysticks_; }

    /**
     * @brief Adds a joystick by index.
     *
     * Opens the joystick at the specified index and adds it to the active list.
     *
     * @param index The index of the joystick to open.
     */
    void addJoystick(int index);

    /**
     * @brief Removes a joystick by ID.
     *
     * Closes the joystick with the specified ID and removes it from the active list.
     *
     * @param id The SDL joystick ID to remove.
     */
    void removeJoystick(SDL_JoystickID id);

private:
    std::vector<SDL_Joystick*> joysticks_; ///< List of active SDL joysticks.

    /**
     * @brief Initializes connected joysticks.
     *
     * Detects and opens all connected joysticks during construction.
     */
    void initializeJoysticks();

    /**
     * @brief Cleans up all joysticks.
     *
     * Closes all open joysticks and clears the active list during destruction.
     */
    void cleanupJoysticks();
};

#endif // JOYSTICK_MANAGER_H