#pragma once
#include <string>
#include <vector>

namespace OSUtils {

    // Distro info from /etc/os-release
    std::string getDistroId();
    std::string getDistroLike();

    // Display/session detection
    std::string getSessionType();
    std::string getDesktopEnv();

    // Command and package manager detection
    bool hasCommand(const std::string& cmd);
    std::vector<std::string> getAvailablePackageManagers();

    // Combined info summary
    std::string getSummary();

} // namespace OSUtils
