/**
 * @file vpinmdb_downloader.h
 * @brief Defines the download functionality for VpinMdb media in ASAPCabinetFE.
 *
 * This header provides a function for downloading files using libcurl.
 */

#ifndef VPINMDB_DOWNLOADER_H
#define VPINMDB_DOWNLOADER_H

#include "log/logging.h"
#include <curl/curl.h>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace vpinmdb {

/**
 * @brief Downloads a single file using libcurl.
 * @param url URL to download from.
 * @param destPath Destination file path.
 * @return True if successful, false otherwise.
 */
bool downloadFile(const std::string& url, const fs::path& destPath);

} // namespace vpinmdb

#endif // VPINMDB_DOWNLOADER_H