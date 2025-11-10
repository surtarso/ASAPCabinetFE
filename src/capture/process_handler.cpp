/**
 * @file process_handler.cpp
 * @brief Implementation of the ProcessHandler class for managing VPX processes.
 */

#include "capture/process_handler.h"
#include "config/iconfig_service.h"
#include "log/logging.h"
#include <signal.h>
#include <unistd.h>
#include <string>

ProcessHandler::ProcessHandler(const std::string& exeDir, IConfigService* configManager)
    : exeDir_(exeDir), configManager_(configManager), vpxPid_(-1) {}

bool ProcessHandler::launchVPX(const std::string& vpxFile) {
    const Settings& settings = configManager_->getSettings();
    std::string logDir = exeDir_ + "logs/";
    std::string vpxLogFile = logDir + "vpx_launch.log";
    std::string mkdirCmd = "mkdir -p " + shellEscape(logDir) + " && rm -f " + vpxLogFile;
    if (std::system(mkdirCmd.c_str()) != 0) {
        LOG_ERROR("Warning: Failed to prepare log directory: " + logDir);
    }

    std::string command = settings.vpxStartArgs + " " + settings.VPinballXPath + " " +
                         settings.vpxPlayCmd + " \"" + vpxFile + "\" " + settings.vpxEndArgs +
                         " > " + vpxLogFile + " 2>&1 & echo $!";
    LOG_DEBUG("Executing VPX Launch command: " + command);

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("Error: Failed to launch VPX process.");
        return false;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        vpxPid_ = std::stoi(buffer);
        LOG_INFO("VPX process launched with PID: " + std::to_string(vpxPid_));
    } else {
        LOG_ERROR("Error: Failed to retrieve VPX PID.");
        pclose(pipe);
        return false;
    }
    pclose(pipe);
    return true;
}

void ProcessHandler::terminateVPX() {
    if (vpxPid_ > 0) {
        LOG_DEBUG("Terminating VPX process with PID: " + std::to_string(vpxPid_));
        if (kill(vpxPid_, SIGTERM) == 0) {
            LOG_DEBUG("SIGTERM sent to VPX process.");
            sleep(1);
            if (kill(vpxPid_, 0) == 0) {
                LOG_DEBUG("VPX still running, sending SIGKILL.");
                kill(vpxPid_, SIGKILL);
            }
        } else {
            LOG_ERROR("Warning: Failed to terminate VPX process with PID: " + std::to_string(vpxPid_));
        }
        vpxPid_ = -1;
    }
}

std::string ProcessHandler::shellEscape(const std::string& str) {
    std::string escaped = "\"";
    for (char c : str) {
        if (c == '"' || c == '\\') escaped += "\\";
        escaped += c;
    }
    escaped += "\"";
    return escaped;
}
