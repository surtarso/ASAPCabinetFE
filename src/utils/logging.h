#ifndef LOGGING_H
#define LOGGING_H

#include "utils/logger.h"
#include <sstream>

#ifdef DEBUG_LOGGING
#define LOG_DEBUG(x) do { \
    std::stringstream ss; \
    ss << x; \
    Logger::getInstance().debug(ss.str()); \
} while (0)
#else
#define LOG_DEBUG(x) do { } while (0)
#endif

// Optional: Add convenience macros for info and error
#define LOG_INFO(x) do { \
    std::stringstream ss; \
    ss << x; \
    Logger::getInstance().info(ss.str()); \
} while (0)

#define LOG_ERROR(x) do { \
    std::stringstream ss; \
    ss << x; \
    Logger::getInstance().error(ss.str()); \
} while (0)

#endif // LOGGING_H