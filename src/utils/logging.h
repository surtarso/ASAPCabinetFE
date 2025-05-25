/**
 * @file logging.h
 * @brief Defines logging macros for ASAPCabinetFE.
 *
 * This header provides logging macros (LOG_DEBUG, LOG_INFO, LOG_ERROR) that interface
 * with the Logger singleton to log messages at different severity levels. Debug logging
 * is enabled only when DEBUG_LOGGING is defined.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include "utils/logger.h"
#include <sstream>

/**
 * @def LOG_DEBUG(x)
 * @brief Logs a debug message if DEBUG_LOGGING is defined.
 *
 * Converts the input expression to a string using a stringstream and logs it as a
 * debug message via Logger::debug. No-op if DEBUG_LOGGING is not defined.
 *
 * @param x The expression to log (e.g., a string or streamable object).
 */
#ifdef DEBUG_LOGGING
#define LOG_DEBUG(x) do { \
    std::stringstream ss; \
    ss << x; \
    Logger::getInstance().debug(ss.str()); \
} while (0)
#else
#define LOG_DEBUG(x) do { } while (0)
#endif

/**
 * @def LOG_INFO(x)
 * @brief Logs an info message.
 *
 * Converts the input expression to a string using a stringstream and logs it as an
 * info message via Logger::info.
 *
 * @param x The expression to log (e.g., a string or streamable object).
 */
#define LOG_INFO(x) do { \
    std::stringstream ss; \
    ss << x; \
    Logger::getInstance().info(ss.str()); \
} while (0)

/**
 * @def LOG_ERROR(x)
 * @brief Logs an error message.
 *
 * Converts the input expression to a string using a stringstream and logs it as an
 * error message via Logger::error.
 *
 * @param x The expression to log (e.g., a string or streamable object).
 */
#define LOG_ERROR(x) do { \
    std::stringstream ss; \
    ss << x; \
    Logger::getInstance().error(ss.str()); \
} while (0)

#endif // LOGGING_H