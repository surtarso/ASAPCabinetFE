#include "utils/logger.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <memory>
#include "utils/logging.h"

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& logFile, bool debugBuild) {
    debugBuild_ = debugBuild;
    
    // Set log path to a dedicated logs directory
    std::filesystem::path logsDir = std::filesystem::path(logFile).parent_path();
    std::filesystem::create_directories(logsDir); // Create logs directory if it doesn't exist

    logFile_.open(logFile, std::ios::out | std::ios::app);
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
    }
    
    info("Logger initialized");
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        info("Logger shutting down");
        logFile_.close();
    }
}

void Logger::log(const std::string& level, const std::string& message) {
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::ctime(&now);
    timestamp = timestamp.substr(0, timestamp.length() - 1); // Remove newline
    
    std::stringstream logMessage;
    logMessage << "[" << timestamp << "] " << level << ": " << message;
    
    if (logFile_.is_open()) {
        logFile_ << logMessage.str() << std::endl;
    }
    
    if (debugBuild_) {
        std::cout << logMessage.str() << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    log("DEBUG", message);
}

void Logger::error(const std::string& message) {
    log("ERROR", message);
}

void Logger::info(const std::string& message) {
    log("INFO", message);
}
