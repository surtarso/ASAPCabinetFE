#ifndef METADATA_PANEL_H
#define METADATA_PANEL_H

#include "imgui.h"
#include "data/table_data.h"
#include "config/settings.h"
#include "utils/media_preview.h"
#include "sound/isound_manager.h"  // ← include ISoundManager

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

    // // Call when panel is closed (editor only)
    // void onClose();

    // Setter for sound manager
    void setSoundManager(ISoundManager* sm) { soundManager_ = sm; }

private:
    bool wasOpen_ = false;
    ISoundManager* soundManager_ = nullptr;  // ← pointer to the sound manager
};

#endif // METADATA_PANEL_H
