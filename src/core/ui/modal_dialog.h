#ifndef CORE_UI_MODAL_DIALOG_H
#define CORE_UI_MODAL_DIALOG_H

#include <string>
#include <vector>
#include <functional>
#include <mutex>

enum class ModalType {
    None,
    Confirm,
    Progress,
    Info,
    Warning,
    Error
};

class ModalDialog {
public:
    ModalDialog();

    // Generic openers
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

    // Draw handler (call once per frame)
    void draw();

    bool isActive() const;

private:
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
    bool pendingOpen_;
};

#endif
