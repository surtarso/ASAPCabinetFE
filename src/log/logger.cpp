#include <iostream>
#include <ctime>
#include <sstream>
#include <filesystem>
#include "log/logger.h"
#include "log/logging.h"

// ANSI color codes
#define COLOR_RED     "\033[31m"  // For ERROR
#define COLOR_GREEN   "\033[32m"  // For INFO
#define COLOR_ORANGE  "\033[33m"  // For DEBUG
#define COLOR_YELLOW  "\033[38;5;226m"  // For WARN
#define COLOR_RESET   "\033[0m"

Logger::Logger() = default;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& logFile, bool debugBuild) {
    debugBuild_ = debugBuild;
    
    std::filesystem::path logsDir = std::filesystem::path(logFile).parent_path();
    std::filesystem::create_directories(logsDir);

    logFile_.open(logFile, std::ios::out | std::ios::app);
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        return;
    }
    
    info("Logger Initialized.");
}

void Logger::setLoadingProgress(std::shared_ptr<LoadingProgress> progress) {
    loadingProgress_ = progress;
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        info("Shutting down...");
        logFile_.close();
    }
}

void Logger::log(const std::string& level, const std::string& message) {
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::ctime(&now);
    timestamp = timestamp.substr(0, timestamp.length() - 1);
    
    std::stringstream logMessage;
    logMessage << "[" << timestamp << "] " << level << ": " << message;
    
    if (logFile_.is_open()) {
        logFile_ << logMessage.str() << std::endl;
    }
    
    if (loadingProgress_) {
        std::lock_guard<std::mutex> lock(loadingProgress_->mutex);
        loadingProgress_->addLogMessage(level + ": " + message);
    }

    if (level == "INFO" || level == "ERROR" || level == "WARN" || debugBuild_) {
        std::string colorCode;
        if (level == "ERROR") {
            colorCode = COLOR_RED;
        } else if (level == "INFO") {
            colorCode = COLOR_GREEN;
        } else if (level == "DEBUG") {
            colorCode = COLOR_ORANGE;
        } else if (level == "WARN") {
            colorCode = COLOR_YELLOW;
        } else {
            colorCode = COLOR_RESET;
        }
        
        std::cout << colorCode << logMessage.str() << COLOR_RESET << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    if (debugBuild_) {
        log("DEBUG", message);
    }
}

void Logger::error(const std::string& message) {
    log("ERROR", message);
}

void Logger::info(const std::string& message) {
    log("INFO", message);
}

void Logger::warn(const std::string& message) {
    log("WARN", message);
}

bool Logger::isDebugEnabled() const {
    return debugBuild_;
}