#include "config/ui/button_handler.h"
#include "core/app.h"
#include "imgui.h"
#include "utils/logging.h"

ButtonHandler::ButtonHandler(IConfigService* configService, App* app, bool& showConfig, bool& hasChanges, float& saveMessageTimer, const InputHandler& inputHandler, bool standaloneMode)
    : configService_(configService), app_(app), showConfig_(showConfig), hasChanges_(hasChanges), saveMessageTimer_(saveMessageTimer), inputHandler_(inputHandler), standaloneMode_(standaloneMode) {}

void ButtonHandler::renderButtonPane() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    if (ImGui::Button("Save", ImVec2(100, 0))) {
        if (onSave_) onSave_();
        configService_->saveConfig(configService_->getIniData());
        if (app_) app_->onConfigSaved(standaloneMode_); // Pass standaloneMode_
        hasChanges_ = false;
        saveMessageTimer_ = 3.0f;
        LOG_DEBUG("Config saved");
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        if (onClose_) onClose_();
        showConfig_ = false;
        LOG_DEBUG("Config closed");
    }
    ImGui::SameLine();
    if (saveMessageTimer_ > 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Saved!");
    } else if (inputHandler_.isCapturingKey()) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Waiting for keypress...");
    }
    ImGui::PopStyleVar();
}