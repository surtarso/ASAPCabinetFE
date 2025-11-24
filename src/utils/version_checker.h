#pragma once
#include <string>
#include <functional>

class VersionChecker {
public:
    using UpdateCallback = std::function<void(const std::string& latestVersion)>;

    VersionChecker(const std::string& currentVersion, const std::string& versionUrl);

    // Returns true if an update is available
    bool checkForUpdate();

    void setUpdateCallback(UpdateCallback cb) { updateCallback_ = cb; }

    const std::string& latestVersion() const { return latestVersion_; }

private:
    std::string currentVersion_;
    std::string versionUrl_;
    std::string latestVersion_;
    UpdateCallback updateCallback_;

    // --- Helpers ---
    std::string normalizeVersion(const std::string& version) const;
};
