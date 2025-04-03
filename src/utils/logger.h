#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <memory>

class Logger {
public:
    static Logger& getInstance();
    void initialize(const std::string& logFile, bool debugBuild);
    
    void debug(const std::string& message);
    void error(const std::string& message);
    void info(const std::string& message);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile_;
    bool debugBuild_ = false;
    void log(const std::string& level, const std::string& message);
};

#endif // LOGGER_H
