#include "editor/ui/modal_dialog.h"
#include <imgui.h>
#include <algorithm>

// Stable popup ID used always. Do NOT change at runtime.
static const char* kPopupId = "ModalDialog_Global";

void ModalDialog::reset() {
    // MUST be called with mutex_ held.
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
    visibleFramesRequired_ = 0;
}

ModalDialog::ModalDialog()
    : type_(ModalType::None),
      selectedOption_(0),
      busy_(false),
      completed_(false),
      pendingOpen_(false),
      scrollToBottom_(false),
      visibleFramesRequired_(0) {}

bool ModalDialog::isActive() const {
    std::scoped_lock lock(mutex_);
    return type_ != ModalType::None || pendingOpen_;
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
    pendingOpen_ = true; // request open on next draw() frame
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
    // Ensure user sees the completion at least one frame
    if (!pendingOpen_) pendingOpen_ = true;
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

void ModalDialog::openCommandOutput(const std::string& title) {
    std::scoped_lock lock(mutex_);
    reset();
    type_ = ModalType::CommandOutput;
    title_ = title;
    outputBuffer_.clear();
    pendingOpen_ = true;
    scrollToBottom_ = true;
}

void ModalDialog::appendCommandOutput(const std::string& text) {
    std::scoped_lock lock(mutex_);
    outputBuffer_ += text;
    outputBuffer_ += "\n";
    scrollToBottom_ = true;
}

void ModalDialog::enqueueUiTask(std::function<void()> fn) {
    std::scoped_lock lk(uiTaskMutex_);
    uiTasks_.push_back(std::move(fn));
}

void ModalDialog::requestFinishProgress(const std::string& resultMessage,
                                        const std::string& resultPath) {
    enqueueUiTask([this, resultMessage, resultPath]() {
        this->finishProgress(resultMessage, resultPath);
    });
}

void ModalDialog::draw() {
    // 1) Run queued UI tasks first (on UI/main thread)
    {
        std::vector<std::function<void()>> localTasks;
        {
            std::scoped_lock lk(uiTaskMutex_);
            localTasks.swap(uiTasks_);
        }
        for (auto &t : localTasks) t();
    }

    // Prepare locals copied from protected state so we don't hold mutex while calling ImGui.
    ModalType localType = ModalType::None;
    std::string localTitle;
    std::string localMessage;
    std::vector<std::string> localOptions;
    int localSelectedOption = 0;
    bool localBusy = false;
    bool localCompleted = false;
    std::string localResultPath;
    std::string localOutput;
    bool localScrollToBottom = false;
    int localVisibleFramesRequired = 0;
    bool needOpen = false;

    {
        std::scoped_lock lock(mutex_);
        if (type_ == ModalType::None && !pendingOpen_) {
            return; // nothing to draw
        }
        // Copy state
        localType = type_;
        localTitle = title_;
        localMessage = message_;
        localOptions = options_;
        localSelectedOption = selectedOption_;
        localBusy = busy_;
        localCompleted = completed_;
        localResultPath = resultPath_;
        localOutput = outputBuffer_;
        localScrollToBottom = scrollToBottom_;
        localVisibleFramesRequired = visibleFramesRequired_;
        needOpen = pendingOpen_;

        // Clear the "pendingOpen_" flag here so we don't call OpenPopup() next frame again.
        // We still keep the actual modal state in the object.
        pendingOpen_ = false;
        // IMPORTANT: we do NOT change type_ here.
    }

    // If requested, open the popup now (must happen outside the mutex).
    if (needOpen) {
        // Force focus to ensure the window is drawn on top
        ImGui::SetNextWindowFocus();

        ImGui::OpenPopup(localTitle.c_str());
        ImGui::OpenPopup(kPopupId);
    }

    // Position the popup in center
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 windowPos = ImVec2(
        viewport->Pos.x + viewport->Size.x * 0.5f,
        viewport->Pos.y + viewport->Size.y * 0.5f
    );
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    // Track whether user requested to close during rendering.
    bool requestClose = false;
    std::function<void(std::string)> toCallConfirm = nullptr;
    std::function<void()> toCallCancel = nullptr;
    std::string chosenOption;

    // Window flags for all modal types
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_Modal;

    if (ImGui::BeginPopupModal(kPopupId, nullptr, flags)) {
        // Render content based on the local copy of the modal state.
        float wrapWidth = ImGui::GetFontSize() * 30.0f;
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);

        if (localType == ModalType::Error)
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", localMessage.c_str());
        else if (localType == ModalType::Warning)
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", localMessage.c_str());
        else
            ImGui::TextWrapped("%s", localMessage.c_str());

        ImGui::PopTextWrapPos();

        // OPTIONS (for non-confirm)
        if (localType != ModalType::Confirm && !localOptions.empty()) {
            // Build cstrs on the stack for Combo
            std::vector<const char*> cstrOptions;
            cstrOptions.reserve(localOptions.size());
            for (const auto &o : localOptions) cstrOptions.push_back(o.c_str());
            // Note: we are using a local copy of selected index; if the UI needs the
            // final index reflected back to the stored state, we'll write it out later.
            ImGui::Combo("##options", &localSelectedOption, cstrOptions.data(),
                         static_cast<int>(cstrOptions.size()));
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (localType == ModalType::Confirm) {
            std::string yesLabel = localOptions.size() >= 1 ? localOptions[0] : "Yes";
            std::string noLabel  = localOptions.size() >= 2 ? localOptions[1] : "No";

            const float buttonWidth = 120.0f;
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            const float totalWidth = (buttonWidth * 2.0f) + spacing;
            float regionWidth = ImGui::GetContentRegionAvail().x;
            float offsetX     = (regionWidth - totalWidth) * 0.5f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

            if (ImGui::Button(yesLabel.c_str(), ImVec2(buttonWidth, 0))) {
                chosenOption = yesLabel;
                requestClose = true;
                // We'll execute the confirm callback after the popup is fully closed.
                // Grab the current confirm callback from the protected state below.
            }

            ImGui::SameLine();

            if (ImGui::Button(noLabel.c_str(), ImVec2(buttonWidth, 0))) {
                chosenOption = noLabel;
                requestClose = true;
                // We'll treat "No" as cancel if a cancel callback exists.
            }
        }
        else if (localType == ModalType::Info ||
                 localType == ModalType::Warning ||
                 localType == ModalType::Error) {
            if (ImGui::Button("OK")) {
                requestClose = true;
            }
        }
        else if (localType == ModalType::Progress) {
            if (localBusy) {
                ImGui::TextColored(ImVec4(1,1,0,1), "Processing...");
            } else if (localCompleted) {
                if (localVisibleFramesRequired > 0) {
                    // Ensure at least N frames are visible after completing.
                    // Decrement and keep the popup open this frame.
                    // We need to persist the decremented count back to state.
                    std::scoped_lock lock(mutex_);
                    visibleFramesRequired_ = std::max(0, visibleFramesRequired_ - 1);
                    ImGui::TextColored(ImVec4(0,1,0,1), "%s", message_.c_str());
                    if (!resultPath_.empty())
                        ImGui::TextWrapped("Saved to: %s", resultPath_.c_str());
                    ImGui::EndPopup();
                    // Do not call CloseCurrentPopup() here; just return after EndPopup()
                    return;
                }

                if (localMessage.empty()) {
                    requestClose = true;
                } else {
                    ImGui::TextColored(ImVec4(0,1,0,1), "%s", localMessage.c_str());
                    if (!localResultPath.empty())
                        ImGui::TextWrapped("Saved to: %s", localResultPath.c_str());
                    if (ImGui::Button("OK")) {
                        requestClose = true;
                    }
                }
            } else {
                // shouldn't normally happen, but show message
                ImGui::TextWrapped("%s", localMessage.c_str());
            }
        }
        else if (localType == ModalType::CommandOutput) {
            ImGui::BeginChild("##output_scroll", ImVec2(800, 500), true,
                              ImGuiWindowFlags_AlwaysHorizontalScrollbar);
            ImGui::TextUnformatted(localOutput.c_str());
            if (localScrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();

            if (ImGui::Button("Close")) {
                requestClose = true;
            }
        }

        // End popup content
        ImGui::EndPopup();
    } // end BeginPopupModal scope

    // If the user requested a close action while rendering the popup, perform the close now,
    // but only AFTER EndPopup() so ImGui's internal stack is consistent.
    if (requestClose) {
        // 1) Close the actual ImGui popup (outside the locked render path)
        ImGui::CloseCurrentPopup();

        // 2) Capture/clear callbacks and modal state under lock, and schedule the callback data
        {
            std::scoped_lock lock(mutex_);
            // capture callbacks (move them out), and remember chosen option
            if (onConfirm_) {
                toCallConfirm = onConfirm_;
            }
            if (onCancel_) {
                toCallCancel = onCancel_;
            }

            // If we had a Confirm modal and NO cancel callback, treat "No" as confirming with that label.
            // We'll invoke appropriate callback below using chosenOption.

            // Clear modal state now so a subsequent OpenPopup() works reliably.
            reset();
        }

        // 3) Execute callbacks outside the lock (and after popup closed)
        if (!chosenOption.empty()) {
            // We need to decide whether chosenOption maps to confirm or cancel.
            // If there is a cancel callback and chosenOption equals the second option text, call cancel.
            // Otherwise call confirm with chosenOption.
            bool handled = false;
            if (!localOptions.empty()) {
                if (localOptions.size() >= 2 && chosenOption == localOptions[1]) {
                    // user pressed "No"
                    if (toCallCancel) {
                        toCallCancel();
                        handled = true;
                    }
                }
            }
            if (!handled) {
                if (toCallConfirm) toCallConfirm(chosenOption);
            }
        } else {
            // Non-confirm modal: call cancel/confirm as appropriate.
            if (toCallCancel && localType == ModalType::Confirm) {
                toCallCancel();
            }
        }
    }
}
