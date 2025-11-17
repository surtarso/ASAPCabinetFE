#include "core/ui/modal_dialog.h"
#include <imgui.h>

/**
 * @brief Resets all modal state. Must be called from within a locked context.
 */
void ModalDialog::reset() {
    type_ = ModalType::None;
    title_.clear();
    message_.clear();
    options_.clear();
    onConfirm_ = nullptr;
    onCancel_ = nullptr;
    resultPath_.clear();
    outputBuffer_.clear();
    selectedOption_ = 0;
    busy_ = false;
    completed_ = false;
    pendingOpen_ = false;
    scrollToBottom_ = false;
}

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
    reset();
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
    reset();
    type_ = ModalType::Progress;
    title_ = title;
    message_ = message;
    busy_ = true;
    completed_ = false;
    pendingOpen_ = true;
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
    // If progress popup was never opened (edge case), request to open it
    // so the user can see the result message and press OK.
    if (!pendingOpen_) pendingOpen_ = true;
}

void ModalDialog::openInfo(const std::string& title,
                           const std::string& message) {
    std::scoped_lock lock(mutex_);
    reset();
    type_ = ModalType::Info;
    title_ = title;
    message_ = message;
    pendingOpen_ = true;
}

void ModalDialog::openWarning(const std::string& title,
                              const std::string& message) {
    std::scoped_lock lock(mutex_);
    reset();
    type_ = ModalType::Warning;
    title_ = title;
    message_ = message;
    pendingOpen_ = true;
}

void ModalDialog::openError(const std::string& title,
                            const std::string& message) {
    std::scoped_lock lock(mutex_);
    reset();
    type_ = ModalType::Error;
    title_ = title;
    message_ = message;
    pendingOpen_ = true;
}


void ModalDialog::draw() {
    std::function<void(std::string)> deferredConfirm;
    std::function<void()> deferredCancel;
    std::string chosenOption;

    {
        std::scoped_lock lock(mutex_);
        if (type_ == ModalType::None) return;

        std::string popupId = title_ + "##" + std::to_string(reinterpret_cast<uintptr_t>(this));

        if (pendingOpen_) {
            ImGui::OpenPopup(popupId.c_str());
            pendingOpen_ = false;
        }

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 windowPos = ImVec2(
            viewport->Pos.x + viewport->Size.x * 0.5f,
            viewport->Pos.y + viewport->Size.y * 0.5f
        );

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal(popupId.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

            float wrapWidth = ImGui::GetFontSize() * 30.0f; // ~30 chars wide, tweak as needed
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);

            if (type_ == ModalType::Error)
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", message_.c_str());
            else if (type_ == ModalType::Warning)
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", message_.c_str());
            else
                ImGui::TextWrapped("%s", message_.c_str());

            ImGui::PopTextWrapPos();

            if (!options_.empty()) {
                std::vector<const char*> cstrOptions;
                for (const auto& opt : options_) cstrOptions.push_back(opt.c_str());
                ImGui::Combo("##options", &selectedOption_,
                             cstrOptions.data(), static_cast<int>(cstrOptions.size()));
            }

            // -------------------- Button Section --------------------
            if (type_ == ModalType::Confirm) {
                if (ImGui::Button("Cancel")) {
                    deferredCancel = onCancel_;
                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Confirm")) {
                    if (!options_.empty())
                        chosenOption = options_[selectedOption_];
                    deferredConfirm = onConfirm_;
                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup();
                }
            }
            else if (type_ == ModalType::Info ||
                     type_ == ModalType::Warning ||
                     type_ == ModalType::Error) {
                if (ImGui::Button("OK")) {
                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup();
                }
            }
            else if (type_ == ModalType::Progress) {
                if (busy_)
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", "Processing...");
                else if (completed_) {
                    // If the thread passed an empty message (meaning: done, hide it now)
                    if (message_.empty()) {
                        type_ = ModalType::None; // Kills the modal state
                        ImGui::CloseCurrentPopup(); // Closes the ImGui window
                    } else {
                        // This is the old "Done! OK" behavior for non-launching tasks
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", message_.c_str());
                        if (!resultPath_.empty())
                            ImGui::TextWrapped("Saved to: %s", resultPath_.c_str());
                        if (ImGui::Button("OK")) {
                            type_ = ModalType::None;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            } else if (type_ == ModalType::CommandOutput) {
                ImGui::BeginChild("##output_scroll", ImVec2(800, 500), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
                ImGui::TextUnformatted(outputBuffer_.c_str());
                if (scrollToBottom_) {
                    ImGui::SetScrollHereY(1.0f);
                    scrollToBottom_ = false;
                }
                ImGui::EndChild();

                if (ImGui::Button("Close")) {
                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    } // <-- lock released here

    // Safe to execute callbacks now
    if (deferredCancel) deferredCancel();
    if (deferredConfirm) deferredConfirm(chosenOption);
}
