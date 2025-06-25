/**
 * @file logging.h
 * @brief Defines logging macros for ASAPCabinetFE.
 *
 * This header provides macro definitions (LOG_DEBUG, LOG_INFO, LOG_ERROR, LOG_WARN)
 * for logging messages at different levels using a LoggerProxy. The macros add
 * automatic context (function, file, line) and minimize dependencies to reduce
 * recompilation overhead.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <string>

namespace asap::logging {
class LoggerProxy {
public:
    static void debug(const std::string& message, const char* func, const char* file, int line);
    static void info(const std::string& message, const char* func, const char* file, int line);
    static void error(const std::string& message, const char* func, const char* file, int line);
    static void warn(const std::string& message, const char* func, const char* file, int line);
    static bool isDebugEnabled();
};
}

/**
 * @def LOG_DEBUG(msg)
 * @brief Logs a debug message if debug mode is enabled.
 *
 * Adds function, file, and line context and logs via LoggerProxy::debug.
 *
 * @param msg The message to log (string literal or std::string).
 */
#define LOG_DEBUG(msg) do { \
    if (asap::logging::LoggerProxy::isDebugEnabled()) { \
        asap::logging::LoggerProxy::debug(std::string(msg), __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    } \
} while (0)

/**
 * @def LOG_INFO(msg)
 * @brief Logs an info message.
 *
 * Adds function, file, and line context and logs via LoggerProxy::info.
 *
 * @param msg The message to log (string literal or std::string).
 */
#define LOG_INFO(msg) asap::logging::LoggerProxy::info(std::string(msg), __PRETTY_FUNCTION__, __FILE__, __LINE__)

/**
 * @def LOG_ERROR(msg)
 * @brief Logs an error message.
 *
 * Adds function, file, and line context and logs via LoggerProxy::error.
 *
 * @param msg The message to log (string literal or std::string).
 */
#define LOG_ERROR(msg) asap::logging::LoggerProxy::error(std::string(msg), __PRETTY_FUNCTION__, __FILE__, __LINE__)

/**
 * @def LOG_WARN(msg)
 * @brief Logs a warning message.
 *
 * Adds function, file, and line context and logs via LoggerProxy::warn.
 *
 * @param msg The message to log (string literal or std::string).
 */
#define LOG_WARN(msg) asap::logging::LoggerProxy::warn(std::string(msg), __PRETTY_FUNCTION__, __FILE__, __LINE__)

#endif // LOGGING_H