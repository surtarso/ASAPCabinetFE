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
    None,       ///< No modal currently active.
    Confirm,    ///< Confirmation dialog (OK / Cancel or custom options).
    Progress,   ///< Progress dialog for ongoing operations.
    Info,       ///< Informational message dialog.
    Warning,    ///< Warning message dialog.
    Error,      ///< Error message dialog.
    CommandOutput ///< Terminal text/output
};

/**
 * @class ModalDialog
 * @brief Thread-safe modal dialog manager using ImGui popups.
 *
 * This class encapsulates all modal behavior (info, error, warning, progress, etc.)
 * and provides a clean interface for other systems (e.g., EditorUI) to display
 * user feedback dialogs without directly interacting with ImGui.
 *
 * Each modal dialog instance tracks its own internal state, allowing safe reuse.
 * Access to internal state is protected by a mutex to allow safe updates from
 * worker threads (e.g., using deferred UI tasks).
 *
 * Usage:
 *  - Call `openInfo()`, `openError()`, etc. to open a modal.
 *  - Call `draw()` once per frame in the main UI thread to render and process events.
 *  - The modal automatically closes itself when dismissed.
 */
class ModalDialog {
public:
    /**
     * @brief Constructs an empty, inactive modal dialog.
     */
    ModalDialog();

    // ---------------------------------------------------------------------
    // Modal Openers
    // ---------------------------------------------------------------------

    /**
     * @brief Opens a confirmation dialog with custom options.
     *
     * @param title        Dialog window title.
     * @param message      Dialog body text.
     * @param options      List of selectable options (e.g. "Yes", "No").
     * @param onConfirm    Callback invoked when the user confirms.
     * @param onCancel     Optional callback for cancel action.
     */
    void openConfirm(const std::string& title,
                     const std::string& message,
                     const std::vector<std::string>& options,
                     std::function<void(const std::string&)> onConfirm,
                     std::function<void()> onCancel = nullptr);

    /**
     * @brief Opens a progress dialog to indicate an ongoing operation.
     *
     * @param title    Dialog window title.
     * @param message  Initial progress message.
     */
    void openProgress(const std::string& title,
                      const std::string& message);

    /**
     * @brief Updates the displayed message of the current progress dialog.
     * @param message New progress text.
     */
    void updateProgress(const std::string& message);

    /**
     * @brief Marks the progress dialog as completed, showing a result message.
     *
     * @param resultMessage Final message after completion.
     * @param resultPath    Optional file or output path to display.
     */
    void finishProgress(const std::string& resultMessage,
                        const std::string& resultPath = "");

    /**
     * @brief Opens a simple informational modal dialog.
     *
     * @param title   Dialog window title.
     * @param message Message to display.
     */
    void openInfo(const std::string& title,
                  const std::string& message);

    /**
     * @brief Opens a warning modal dialog.
     *
     * @param title   Dialog window title.
     * @param message Message to display.
     */
    void openWarning(const std::string& title,
                     const std::string& message);

    /**
     * @brief Opens an error modal dialog.
     *
     * @param title   Dialog window title.
     * @param message Message to display.
     */
    void openError(const std::string& title,
                   const std::string& message);

    void openCommandOutput(const std::string& title) {
        std::scoped_lock lock(mutex_);
        type_ = ModalType::CommandOutput;
        title_ = title;
        outputBuffer_.clear();
        pendingOpen_ = true;
        scrollToBottom_ = true;
    }

    void appendCommandOutput(const std::string& text) {
        std::scoped_lock lock(mutex_);
        outputBuffer_ += text + "\n";
        scrollToBottom_ = true;
    }


    // ---------------------------------------------------------------------
    // Rendering and State
    // ---------------------------------------------------------------------

    /**
     * @brief Renders the modal if active. Must be called once per frame.
     *
     * Should be called from the main ImGui thread after ImGui::NewFrame().
     */
    void draw();

    /**
     * @brief Checks if a modal dialog is currently active.
     * @return True if a modal is open or pending.
     */
    bool isActive() const;

private:
    // ---------------------------------------------------------------------
    // Internal State
    // ---------------------------------------------------------------------

    mutable std::mutex mutex_;                        ///< Protects modal state for thread-safe operations.
    ModalType type_;                                  ///< Current modal type.
    std::string title_;                               ///< Modal window title.
    std::string message_;                             ///< Main message body text.
    std::vector<std::string> options_;                ///< Selectable options for confirmation modals.
    int selectedOption_;                              ///< Index of currently selected option in combo box.
    std::function<void(const std::string&)> onConfirm_; ///< Confirm callback function.
    std::function<void()> onCancel_;                  ///< Cancel callback function.
    bool busy_;                                       ///< True if progress is active.
    bool completed_;                                  ///< True if progress operation is finished.
    std::string resultPath_;                          ///< Optional path displayed after completion.
    bool pendingOpen_;                                ///< True if ImGui::OpenPopup() should be triggered next frame.
    std::string outputBuffer_;        // Stores command output text
    bool scrollToBottom_ = false;    // Scroll helper
};

#endif // CORE_UI_MODAL_DIALOG_H
