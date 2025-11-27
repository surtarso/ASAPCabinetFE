#ifndef IAPP_CALLBACKS_H
#define IAPP_CALLBACKS_H

class ISoundManager; // Forward declaration

/**
 * @brief Interface for handling various application callbacks.
 *
 * The IAppCallbacks interface provides a set of pure virtual methods that allow an
 * application to dynamically update or reload different components at runtime.
 *
 * Methods include:
 * - reloadFont(bool standaloneMode): Reloads the application's font resources.
 * - reloadTablesAndTitle(): Updates the display tables and application title.
 * - reloadOverlaySettings(): Reapplies settings for overlays.
 * - getSoundManager(): Retrieves the ISoundManager instance for real-time updates.
 *
 * A virtual destructor is provided to ensure proper cleanup in derived classes.
 */
class IAppCallbacks {
public:
    virtual void reloadFont(bool standaloneMode) = 0;
    virtual void reloadTablesAndTitle() = 0;
    virtual void reloadOverlaySettings() = 0;
    virtual ISoundManager* getSoundManager() = 0; // Changed to ISoundManager*
    virtual ~IAppCallbacks() = default;
};

#endif // IAPP_CALLBACKS_H
