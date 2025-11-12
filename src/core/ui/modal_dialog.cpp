#include "core/ui/modal_dialog.h"
#include <imgui.h>

ModalDialog::ModalDialog()
    : type_(ModalType::None), selectedOption_(0),
      busy_(false), completed_(false), pendingOpen_(false) {}

bool ModalDialog::isActive() const {
    std::scoped_lock lock(mutex_);
    return type_ != ModalType::None;
}

void ModalDialog::openConfirm(const std::string& title,
                              const std::string& message,
                              const std::vector<std::string>& options,
                              std::function<void(const std::string&)> onConfirm,
                              std::function<void()> onCancel) {
    std::scoped_lock lock(mutex_);
    type_ = ModalType::Confirm;
    title_ = title;
    message_ = message;
    options_ = options;
    onConfirm_ = std::move(onConfirm);
    onCancel_ = std::move(onCancel);
    selectedOption_ = 0;
    busy_ = false;
    completed_ = false;
    resultPath_.clear();
    pendingOpen_ = true;
    // ImGui::OpenPopup(title_.c_str());
}

void ModalDialog::openProgress(const std::string& title,
                               const std::string& message) {
    std::scoped_lock lock(mutex_);
    type_ = ModalType::Progress;
    title_ = title;
    message_ = message;
    busy_ = true;
    completed_ = false;
}

void ModalDialog::updateProgress(const std::string& message) {
    std::scoped_lock lock(mutex_);
    message_ = message;
}

void ModalDialog::finishProgress(const std::string& resultMessage,
                                 const std::string& resultPath) {
    std::scoped_lock lock(mutex_);
    message_ = resultMessage;
    resultPath_ = resultPath;
    busy_ = false;
    completed_ = true;
}

void ModalDialog::openInfo(const std::string& title,
                           const std::string& message) {
    std::scoped_lock lock(mutex_);
    type_ = ModalType::Info;
    title_ = title;
    message_ = message;
    pendingOpen_ = true;
}

void ModalDialog::openWarning(const std::string& title,
                              const std::string& message) {
    std::scoped_lock lock(mutex_);
    type_ = ModalType::Warning;
    title_ = title;
    message_ = message;
    pendingOpen_ = true;
}

void ModalDialog::openError(const std::string& title,
                            const std::string& message) {
    std::scoped_lock lock(mutex_);
    type_ = ModalType::Error;
    title_ = title;
    message_ = message;
    pendingOpen_ = true;
}


void ModalDialog::draw() {
    std::scoped_lock lock(mutex_);
    if (type_ == ModalType::None) return;

    std::string popupId = title_ + "##" + std::to_string(reinterpret_cast<uintptr_t>(this));

    if (pendingOpen_) {
        ImGui::OpenPopup(popupId.c_str());
        pendingOpen_ = false;
    }

    // Compute real center in global coordinates
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    float verticalOffset = -viewport->Size.y * 0.15f;
    ImVec2 windowPos = ImVec2(
        viewport->Pos.x + viewport->Size.x * 0.5f,
        viewport->Pos.y + viewport->Size.y * 0.5f - verticalOffset
    );

    // Forcefully reset to root layer (so it's not affected by parent window stack)
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    // ImGui::SetNextWindowViewport(viewport->ID);


    if (ImGui::BeginPopupModal(popupId.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (type_ == ModalType::Error)
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", message_.c_str());
        else if (type_ == ModalType::Warning)
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", message_.c_str());
        else
            ImGui::TextWrapped("%s", message_.c_str());
        // ImGui::TextWrapped("%s", message_.c_str());

        if (!options_.empty()) {
            std::vector<const char*> cstrOptions;
            cstrOptions.reserve(options_.size());
            for (const auto& opt : options_)
                cstrOptions.push_back(opt.c_str());

            ImGui::Combo("##options", &selectedOption_,
                        cstrOptions.data(),
                        static_cast<int>(cstrOptions.size()));
        }

        if (busy_)
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Processing...");
        else if (completed_) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Done!");
            if (!resultPath_.empty())
                ImGui::TextWrapped("Saved to: %s", resultPath_.c_str());
        }

        if (!busy_ && !completed_) {
            if (ImGui::Button("Cancel")) {
                if (onCancel_) onCancel_();
                type_ = ModalType::None;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Confirm")) {
                std::string chosen;
                if (!options_.empty())
                    chosen = options_[selectedOption_];
                if (onConfirm_) onConfirm_(chosen);
            }
        } else if (completed_) {
            if (ImGui::Button("OK")) {
                type_ = ModalType::None;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }

    // ImGui::PopID();
}
