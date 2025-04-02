#ifndef JOYSTICK_MANAGER_H
#define JOYSTICK_MANAGER_H

#include <SDL2/SDL.h>
#include <vector>

class JoystickManager {
public:
    JoystickManager();
    ~JoystickManager();
    std::vector<SDL_Joystick*> getJoysticks() const { return joysticks_; }
    void addJoystick(int index);
    void removeJoystick(SDL_JoystickID id);

private:
    std::vector<SDL_Joystick*> joysticks_;
    void initializeJoysticks();
    void cleanupJoysticks();
};

#endif // JOYSTICK_MANAGER_H