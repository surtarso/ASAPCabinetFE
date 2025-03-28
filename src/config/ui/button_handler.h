#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "config/config_service.h"
#include "config/ui/input_handler.h"
#include <functional>
#include <string>

class App;

class ButtonHandler {
public:
    ButtonHandler(IConfigService* configService, App* app, bool& showConfig, bool& hasChanges, float& saveMessageTimer, const InputHandler& inputHandler);
    void renderButtonPane();
    void setOnSave(std::function<void()> onSave) { onSave_ = onSave; }
    void setOnClose(std::function<void()> onClose) { onClose_ = onClose; } // New callback for Close

private:
    IConfigService* configService_;
    App* app_;
    bool& showConfig_;
    bool& hasChanges_;
    float& saveMessageTimer_;
    const InputHandler& inputHandler_;
    std::function<void()> onSave_;
    std::function<void()> onClose_; // New callback
};

#endif // BUTTON_HANDLER_H