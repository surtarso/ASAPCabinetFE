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
    visibleFramesRequired_ = 1;
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

    // Require at least 1 frame visible before closing
    visibleFramesRequired_ = std::max(visibleFramesRequired_, 1);
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

void ModalDialog::enqueueUiTask(std::function<void()> fn) {
    std::scoped_lock lk(uiTaskMutex_);
    uiTasks_.push_back(std::move(fn));
}

void ModalDialog::requestFinishProgress(const std::string& resultMessage,
                                        const std::string& resultPath) {
    // Enqueue a small task that will call finishProgress() from the UI thread.
    enqueueUiTask([this, resultMessage, resultPath]() {
        // finishProgress already takes the modal mutex internally.
        this->finishProgress(resultMessage, resultPath);
    });
}

void ModalDialog::draw() {
    // ---- Execute queued UI tasks first (on UI/main thread) ----
    {
        std::vector<std::function<void()>> localTasks;
        {
            std::scoped_lock lk(uiTaskMutex_);
            localTasks.swap(uiTasks_);
        }
        for (auto &t : localTasks) {
            t();
        }
    }

    std::function<void(std::string)> deferredConfirm;
    std::function<void()> deferredCancel;
    std::string chosenOption;

    {
        std::scoped_lock lock(mutex_);
        if (type_ == ModalType::None) return;

        std::string popupId =
            title_ + "##" + std::to_string(reinterpret_cast<uintptr_t>(this));

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

            float wrapWidth = ImGui::GetFontSize() * 30.0f;
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);

            if (type_ == ModalType::Error)
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", message_.c_str());
            else if (type_ == ModalType::Warning)
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", message_.c_str());
            else
                ImGui::TextWrapped("%s", message_.c_str());

            ImGui::PopTextWrapPos();


            // ---------------------------------------------------------
            // OPTIONS DROPDOWN (hidden for Confirm modals)
            // ---------------------------------------------------------
            if (type_ != ModalType::Confirm && !options_.empty()) {
                std::vector<const char*> cstrOptions;
                cstrOptions.reserve(options_.size());
                for (const auto& opt : options_)
                    cstrOptions.push_back(opt.c_str());

                ImGui::Combo("##options",
                             &selectedOption_,
                             cstrOptions.data(),
                             static_cast<int>(cstrOptions.size()));
            }


            // ---------------------------------------------------------
            // CONFIRM MODAL BUTTONS (centered)
            // ---------------------------------------------------------
            // Add space or line between text and buttons
            ImGui::Separator();
            ImGui::Spacing();

            if (type_ == ModalType::Confirm)
            {
                std::string yesLabel = options_.size() >= 1 ? options_[0] : "Yes";
                std::string noLabel  = options_.size() >= 2 ? options_[1] : "No";

                const float buttonWidth = 120.0f;
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const float totalWidth = (buttonWidth * 2.0f) + spacing;
                float regionWidth = ImGui::GetContentRegionAvail().x;
                float offsetX     = (regionWidth - totalWidth) * 0.5f;

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

                // YES
                if (ImGui::Button(yesLabel.c_str(), ImVec2(buttonWidth, 0))) {
                    chosenOption = yesLabel;
                    deferredConfirm = onConfirm_;
                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup(); // safe - no return
                }

                ImGui::SameLine();

                // NO
                if (ImGui::Button(noLabel.c_str(), ImVec2(buttonWidth, 0))) {
                    chosenOption = noLabel;

                    if (onCancel_) deferredCancel = onCancel_;
                    else deferredConfirm = onConfirm_;

                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup(); // safe
                }
            }


            // ---------------------------------------------------------
            // INFO / WARNING / ERROR (simple OK button)
            // ---------------------------------------------------------
            else if (type_ == ModalType::Info ||
                     type_ == ModalType::Warning ||
                     type_ == ModalType::Error)
            {
                if (ImGui::Button("OK")) {
                    type_ = ModalType::None;
                    ImGui::CloseCurrentPopup();
                }
            }


            // ---------------------------------------------------------
            // PROGRESS POPUP
            // ---------------------------------------------------------
            else if (type_ == ModalType::Progress) {
                if (busy_) {
                    ImGui::TextColored(ImVec4(1,1,0,1), "Processing...");
                }
                else if (completed_) {
                    if (visibleFramesRequired_ > 0) {
                        visibleFramesRequired_--;
                        ImGui::TextColored(ImVec4(0,1,0,1), "%s", message_.c_str());
                        if (!resultPath_.empty())
                            ImGui::TextWrapped("Saved to: %s", resultPath_.c_str());
                        ImGui::EndPopup();
                        return;
                    }

                    if (message_.empty()) {
                        type_ = ModalType::None;
                        ImGui::CloseCurrentPopup();
                    } else {
                        ImGui::TextColored(ImVec4(0,1,0,1), "%s", message_.c_str());
                        if (!resultPath_.empty())
                            ImGui::TextWrapped("Saved to: %s", resultPath_.c_str());
                        if (ImGui::Button("OK")) {
                            type_ = ModalType::None;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }


            // ---------------------------------------------------------
            // COMMAND OUTPUT WINDOW
            // ---------------------------------------------------------
            else if (type_ == ModalType::CommandOutput) {

                ImGui::BeginChild("##output_scroll", ImVec2(800, 500), true,
                                  ImGuiWindowFlags_AlwaysHorizontalScrollbar);

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
    } // lock released here

    // ---------------------------------------------------------
    // EXECUTE CALLBACKS *AFTER* ImGui is done
    // ---------------------------------------------------------
    if (deferredCancel) deferredCancel();
    if (deferredConfirm) deferredConfirm(chosenOption);
}
