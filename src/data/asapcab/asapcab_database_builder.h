// src/data/asapcab/asapcab_database_builder.h
#pragma once
#include <nlohmann/json.hpp>

namespace data::asapcabdb {
class AsapCabDatabaseBuilder {
public:
    nlohmann::json build(const nlohmann::json& db_vpsdb,
                         const nlohmann::json& db_lbdb,
                         const nlohmann::json& db_vpinmdb,
                         const nlohmann::json& db_ipdb);
};
} // namespace data::asapcabdb
