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

#include "vpsdb_metadata.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>

namespace vpsdb {

class VpsdbCatalog {
public:
    VpsdbCatalog(const std::string& vpsdbFilePath, SDL_Renderer* renderer);
    ~VpsdbCatalog();
    bool render();

private:
    std::string vpsdbFilePath_;
    SDL_Renderer* renderer_;
    std::vector<TableIndex> index_;
    PinballTable currentTable_;
    size_t currentIndex_;
    bool loaded_;
    bool isOpen;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> backglassTexture_;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> playfieldTexture_;
    std::string currentBackglassPath_;
    std::string currentPlayfieldPath_;

    void loadJson();
    void loadTable(size_t index);
    void renderField(const char* key, const std::string& value);
    std::string join(const std::vector<std::string>& vec, const std::string& delim);
    void loadThumbnails();
    void clearThumbnails();
    bool downloadImage(const std::string& url, const std::string& cachePath);
    SDL_Texture* loadTexture(const std::string& path);
    void applySearchFilter(const char* searchTerm);
};

} // namespace vpsdb

#endif // VPSDB_CATALOG_H