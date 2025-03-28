#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "keybinds/ikeybind_provider.h"
#include "config/config_service.h" // Added for SettingsSection
#include <SDL2/SDL.h>
#include <string>
#include <map>

class InputHandler {
public:
    InputHandler(IKeybindProvider* keybindProvider);
    void handleEvent(const SDL_Event& event, std::map<std::string, SettingsSection>& iniData, const std::string& currentSection);
    bool isCapturingKey() const { return isCapturingKey_; }
    void startCapturing(const std::string& keyName);

private:
    IKeybindProvider* keybindProvider_;
    bool isCapturingKey_ = false;
    std::string capturingKeyName_;
    std::string capturedKeyName_;
    void updateKeybind(std::map<std::string, SettingsSection>& iniData, const std::string& currentSection);
};

#endif // INPUT_HANDLER_H