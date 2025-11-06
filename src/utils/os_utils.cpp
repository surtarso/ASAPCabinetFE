#include "os_utils.h"
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <sys/utsname.h>

namespace OSUtils {

static std::string getValueFromOsRelease(const std::string& key) {
    std::ifstream file("/etc/os-release");
    if (!file.is_open())
        return "unknown";

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind(key + "=", 0) == 0) {
            std::string val = line.substr(key.size() + 1);
            if (!val.empty() && val.front() == '"' && val.back() == '"')
                val = val.substr(1, val.size() - 2);
            return val;
        }
    }
    return "unknown";
}

std::string getDistroId() {
    return getValueFromOsRelease("ID");
}

std::string getDistroLike() {
    return getValueFromOsRelease("ID_LIKE");
}

std::string getSessionType() {
    const char* env = std::getenv("XDG_SESSION_TYPE");
    return env ? env : "unknown";
}

std::string getDesktopEnv() {
    const char* env = std::getenv("XDG_CURRENT_DESKTOP");
    return env ? env : "unknown";
}

bool hasCommand(const std::string& cmd) {
    std::string command = "command -v " + cmd + " >/dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}

std::vector<std::string> getAvailablePackageManagers() {
    std::vector<std::string> result;
    const std::vector<std::string> managers = {
        "apt", "apt-get", "pacman", "dnf", "zypper", "emerge"
    };

    for (const auto& m : managers)
        if (hasCommand(m))
            result.push_back(m);

    return result;
}

std::string getSummary() {
    std::ostringstream ss;

    struct utsname sysinfo{};
    uname(&sysinfo);

    ss << "System Info:\n";
    ss << "  Kernel: " << sysinfo.sysname << " " << sysinfo.release << "\n";
    ss << "  Distro: " << getDistroId();
    std::string like = getDistroLike();
    if (like != "unknown")
        ss << " (like " << like << ")";
    ss << "\n";
    ss << "  Session: " << getSessionType() << "\n";
    ss << "  Desktop: " << getDesktopEnv() << "\n";
    ss << "  Package managers: ";

    auto pms = getAvailablePackageManagers();
    if (pms.empty())
        ss << "none detected";
    else {
        for (size_t i = 0; i < pms.size(); ++i) {
            ss << pms[i];
            if (i < pms.size() - 1)
                ss << ", ";
        }
    }
    ss << "\n";

    return ss.str();
}

} // namespace OSUtils
