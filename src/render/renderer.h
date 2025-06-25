/**
 * @file renderer.h
 * @brief Defines the Renderer class for rendering VPX table assets in ASAPCabinetFE.
 *
 * This header provides the Renderer class, which implements the IRenderer interface
 * to render Visual Pinball X (VPX) table assets (playfield, backglass, DMD, topper) using
 * SDL renderers. It coordinates with IAssetManager for asset retrieval and IWindowManager
 * for renderer setup.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include "irenderer.h"
#include "ivideo_player.h"
#include "config/settings.h" // Include Settings for passing by const reference
#include <SDL.h>

/**
 * @class Renderer
 * @brief Renders VPX table assets across playfield, backglass, DMD, and topper windows.
 *
 * This class implements the IRenderer interface to manage rendering operations for
 * the playfield, backglass, DMD, and topper displays using dedicated SDL renderers. It fetches
 * assets (textures, videos) from IAssetManager and updates renderers via IWindowManager.
 */
class Renderer : public IRenderer {
public:
    /**
     * @brief Constructs a Renderer instance with specified SDL renderers.
     *
     * Initializes the renderer with separate SDL renderers for playfield, backglass,
     * DMD, and topper displays.
     *
     * @param playfieldRenderer The SDL renderer for the playfield display.
     * @param backglassRenderer The SDL renderer for the backglass display.
     * @param dmdRenderer The SDL renderer for the DMD display.
     * @param topperRenderer The SDL renderer for the topper display.
     */
    Renderer(SDL_Renderer* playfieldRenderer, 
             SDL_Renderer* backglassRenderer,
             SDL_Renderer* dmdRenderer,
             SDL_Renderer* topperRenderer);

    /**
     * @brief Renders assets to all display windows.
     *
     * Fetches textures and video frames from the asset manager and renders them to
     * the playfield, backglass, DMD, and topper windows using their respective renderers.
     *
     * @param assets The asset manager providing textures and video players.
     */
    void render(IAssetManager& assets) override;

    /**
     * @brief Updates the SDL renderers using the window manager.
     *
     * Sets the renderers for playfield, backglass, DMD, and topper displays based on the
     * current window configuration provided by the window manager.
     *
     * @param windowManager The window manager providing access to renderer contexts.
     */
    void setRenderers(IWindowManager* windowManager) override;

private:
    SDL_Renderer* playfieldRenderer_; ///< SDL renderer for the playfield display.
    SDL_Renderer* backglassRenderer_; ///< SDL renderer for the backglass display.
    SDL_Renderer* dmdRenderer_;       ///< SDL renderer for the DMD display.
    SDL_Renderer* topperRenderer_;    ///< SDL renderer for the topper display.

    /**
     * @brief Renders assets to a specific window.
     *
     * Renders the texture or video frame for the specified window, along with optional
     * wheel and title overlays, using the provided renderer and settings.
     *
     * @param assets The asset manager providing textures and video players.
     * @param renderer The SDL renderer for the window.
     * @param windowName The name of the window ("playfield", "backglass", "dmd", "topper").
     * @param isVisible Whether the window is visible (always true for playfield).
     * @param mediaX X-coordinate of the media rectangle.
     * @param mediaY Y-coordinate of the media rectangle.
     * @param mediaWidth Width of the media rectangle.
     * @param mediaHeight Height of the media rectangle.
     * @param rotation Rotation angle for rendering.
     * @param settings Reference to current application settings for rendering options.
     */
    void renderWindow(IAssetManager& assets, SDL_Renderer* renderer, const std::string& windowName,
                      bool isVisible, int mediaX, int mediaY, int mediaWidth, int mediaHeight,
                      double rotation, const Settings& settings);

    // Individual render methods are kept for modularity
    // They will now also pass the Settings object.
    void renderPlayfieldWindow(IAssetManager& assets);
    void renderBackglassWindow(IAssetManager& assets);
    void renderDMDWindow(IAssetManager& assets);
    void renderTopperWindow(IAssetManager& assets);
};

#endif // RENDERER_H