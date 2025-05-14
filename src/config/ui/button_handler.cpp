#include "config/ui/button_handler.h"
#include "core/app.h"
#include "imgui.h"
#include "utils/logging.h"

ButtonHandler::ButtonHandler(bool& showConfig, float& saveMessageTimer, const InputHandler& inputHandler)
    : showConfig_(showConfig), saveMessageTimer_(saveMessageTimer), inputHandler_(inputHandler) {}

void ButtonHandler::renderButtonPane() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    if (ImGui::Button("Save", ImVec2(100, 0))) {
        if (onSave_) {
            onSave_(); // Calls ConfigUI::saveConfig
            LOG_DEBUG("Save button pressed, onSave_ called");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        if (onClose_) {
            onClose_(); // Calls ConfigUI::discardChanges
            LOG_DEBUG("Config closed");
        }
        saveMessageTimer_ = 0.0f;
        showConfig_ = false;
    }
    ImGui::SameLine();
    if (saveMessageTimer_ > 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Saved!");
    } else if (inputHandler_.isCapturingKey()) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Waiting for keypress...");
    }
    ImGui::PopStyleVar();
}