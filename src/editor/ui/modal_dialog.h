#ifndef CORE_UI_MODAL_DIALOG_H
#define CORE_UI_MODAL_DIALOG_H

#include <string>
#include <vector>
#include <functional>
#include <mutex>

/**
 * @brief Modal dialog types for user interaction and feedback.
 */
enum class ModalType {
    None,
    Confirm,
    Progress,
    Info,
    Warning,
    Error,
    CommandOutput
};

class ModalDialog {
public:
    ModalDialog();

    // Openers
    void openConfirm(const std::string& title,
                     const std::string& message,
                     const std::vector<std::string>& options,
                     std::function<void(const std::string&)> onConfirm,
                     std::function<void()> onCancel = nullptr);

    void openProgress(const std::string& title,
                      const std::string& message);

    void updateProgress(const std::string& message);
    void finishProgress(const std::string& resultMessage,
                        const std::string& resultPath = "");

    void openInfo(const std::string& title,
                  const std::string& message);
    void openWarning(const std::string& title,
                     const std::string& message);
    void openError(const std::string& title,
                   const std::string& message);

    void openCommandOutput(const std::string& title);
    void appendCommandOutput(const std::string& text);

    // Rendering & tasks
    void draw();
    bool isActive() const;
    void enqueueUiTask(std::function<void()> fn);
    void requestFinishProgress(const std::string& resultMessage,
                               const std::string& resultPath = "");

private:
    void reset(); // MUST be called while holding mutex

    // UI task queue (thread-safe)
    mutable std::mutex uiTaskMutex_;
    std::vector<std::function<void()>> uiTasks_;

    // Modal internal state (thread-protected)
    mutable std::mutex mutex_;
    ModalType type_;
    std::string title_;
    std::string message_;
    std::vector<std::string> options_;
    int selectedOption_;
    std::function<void(const std::string&)> onConfirm_;
    std::function<void()> onCancel_;
    bool busy_;
    bool completed_;
    std::string resultPath_;
    bool pendingOpen_;      // request to OpenPopup() on this frame
    std::string outputBuffer_;
    bool scrollToBottom_;
    int visibleFramesRequired_;

    // NOTE: no public pendingClose_ here — handled internally in draw()
};

#endif // CORE_UI_MODAL_DIALOG_H
