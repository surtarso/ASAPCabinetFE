/**
 * @file process_handler.h
 * @brief Defines the ProcessHandler class for managing VPX processes in ASAPCabinetFE.
 *
 * This header provides the ProcessHandler class, which launches and terminates VPX
 * processes for screenshot capture or gameplay. It uses IConfigService for configuration
 * settings and the executable directory for path resolution.
 */

#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#include <string>

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class ProcessHandler
 * @brief Manages launching and terminating VPX processes.
 *
 * This class handles the execution of VPX files (e.g., for screenshot mode or gameplay),
 * tracking the process ID and terminating the process when needed. It integrates with
 * IConfigService for settings and uses the executable directory for path resolution.
 */
class ProcessHandler {
public:
    /**
     * @brief Constructs a ProcessHandler instance.
     *
     * Initializes the handler with the executable directory and configuration service.
     *
     * @param exeDir The executable directory for resolving file paths.
     * @param configManager The configuration service for settings.
     */
    ProcessHandler(const std::string& exeDir, IConfigService* configManager);

    /**
     * @brief Launches a VPX process.
     *
     * Starts a VPX process for the specified VPX file.
     *
     * @param vpxFile The path to the VPX file to launch.
     * @return True if the process launches successfully, false otherwise.
     */
    bool launchVPX(const std::string& vpxFile);

    /**
     * @brief Terminates the running VPX process.
     *
     * Stops the VPX process if it is running.
     */
    void terminateVPX();

    /**
     * @brief Escapes a string for safe shell usage.
     *
     * Escapes special characters in the string to prevent shell injection.
     *
     * @param str The input string to escape.
     * @return The escaped string.
     */
    std::string shellEscape(const std::string& str);

private:
    std::string exeDir_;         ///< Executable directory for resolving file paths.
    IConfigService* configManager_; ///< Configuration service for settings.
    pid_t vpxPid_;               ///< Process ID of the running VPX process.
};

#endif // PROCESS_HANDLER_H