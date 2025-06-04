#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <memory>
#include "core/loading_progress.h"

class Logger {
public:
    static Logger& getInstance();
    void initialize(const std::string& logFile, bool debugBuild);
    void setLoadingProgress(std::shared_ptr<LoadingProgress> progress);
    void debug(const std::string& message);
    void error(const std::string& message);
    void info(const std::string& message);
    bool isDebugEnabled() const;

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile_;
    bool debugBuild_ = false;
    std::shared_ptr<LoadingProgress> loadingProgress_;

    void log(const std::string& level, const std::string& message);
};

#endif // LOGGER_H