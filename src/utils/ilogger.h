#ifndef ILOGGER_H
#define ILOGGER_H

#include <string>

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void debug(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
};

#endif // ILOGGER_H
