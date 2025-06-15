/**
 * @file vpsdb_catalog.h
 * @brief Header for VpsdbCatalog class to display VPSDB pinball table metadata.
 *
 * Declares the VpsdbCatalog class, which renders a centered, unmovable, unresizable
 * ImGui panel to display one table's metadata and thumbnails from the VPSDB JSON file,
 * with navigation buttons to cycle through tables. Toggled via a showVpsdb_ boolean
 * in the main app render loop.
 */

#ifndef VPSDB_CATALOG_H
#define VPSDB_CATALOG_H

#include "vps_database_client.h"
#include "vpsdb_metadata.h"
#include "config/settings.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>

namespace vpsdb {

class VpsdbCatalog {
public:
    VpsdbCatalog(const std::string& vpsdbFilePath, SDL_Renderer* renderer, const Settings& settings);
    ~VpsdbCatalog();
    bool render();

private:
    std::string vpsdbFilePath_;
    SDL_Renderer* renderer_;
    std::vector<TableIndex> index_;
    PinballTable currentTable_;
    size_t currentIndex_;
    bool loaded_;
    std::atomic<bool> isLoading_;
    std::atomic<int> progressStage_; // 0: Not started, 1: Fetching, 2: Loading JSON, 3: Done
    bool isOpen;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> backglassTexture_;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> playfieldTexture_;
    std::string currentBackglassPath_;
    std::string currentPlayfieldPath_;
    std::unique_ptr<VpsDatabaseClient> vpsDbClient_;
    const Settings& settings_;
    std::thread initThread_;
    std::mutex mutex_;

    void loadJson();
    void loadTable(size_t index);
    void renderField(const char* key, const std::string& value);
    std::string join(const std::vector<std::string>& vec, const std::string& delim);
    void loadThumbnails();
    void clearThumbnails();
    bool downloadImage(const std::string& url, const std::string& cachePath);
    SDL_Texture* loadTexture(const std::string& path);
    void applySearchFilter(const char* searchTerm);
    void initInBackground();
};

} // namespace vpsdb

#endif // VPSDB_CATALOG_H