#ifndef KEYBIND_MANAGER_H
#define KEYBIND_MANAGER_H

#include "keybinds/ikeybind_provider.h"
#include <map>
#include <string>
#include <SDL2/SDL.h>

class KeybindManager : public IKeybindProvider {
public:
    KeybindManager();
    ~KeybindManager() override = default;

    // IKeybindProvider interface
    SDL_Keycode getKey(const std::string& action) const override;
    void setKey(const std::string& action, SDL_Keycode key) override;
    std::vector<std::string> getActions() const override;
    std::string getTooltip(const std::string& action) const override;

    // Methods for loading/saving keybindings
    void loadKeybinds(const std::map<std::string, std::string>& keybindData);
    void saveKeybinds(std::ofstream& file) const;

private:
    struct Keybind {
        SDL_Keycode key;
        std::string tooltip;
    };

    std::map<std::string, Keybind> keybinds_;

    void initializeDefaults();
};

#endif // KEYBIND_MANAGER_H