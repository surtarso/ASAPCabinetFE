#ifndef IAPP_CALLBACKS_H
#define IAPP_CALLBACKS_H

/**
 * @brief Interface for handling various application callbacks.
 *
 * The IAppCallbacks interface provides a set of pure virtual methods that allow an
 * application to dynamically update or reload different components at runtime.
 *
 * Methods include:
 * - reloadFont(bool standaloneMode): Reloads the application's font resources.
 * - reloadWindows(): Refreshes the layout or states of any existing windows.
 * - reloadAssetsAndRenderers(): Reloads asset and renderer configurations.
 * - reloadTablesAndTitle(): Updates the display tables and application title.
 * - reloadOverlaySettings(): Reapplies settings for overlays.
 *
 * A virtual destructor is provided to ensure proper cleanup in derived classes.
 */
class IAppCallbacks {
public:
    virtual void reloadFont(bool standaloneMode) = 0;
    virtual void reloadWindows() = 0;
    virtual void reloadAssetsAndRenderers() = 0;
    virtual void reloadTablesAndTitle() = 0;
    virtual void reloadOverlaySettings() = 0;
    virtual ~IAppCallbacks() = default;
};

#endif // IAPP_CALLBACKS_H