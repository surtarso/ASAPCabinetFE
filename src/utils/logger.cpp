#include <iostream>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <memory>
#include "utils/logger.h"
#include "utils/logging.h"

// ANSI color codes
#define COLOR_RED     "\033[31m"  // For ERROR
#define COLOR_GREEN   "\033[32m"  // For INFO
#define COLOR_YELLOW  "\033[33m"  // For DEBUG
#define COLOR_RESET   "\033[0m"

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
        return;
    }
    
    debug("Logger: Initialized");
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        info("Shutting down");
        logFile_.close();
    }
}

void Logger::log(const std::string& level, const std::string& message) {
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::ctime(&now);
    timestamp = timestamp.substr(0, timestamp.length() - 1); // Remove newline
    
    std::stringstream logMessage;
    logMessage << "[" << timestamp << "] " << level << ": " << message;
    
    // Always write to file (no color) if open
    if (logFile_.is_open()) {
        logFile_ << logMessage.str() << std::endl;
    }
    
    // Decide when to print to console
    // Only DEBUG messages are conditionally printed based on debugBuild_
    // INFO and ERROR messages are always printed to console.
    if (level == "INFO" || level == "ERROR" || debugBuild_) {
        std::string colorCode;
        if (level == "ERROR") {
            colorCode = COLOR_RED;
        } else if (level == "INFO") {
            colorCode = COLOR_GREEN;
        } else if (level == "DEBUG") {
            colorCode = COLOR_YELLOW;
        } else {
            colorCode = COLOR_RESET; // Default to no color change
        }
        
        std::cout << colorCode << logMessage.str() << COLOR_RESET << std::endl;
    }
}

// And ensure your debug() method still only logs in debug builds:
void Logger::debug(const std::string& message) {
    if (debugBuild_) { // Only log debug messages in debug builds
        log("DEBUG", message);
    }
}

void Logger::error(const std::string& message) {
    log("ERROR", message);
}

void Logger::info(const std::string& message) {
    log("INFO", message);
}