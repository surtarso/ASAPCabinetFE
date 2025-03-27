#ifndef IKEYBIND_PROVIDER_H
#define IKEYBIND_PROVIDER_H

#include <string>
#include <vector>
#include <SDL2/SDL.h>

class IKeybindProvider {
public:
    virtual ~IKeybindProvider() = default;
    virtual SDL_Keycode getKey(const std::string& action) const = 0;
    virtual void setKey(const std::string& action, SDL_Keycode key) = 0;
    virtual std::vector<std::string> getActions() const = 0;
    virtual std::string getTooltip(const std::string& action) const = 0;
    virtual void setJoystickButton(const std::string& action, int joystickId, uint8_t button) = 0;
    virtual void setJoystickHat(const std::string& action, int joystickId, uint8_t hat, uint8_t direction) = 0;
    virtual void setJoystickAxis(const std::string& action, int joystickId, uint8_t axis, bool positiveDirection) = 0;
    virtual std::string eventToString(const SDL_Event& event) const = 0; // New method
};

#endif // IKEYBIND_PROVIDER_H