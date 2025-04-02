#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#include <string>

class IConfigService;

class ProcessHandler {
public:
    ProcessHandler(const std::string& exeDir, IConfigService* configManager);
    bool launchVPX(const std::string& vpxFile);
    void terminateVPX();
    std::string shellEscape(const std::string& str);

private:
    std::string exeDir_;
    IConfigService* configManager_;
    pid_t vpxPid_;
};

#endif // PROCESS_HANDLER_H