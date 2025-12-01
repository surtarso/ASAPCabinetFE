/**
 * @file vpsdb_metadata.h
 * @brief Data structures for parsing VPSDB JSON in ASAPCabinetFE.
 *
 * Defines structs for pinball table metadata, including nested fields like tableFiles,
 * b2sFiles, and wheelArtFiles, to match the VPSDB JSON schema.
 */

#ifndef VPSDB_METADATA_H
#define VPSDB_METADATA_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace vpsdb {

struct Url {
    std::string url;
    bool broken;
};

struct TableFile {
    std::string id;
    int64_t createdAt;
    int64_t updatedAt;
    std::vector<Url> urls;
    std::vector<std::string> authors;
    std::vector<std::string> features;
    std::string tableFormat;
    std::string comment;
    std::string version;
    std::string imgUrl;
};

struct TopperFile {
    std::string id;
    int64_t createdAt;
    int64_t updatedAt;
    std::vector<Url> urls;
    std::vector<std::string> authors;
    std::string version;
};

struct PinballTable {
    std::string id;
    int64_t updatedAt;
    std::string manufacturer;
    std::string name;
    int year;
    std::vector<std::string> theme;
    std::vector<std::string> designers;
    std::string type;
    int players;
    std::string ipdbUrl;
    std::vector<TableFile> tableFiles;
    std::vector<TableFile> b2sFiles;
    std::vector<TableFile> wheelArtFiles;
    std::vector<TopperFile> topperFiles;
    int64_t lastCreatedAt;
};

struct TableIndex {
    std::string id;
    std::string name;
    std::string manufacturer;
    int year;
};

} // namespace vpsdb

#endif // VPSDB_METADATA_H