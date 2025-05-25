/**
 * @file button_handler.h
 * @brief Defines the ButtonHandler class for managing configuration UI buttons in ASAPCabinetFE.
 *
 * This header provides the ButtonHandler class, which handles the rendering and logic
 * of ImGui buttons (e.g., Save, Close) in the configuration UI. It interacts with
 * InputHandler to avoid conflicts during key capture and supports custom callbacks for
 * button actions.
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "config/iconfig_service.h"
#include "config/ui/input_handler.h"
#include <functional>
#include <string>

/**
 * @class App
 * @brief Main application class (forward declaration).
 */
class App;

/**
 * @class ButtonHandler
 * @brief Manages ImGui buttons in the configuration UI.
 *
 * This class renders the button pane in the configuration UI and handles button actions
 * like Save and Close. It tracks the UI visibility and save message timer, and supports
 * custom callbacks for save and close operations, ensuring no conflicts with InputHandler
 * during key capture.
 */
class ButtonHandler {
public:
    /**
     * @brief Constructs a ButtonHandler instance.
     *
     * Initializes the handler with references to the UI visibility flag, save message
     * timer, and InputHandler for conflict checking.
     *
     * @param showConfig Reference to the UI visibility flag.
     * @param saveMessageTimer Reference to the save message timer.
     * @param inputHandler The input handler for checking key capture state.
     */
    ButtonHandler(bool& showConfig, float& saveMessageTimer, const InputHandler& inputHandler);

    /**
     * @brief Renders the button pane.
     *
     * Displays ImGui buttons (e.g., Save, Close) and handles their actions, invoking
     * registered callbacks if set.
     */
    void renderButtonPane();

    /**
     * @brief Sets the callback for the Save button.
     *
     * @param onSave The callback function to invoke when the Save button is clicked.
     */
    void setOnSave(std::function<void()> onSave) { onSave_ = onSave; }

    /**
     * @brief Sets the callback for the Close button.
     *
     * @param onClose The callback function to invoke when the Close button is clicked.
     */
    void setOnClose(std::function<void()> onClose) { onClose_ = onClose; }

private:
    bool& showConfig_;                      ///< Reference to the UI visibility flag.
    float& saveMessageTimer_;               ///< Reference to the save message timer.
    const InputHandler& inputHandler_;      ///< Input handler for checking key capture state.
    std::function<void()> onSave_;          ///< Callback for the Save button action.
    std::function<void()> onClose_;         ///< Callback for the Close button action.
};

#endif // BUTTON_HANDLER_H