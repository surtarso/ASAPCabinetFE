#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include <functional>
#include <string>

class App;

class ButtonHandler {
public:
    ButtonHandler(bool& showConfig, float& saveMessageTimer, const InputHandler& inputHandler);
    void renderButtonPane();
    void setOnSave(std::function<void()> onSave) { onSave_ = onSave; }
    void setOnClose(std::function<void()> onClose) { onClose_ = onClose; }

private:
    bool& showConfig_;
    float& saveMessageTimer_;
    const InputHandler& inputHandler_;
    std::function<void()> onSave_;
    std::function<void()> onClose_;
};

#endif // BUTTON_HANDLER_H