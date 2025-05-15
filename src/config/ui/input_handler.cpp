#include "config/ui/input_handler.h"
#include "utils/logging.h"

InputHandler::InputHandler(IKeybindProvider* keybindProvider)
    : keybindProvider_(keybindProvider) {
        LOG_DEBUG("InputHandler: Initialized with keybindProvider: " << keybindProvider_);
    }

void InputHandler::startCapturing(const std::string& keyName) {
    isCapturingKey_ = true;
    capturingKeyName_ = keyName;
    capturedKeyName_.clear();
    LOG_DEBUG("InputHandler: Started capturing for key: " << keyName);
}

void InputHandler::handleEvent(const SDL_Event& event, std::map<std::string, SettingsSection>& iniData, const std::string& currentSection) {
    if (!isCapturingKey_) return;

    if (event.type == SDL_KEYDOWN) {
        capturedKeyName_ = SDL_GetKeyName(event.key.keysym.sym);
        LOG_DEBUG("InputHandler: Captured key: " << capturedKeyName_ << " for " << capturingKeyName_);
        updateKeybind(iniData, currentSection);
        isCapturingKey_ = false;
    } else if (event.type == SDL_JOYBUTTONDOWN) {
        capturedKeyName_ = "Joy" + std::to_string(event.jbutton.which) + "Button" + std::to_string(event.jbutton.button);
        LOG_DEBUG("InputHandler: Captured joystick button: " << capturedKeyName_ << " for " << capturingKeyName_);
        updateKeybind(iniData, currentSection);
        isCapturingKey_ = false;
    }
}

void InputHandler::updateKeybind(std::map<std::string, SettingsSection>& iniData, const std::string& currentSection) {
    if (currentSection != "Keybinds" || capturedKeyName_.empty()) return;

    auto& section = iniData[currentSection];
    for (auto& [key, value] : section.keyValues) {
        if (key == capturingKeyName_) {
            value = capturedKeyName_;
            LOG_DEBUG("InputHandler: Updated keybind: " << key << " = " << value);
            break;
        }
    }
}