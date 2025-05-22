#ifndef IRENDERER_H
#define IRENDERER_H

#include "render/iasset_manager.h"
#include "core/iwindow_manager.h"

/**
 * @brief Interface defining the rendering operations.
 *
 * This interface declares the necessary functions for rendering assets and
 * configuring rendering dependencies.
 *
 * @note The virtual destructor ensures proper cleanup in derived classes.
 *
 * Methods:
 * - render: Must be implemented to render the assets provided by an asset manager.
 * - setRenderers: Configures the renderer with the window manager responsible for rendering contexts.
 *
 * @see IAssetManager, IWindowManager
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /**
     * @brief Renders the object using the provided asset manager.
     *
     * This pure virtual function must be implemented by derived classes to
     * perform the actual rendering. It utilizes the asset manager to access
     * necessary resources such as textures, models, and shaders required for
     * drawing the object.
     *
     * @param assets A reference to an IAssetManager instance for managing assets.
     */
    virtual void render(IAssetManager& assets) = 0;


    /**
     * @brief Sets the window manager for the renderer system.
     *
     * This method provides a way to configure the renderer by assigning an 
     * instance of IWindowManager, which is used to handle window-specific operations 
     * and manage rendering surfaces.
     *
     * @param windowManager A pointer to an IWindowManager instance that controls 
     *                      window management and rendering.
     */
    virtual void setRenderers(IWindowManager* windowManager) = 0;
};

#endif // IRENDERER_H