#ifndef DMD_RENDERER_H
#define DMD_RENDERER_H

#include <SDL.h>
#include <string>
#include <map>
#include <array>
#include <algorithm>
#include <memory>
#include <cstdint>

// Custom deleter for SDL_Texture using a type alias for cleaner code
using SdlTexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

/**
 * @brief Handles procedural DMD rendering and manages a cache of still/animated image assets.
 */
class DmdSDLRenderer {
public:
    DmdSDLRenderer();
    ~DmdSDLRenderer();

    /**
     * @brief Loads assets (images and GIFs) from a directory into the internal cache.
     * @param directoryPath The path to the asset directory (e.g., "assets/img/dmd_still").
     * @param renderer The SDL renderer used to create textures.
     */
    void loadAssetsFromDirectory(const std::string& directoryPath, SDL_Renderer* renderer);

    /**
     * @brief Retrieves a cached texture asset by its filename.
     * @param name The filename of the asset (e.g., "bally.png").
     * @return The raw SDL_Texture pointer, or nullptr if not found.
     */
    SDL_Texture* getAsset(const std::string& name) const;

    /**
     * @brief Renders the DMD content, prioritizing cached assets over procedural text.
     * @param renderer The target SDL renderer.
     * @param displayText The string to display (Manufacturer name for asset lookup).
     * @param width The screen width.
     * @param height The screen height.
     * @param time Accumulated time for animation/flicker effects.
     * @param defaultText Fallback text if asset fails or is not found.
     */
    void render(SDL_Renderer* renderer, const std::string& displayText, int width, int height, float time, std::string defaultText);

private:
    /**
     * @brief Renders the fallback procedural dot-matrix text.
     */
    void renderProceduralText(SDL_Renderer* renderer, const std::string& textToDisplay, int width, int height, float time);

    void drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius);
    void drawDmdChar(SDL_Renderer* renderer, char c, int startX, int startY, int dotRadius, int pixelPerDot, SDL_Color color, Uint8 glowAlpha);

    /**
     * @brief Renders the asset texture using a dot-matrix mask effect, preserving the DMD style.
     * @param assetTexture The loaded image/GIF frame to use as a mask.
     * @details This function includes the unlit background dots.
     */
    void drawDmdAssetMasked(SDL_Renderer* renderer, SDL_Texture* assetTexture, int width, int height, float time);

    // Asset Cache: Filename -> {Texture, isAnimated}
    // We use a raw pointer for the renderer to avoid circular dependency in the header.
    std::map<std::string, std::pair<SdlTexturePtr, bool>> assetCache_;

    // --- DMD Text Rendering Constants/Members ---

    // 5x9 font bitmap data (Declared here, Defined in the CPP file)
    static const std::map<char, std::array<uint16_t, 5>> DMD_FONT;

    // Use static constexpr for simple constants (safe in header)
    static constexpr int CHAR_HEIGHT_DOTS = 9;
};

#endif // DMD_RENDERER_H
