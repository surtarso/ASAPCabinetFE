#include "config/ui/event_handler.h"

void ConfigEventHandler::setOnSave(std::function<void()> onSave) {
    onSave_ = onSave;
}

void ConfigEventHandler::setOnDiscard(std::function<void()> onDiscard) {
    onDiscard_ = onDiscard;
}

void ConfigEventHandler::handleSave() {
    if (onSave_) onSave_();
}

void ConfigEventHandler::handleDiscard() {
    if (onDiscard_) onDiscard_();
}