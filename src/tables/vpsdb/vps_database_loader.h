#ifndef VPS_DATABASE_LOADER_H
#define VPS_DATABASE_LOADER_H

#include <string>
#include <json.hpp>

class VpsDatabaseLoader {
public:
    VpsDatabaseLoader(const std::string& vpsDbPath);
    bool load();
    const nlohmann::json& getVpsDb() const;

private:
    std::string vpsDbPath_;
    nlohmann::json vpsDb_;
};

#endif // VPS_DATABASE_LOADER_H