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
};

#endif // IKEYBIND_PROVIDER_H