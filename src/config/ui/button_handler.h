#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "config/config_service.h"
#include "config/ui/input_handler.h" // Added for InputHandler
#include <functional>
#include <string>

class App;

class ButtonHandler {
public:
    ButtonHandler(IConfigService* configService, App* app, bool& showConfig, bool& hasChanges, float& saveMessageTimer, const InputHandler& inputHandler);
    void renderButtonPane();
    void setOnSave(std::function<void()> onSave) { onSave_ = onSave; }

private:
    IConfigService* configService_;
    App* app_;
    bool& showConfig_;
    bool& hasChanges_;
    float& saveMessageTimer_;
    const InputHandler& inputHandler_; // To check capturing state
    std::function<void()> onSave_;
};

#endif // BUTTON_HANDLER_H