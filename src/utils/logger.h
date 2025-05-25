/**
 * @file logger.h
 * @brief Defines the Logger singleton class for file-based logging in ASAPCabinetFE.
 *
 * This header provides the Logger class, a singleton that manages logging of debug,
 * info, and error messages to a file. It is used by logging macros in logging.h and
 * supports conditional debug logging in debug builds.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <memory>

/**
 * @class Logger
 * @brief Singleton class for logging messages to a file.
 *
 * This class provides a singleton interface for logging debug, info, and error
 * messages to a specified log file. It supports conditional debug logging based on
 * the build configuration and ensures thread-safe initialization. The class is used
 * by logging macros (LOG_DEBUG, LOG_INFO, LOG_ERROR) to record application events.
 */
class Logger {
public:
    /**
     * @brief Gets the singleton Logger instance.
     *
     * Returns a reference to the singleton Logger instance, ensuring thread-safe
     * initialization.
     *
     * @return Reference to the singleton Logger instance.
     */
    static Logger& getInstance();

    /**
     * @brief Initializes the logger with a log file and build configuration.
     *
     * Opens the specified log file for writing and sets the debug build flag to
     * enable or disable debug logging.
     *
     * @param logFile The path to the log file.
     * @param debugBuild True if this is a debug build, false otherwise.
     */
    void initialize(const std::string& logFile, bool debugBuild);
    
    /**
     * @brief Logs a debug message.
     *
     * Writes the message to the log file with a "DEBUG" level if debug logging is
     * enabled (debugBuild_ is true).
     *
     * @param message The debug message to log.
     */
    void debug(const std::string& message);

    /**
     * @brief Logs an error message.
     *
     * Writes the message to the log file with an "ERROR" level.
     *
     * @param message The error message to log.
     */
    void error(const std::string& message);

    /**
     * @brief Logs an info message.
     *
     * Writes the message to the log file with an "INFO" level.
     *
     * @param message The info message to log.
     */
    void info(const std::string& message);

private:
    /**
     * @brief Constructs a Logger instance.
     *
     * Private default constructor to enforce singleton pattern.
     */
    Logger() = default;

    /**
     * @brief Destroys the Logger instance.
     *
     * Closes the log file to release resources.
     */
    ~Logger();

    Logger(const Logger&) = delete;            ///< Deleted copy constructor to prevent copying.
    Logger& operator=(const Logger&) = delete; ///< Deleted assignment operator to prevent assignment.

    std::ofstream logFile_;  ///< Output stream for writing to the log file.
    bool debugBuild_ = false; ///< Flag indicating if debug logging is enabled.

    /**
     * @brief Logs a message with the specified level.
     *
     * Writes the message to the log file, prefixed with the specified level
     * (e.g., "DEBUG", "INFO", "ERROR").
     *
     * @param level The log level (e.g., "DEBUG", "INFO", "ERROR").
     * @param message The message to log.
     */
    void log(const std::string& level, const std::string& message);
};

#endif // LOGGER_H