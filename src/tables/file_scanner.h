/**
 * @file file_scanner.h
 * @brief Defines the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 *
 * This header provides the FileScanner class, which implements a static method to scan
 * VPX files from a specified directory and construct TableData objects. The scanner
 * supports recursive directory traversal, progress tracking via LoadingProgress, and
 * configurable path resolution for media assets (e.g., images, videos) based on
 * Settings. It can optionally use an existing index to optimize scanning for incremental
 * updates.
 */

#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

#include "table_data.h"
#include "config/settings.h"
#include "core/ui/loading_progress.h"
#include <vector>
#include <fstream>

namespace fs = std::filesystem;

/**
 * @class FileScanner
 * @brief Scans VPX table files and constructs TableData objects.
 *
 * This class provides a static method to recursively scan a directory for VPX files,
 * populate TableData objects with file paths and media asset paths, and track progress
 * if a LoadingProgress object is provided. The scanner can optimize for incremental
 * updates by comparing against an existing index, skipping unchanged files.
 */
class FileScanner {
public:
    /**
     * @brief Scans the VPX tables directory and returns a list of tables.
     *
     * Recursively scans the directory specified in settings.VPXTablesPath for .vpx files,
     * constructs TableData objects with file paths, and resolves media asset paths. If
     * existingTables is provided and settings.forceRebuildMetadata is false, skips files
     * that are unchanged based on fileLastModified and hashes. Updates progress tracking
     * if provided.
     *
     * @param settings The application settings controlling the scan.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @param existingTables Optional vector of existing TableData for incremental scanning.
     * @return A vector of TableData objects representing the scanned tables.
     */
    static std::vector<TableData> scan(const Settings& settings, LoadingProgress* progress = nullptr, 
                                       const std::vector<TableData>* existingTables = nullptr);
private:
    static const std::vector<std::string> MANUFACTURERS_LOWERCASE;
};

#endif // FILE_SCANNER_H