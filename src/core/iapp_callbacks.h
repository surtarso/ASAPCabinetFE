#ifndef IAPP_CALLBACKS_H
#define IAPP_CALLBACKS_H

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