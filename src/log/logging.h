/**
 * @file logging.h
 * @brief Defines logging macros for ASAPCabinetFE.
 *
 * This header provides macro definitions (LOG_DEBUG, LOG_INFO, LOG_ERROR) for logging
 * messages at different levels using the Logger singleton. The macros handle string
 * streaming and conditional logging based on debug mode.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include "log/logger.h"
#include <sstream>

/**
 * @def LOG_DEBUG(msg)
 * @brief Logs a debug message if debug mode is enabled.
 *
 * Constructs a stringstream to format the message and logs it using Logger::debug
 * only if isDebugEnabled() returns true. Uses a do-while(0) loop for proper scoping.
 *
 * @param msg The message to log, can include stream operators (e.g., <<).
 */
#define LOG_DEBUG(msg) do { \
    if (Logger::getInstance().isDebugEnabled()) { \
        std::stringstream ss; \
        ss << msg; \
        Logger::getInstance().debug(ss.str()); \
    } \
} while (0)

/**
 * @def LOG_INFO(msg)
 * @brief Logs an info message.
 *
 * Constructs a stringstream to format the message and logs it using Logger::info
 * unconditionally. Uses a do-while(0) loop for proper scoping.
 *
 * @param msg The message to log, can include stream operators (e.g., <<).
 */
#define LOG_INFO(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    Logger::getInstance().info(ss.str()); \
} while (0)

/**
 * @def LOG_ERROR(msg)
 * @brief Logs an error message.
 *
 * Constructs a stringstream to format the message and logs it using Logger::error
 * unconditionally. Uses a do-while(0) loop for proper scoping.
 *
 * @param msg The message to log, can include stream operators (e.g., <<).
 */
#define LOG_ERROR(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    Logger::getInstance().error(ss.str()); \
} while (0)

/**
 * @def LOG_WARN(msg)
 * @brief Logs an error message.
 *
 * Constructs a stringstream to format the message and logs it using Logger::error
 * unconditionally. Uses a do-while(0) loop for proper scoping.
 *
 * @param msg The message to log, can include stream operators (e.g., <<).
 */
#define LOG_WARN(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    Logger::getInstance().warn(ss.str()); \
} while (0)

#endif // LOGGING_H