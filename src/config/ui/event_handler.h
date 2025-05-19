#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <functional>
#include <map>
#include "config/settings_section.h"

class ConfigEventHandler {
public:
    void setOnSave(std::function<void()> onSave);
    void setOnDiscard(std::function<void()> onDiscard);
    void handleSave();
    void handleDiscard();

private:
    std::function<void()> onSave_;
    std::function<void()> onDiscard_;
};

#endif // EVENT_HANDLER_H