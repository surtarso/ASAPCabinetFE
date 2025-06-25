/**
 * @file logger_proxy.cpp
 * @brief Implements the LoggerProxy class for forwarding log messages in ASAPCabinetFE.
 *
 * This file provides the implementation of LoggerProxy, which forwards log messages
 * to the Logger singleton, adding context (function, file, line) to each message.
 */

#include "log/logging.h"
#include "log/logger.h"
#include <string>
#include <algorithm>
#include <filesystem>

namespace asap::logging {

static std::string formatFunctionName(const std::string& func) {
    std::string result = func;

    // Find the class and function name by locating the last '::' before parameters
    size_t paramStart = result.find('(');
    if (paramStart == std::string::npos) {
        paramStart = result.length();
    }

    size_t classFuncStart = result.rfind("::", paramStart);
    if (classFuncStart == std::string::npos) {
        // No class, just function name; find the last space before function name
        size_t lastSpace = result.rfind(' ', paramStart);
        if (lastSpace != std::string::npos) {
            result = result.substr(lastSpace + 1, paramStart - lastSpace - 1);
        } else {
            result = result.substr(0, paramStart);
        }
    } else {
        // Extract class::function, removing return type and parameters
        size_t returnTypeEnd = result.rfind(' ', classFuncStart);
        if (returnTypeEnd == std::string::npos) {
            returnTypeEnd = 0;
        } else {
            returnTypeEnd++;
        }
        result = result.substr(returnTypeEnd, paramStart - returnTypeEnd);
    }

    // Trim any trailing spaces or qualifiers
    while (!result.empty() && (result.back() == ' ' || result.back() == ':')) {
        result.pop_back();
    }

    return result;
}

static std::string formatFilePath(const std::string& file) {
    // Extract just the filename from the full path
    std::filesystem::path filePath(file);
    return filePath.filename().string();
}

void LoggerProxy::debug(const std::string& message, const char* func, const char* file, int line) {
    if (Logger::getInstance().isDebugEnabled()) {
        std::string formattedFunc = formatFunctionName(func);
        std::string formattedFile = formatFilePath(file);
        std::string msg = formattedFunc + "(" + formattedFile + ":" + std::to_string(line) + ") " + message;
        Logger::getInstance().debug(msg);
    }
}

void LoggerProxy::info(const std::string& message, const char* func, const char* file, int line) {
    std::string msg;
    if (Logger::getInstance().isDebugEnabled()) {
        std::string formattedFunc = formatFunctionName(func);
        std::string formattedFile = formatFilePath(file);
        msg = formattedFunc + "(" + formattedFile + ":" + std::to_string(line) + ") " + message;
    } else {
        msg = message;
    }
    Logger::getInstance().info(msg);
}

void LoggerProxy::error(const std::string& message, const char* func, const char* file, int line) {
    std::string msg;
    if (Logger::getInstance().isDebugEnabled()) {
        std::string formattedFunc = formatFunctionName(func);
        std::string formattedFile = formatFilePath(file);
        msg = formattedFunc + "(" + formattedFile + ":" + std::to_string(line) + ") " + message;
    } else {
        msg = message;
    }
    Logger::getInstance().error(msg);
}

void LoggerProxy::warn(const std::string& message, const char* func, const char* file, int line) {
    std::string msg;
    if (Logger::getInstance().isDebugEnabled()) {
        std::string formattedFunc = formatFunctionName(func);
        std::string formattedFile = formatFilePath(file);
        msg = formattedFunc + "(" + formattedFile + ":" + std::to_string(line) + ") " + message;
    } else {
        msg = message;
    }
    Logger::getInstance().warn(msg);
}

bool LoggerProxy::isDebugEnabled() {
    return Logger::getInstance().isDebugEnabled();
}

} // namespace asap::logging