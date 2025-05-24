/**
 * @file renderer.h
 * @brief Defines the Renderer class for rendering VPX table assets in ASAPCabinetFE.
 *
 * This header provides the Renderer class, which implements the IRenderer interface
 * to render Visual Pinball X (VPX) table assets (playfield, backglass, DMD) using
 * SDL renderers. It coordinates with IAssetManager for asset retrieval and IWindowManager
 * for renderer setup.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include "render/irenderer.h"
#include <SDL.h>

/**
 * @class Renderer
 * @brief Renders VPX table assets across playfield, backglass, and DMD windows.
 *
 * This class implements the IRenderer interface to manage rendering operations for
 * the playfield, backglass, and DMD displays using dedicated SDL renderers. It fetches
 * assets (textures, videos) from IAssetManager and updates renderers via IWindowManager.
 */
class Renderer : public IRenderer {
public:
    /**
     * @brief Constructs a Renderer instance with specified SDL renderers.
     *
     * Initializes the renderer with separate SDL renderers for playfield, backglass,
     * and DMD displays.
     *
     * @param playfieldRenderer The SDL renderer for the playfield display.
     * @param backglassRenderer The SDL renderer for the backglass display.
     * @param dmdRenderer The SDL renderer for the DMD display.
     */
    Renderer(SDL_Renderer* playfieldRenderer, 
             SDL_Renderer* backglassRenderer,
             SDL_Renderer* dmdRenderer);

    /**
     * @brief Renders assets to all display windows.
     *
     * Fetches textures and video frames from the asset manager and renders them to
     * the playfield, backglass, and DMD windows using their respective renderers.
     *
     * @param assets The asset manager providing textures and video players.
     */
    void render(IAssetManager& assets) override;

    /**
     * @brief Updates the SDL renderers using the window manager.
     *
     * Sets the renderers for playfield, backglass, and DMD displays based on the
     * current window configuration provided by the window manager.
     *
     * @param windowManager The window manager providing access to renderer contexts.
     */
    void setRenderers(IWindowManager* windowManager) override;

private:
    SDL_Renderer* playfieldRenderer_; ///< SDL renderer for the playfield display.
    SDL_Renderer* backglassRenderer_; ///< SDL renderer for the backglass display.
    SDL_Renderer* dmdRenderer_;       ///< SDL renderer for the DMD display.

    /**
     * @brief Renders assets to the playfield window.
     *
     * Renders the playfield texture or video frame from the asset manager to the
     * playfield renderer.
     *
     * @param assets The asset manager providing the playfield texture or video player.
     */
    void renderPlayfieldWindow(IAssetManager& assets);

    /**
     * @brief Renders assets to the backglass window.
     *
     * Renders the backglass texture or video frame from the asset manager to the
     * backglass renderer.
     *
     * @param assets The asset manager providing the backglass texture or video player.
     */
    void renderBackglassWindow(IAssetManager& assets);

    /**
     * @brief Renders assets to the DMD window.
     *
     * Renders the DMD texture or video frame from the asset manager to the DMD renderer.
     *
     * @param assets The asset manager providing the DMD texture or video player.
     */
    void renderDMDWindow(IAssetManager& assets);
};

#endif // RENDERER_H