#ifndef METADATA_PANEL_H
#define METADATA_PANEL_H

#include "imgui.h"
#include "tables/table_data.h"
#include "config/settings.h"
#include "utils/media_preview.h"

class MetadataPanel {
public:

    // Frontend (no renderer → no images)
    void render(const TableData& currentTable,
                int playfieldWidth,
                int playfieldHeight,
                const Settings& settings);

    void render(const TableData& currentTable,
            int playfieldWidth,
            int playfieldHeight,
            const Settings& settings,
            SDL_Renderer* uiRenderer);

    // Call when panel is closed (editor only)
    void onClose();

private:
    bool wasOpen_ = false;

};

#endif // METADATA_PANEL_H
