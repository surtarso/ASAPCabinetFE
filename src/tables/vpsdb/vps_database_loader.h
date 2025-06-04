#ifndef VPS_DATABASE_LOADER_H
#define VPS_DATABASE_LOADER_H

#include <string>
#include <json.hpp>
#include "core/loading_progress.h"

class VpsDatabaseLoader {
public:
    VpsDatabaseLoader(const std::string& vpsDbPath);
    bool load(LoadingProgress* progress = nullptr);
    const nlohmann::json& getVpsDb() const;

private:
    std::string vpsDbPath_;
    nlohmann::json vpsDb_;
};

#endif // VPS_DATABASE_LOADER_H