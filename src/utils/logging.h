#ifndef LOGGING_H
#define LOGGING_H

#include "utils/logger.h"
#include <sstream>

#define LOG_DEBUG(msg) do { \
    if (Logger::getInstance().isDebugEnabled()) { \
        std::stringstream ss; \
        ss << msg; \
        Logger::getInstance().debug(ss.str()); \
    } \
} while (0)

#define LOG_INFO(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    Logger::getInstance().info(ss.str()); \
} while (0)

#define LOG_ERROR(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    Logger::getInstance().error(ss.str()); \
} while (0)

#endif // LOGGING_H