#ifndef VPSDB_CATALOG_IMAGE_H
#define VPSDB_CATALOG_IMAGE_H

#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace vpsdb {

class VpsdbCatalog; // Forward declaration

class VpsdbImage {
public:
    static void loadThumbnails(VpsdbCatalog& catalog);
    static void clearThumbnails(VpsdbCatalog& catalog);
    static bool downloadImage(const std::string& url, const std::string& cachePath);
    static SDL_Texture* loadTexture(VpsdbCatalog& catalog, const std::string& path);
};

} // namespace vpsdb

#endif // VPSDB_CATALOG_IMAGE_H