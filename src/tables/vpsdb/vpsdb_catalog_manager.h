/**
 * @file vpsdb_catalog_manager.h
 * @brief Header for VpsdbCatalog class to display VPSDB pinball table metadata.
 *
 * Declares the VpsdbCatalog class, which renders a centered, unmovable, unresizable
 * ImGui panel to display one table's metadata and thumbnails from the VPSDB JSON file,
 * with navigation buttons to cycle through tables. Toggled via a showVpsdb_ boolean
 * in the main app render loop.
 */

#ifndef VPSDB_CATALOG_MANAGER_H
#define VPSDB_CATALOG_MANAGER_H

#include "vpsdb_metadata.h"
#include "config/settings.h"
#include "vpsdb_catalog_image.h"
#include "vpsdb_catalog_json.h"
#include "vpsdb_catalog_table.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace vpsdb {

class VpsdbCatalog {
    friend class VpsdbImage; // Allow VpsdbImage to access private members

public:
    VpsdbCatalog(SDL_Renderer* renderer, const Settings& settings, VpsdbJsonLoader& jsonLoader);
    ~VpsdbCatalog();
    bool render();

private:
    SDL_Renderer* renderer_;
    PinballTable currentTable_;
    size_t currentIndex_;
    std::atomic<bool> isTableLoading_;
    bool isOpen;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> backglassTexture_;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> playfieldTexture_;
    std::string currentBackglassPath_;
    std::string currentPlayfieldPath_;
    const Settings& settings_;
    VpsdbJsonLoader& jsonLoader_;
    std::thread tableLoadThread_;
    std::mutex mutex_;
    std::queue<LoadedTableData> loadedTableQueue_;

    void renderField(const char* key, const std::string& value);
    std::string join(const std::vector<std::string>& vec, const std::string& delim);
    void applySearchFilter(const char* searchTerm);
    void openUrl(const std::string& url);
    void startTableLoad(size_t index, const std::string& vpsdbImageCacheDir);

    // cache table textures in memory so re-opening a table is instant
    struct CachedTextures {
        SDL_Texture* backglass = nullptr;
        SDL_Texture* playfield = nullptr;
    };
    std::unordered_map<std::string, SDL_Texture*> textureCache_;
};

} // namespace vpsdb

#endif // VPSDB_CATALOG_MANAGER_H
