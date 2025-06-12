/**
 * @file vpx_scanner.h
 * @brief Defines the FileScanner class for scanning VPX table files in ASAPCabinetFE.
 *
 * This header provides the FileScanner class, which implements a static method to scan
 * VPX files from a specified directory and construct TableData objects. The scanner
 * supports recursive directory traversal, progress tracking via LoadingProgress, and
 * configurable path resolution for media assets (e.g., images, videos) based on
 * Settings. The process can be customized via settings like VPXTablesPath and
 * forceImagesOnly, with potential for further configUI integration in the future.
 */

#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H // Header guard to prevent multiple inclusions

#include "tables/table_data.h" // Structure for storing table data
#include "config/settings.h" // Configuration settings for path and media options
#include "core/loading_progress.h" // Structure for tracking scanning progress
#include <vector> // For returning a vector of TableData

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file operations

/**
 * @class FileScanner
 * @brief Scans VPX table files and constructs TableData objects.
 *
 * This class provides a static method to recursively scan a directory for VPX files,
 * populate TableData objects with file paths and media asset paths, and track progress
 * if a LoadingProgress object is provided. The scanner uses Settings to determine the
 * base path and media preferences (e.g., images vs. videos), making it configurable
 * with potential for user adjustments via configUI.
 */
class FileScanner {
public:
    /**
     * @brief Scans the VPX tables directory and returns a list of tables.
     *
     * Recursively scans the directory specified in settings.VPXTablesPath for .vpx files,
     * constructs TableData objects with file paths, and resolves media asset paths (e.g.,
     * music, images, videos) based on Settings. If progress is provided, it updates
     * currentTablesLoaded and totalTablesToLoad. The method supports a forceImagesOnly
     * option to disable video paths and can be extended via configUI for custom path
     * configurations.
     *
     * @param settings The application settings controlling the scan (e.g., VPXTablesPath, media paths).
     * @param progress Optional pointer to LoadingProgress for real-time progress updates.
     * @return A vector of TableData objects representing the scanned tables.
     */
    static std::vector<TableData> scan(const Settings& settings, LoadingProgress* progress = nullptr);
private:
    static const std::vector<std::string> MANUFACTURERS_LOWERCASE;
};

#endif // FILE_SCANNER_H