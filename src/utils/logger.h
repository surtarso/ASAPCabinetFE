#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <memory>
#include "core/loading_progress.h"

/**
 * @file logger.h
 * @brief Defines the Logger class for logging in ASAPCabinetFE.
 *
 * This header provides the Logger singleton class, which handles logging to a file,
 * console with color coding, and integration with LoadingProgress. It supports
 * debug, info, and error levels with conditional logging based on build mode.
 */

/**
 * @class Logger
 * @brief Singleton class for logging messages in ASAPCabinetFE.
 *
 * The Logger provides a centralized logging mechanism with file output, console
 * output (with ANSI colors), and integration with loading progress updates. It
 * uses RAII for file management and ensures thread safety for progress updates.
 */
class Logger {
public:
    /**
     * @brief Gets the singleton instance of Logger.
     *
     * @return Reference to the singleton Logger instance.
     */
    static Logger& getInstance();

    /**
     * @brief Initializes the logger with a log file path and debug mode.
     *
     * Creates the log directory if it doesn’t exist and opens the log file.
     *
     * @param logFile The path to the log file.
     * @param debugBuild Flag indicating if debug logging is enabled.
     */
    void initialize(const std::string& logFile, bool debugBuild);

    /**
     * @brief Sets the loading progress object for logging integration.
     *
     * @param progress Shared pointer to the LoadingProgress instance.
     */
    void setLoadingProgress(std::shared_ptr<LoadingProgress> progress);

    /**
     * @brief Logs a debug message.
     *
     * Logs only if debug mode is enabled.
     *
     * @param message The debug message to log.
     */
    void debug(const std::string& message);

    /**
     * @brief Logs an error message.
     *
     * Logs unconditionally with error level.
     *
     * @param message The error message to log.
     */
    void error(const std::string& message);

    /**
     * @brief Logs an info message.
     *
     * Logs unconditionally with info level.
     *
     * @param message The info message to log.
     */
    void info(const std::string& message);

    /**
     * @brief Checks if debug logging is enabled.
     *
     * @return True if debug mode is enabled, false otherwise.
     */
    bool isDebugEnabled() const;

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile_;         ///< File stream for log output.
    bool debugBuild_ = false;       ///< Flag indicating debug mode.
    std::shared_ptr<LoadingProgress> loadingProgress_; ///< Shared pointer to loading progress.

    void log(const std::string& level, const std::string& message);
};

#endif // LOGGER_H