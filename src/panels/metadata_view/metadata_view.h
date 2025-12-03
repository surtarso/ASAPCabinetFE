#ifndef METADATA_VIEW_H
#define METADATA_VIEW_H

#include "config/settings.h"
#include "data/table_data.h"
#include "imgui.h"
#include "sound/isound_manager.h"
#include "utils/media_preview.h"

class MetadataView
{
public:
    // Editor
    void render(const TableData &currentTable,
                int editorWidth,
                int editorHeight,
                const Settings &settings,
                SDL_Renderer *uiRenderer);

    void setSoundManager(ISoundManager *sm) { soundManager_ = sm; }

private:
    bool wasOpen_ = false;
    ISoundManager *soundManager_ = nullptr;
};

#endif // METADATA_VIEW_H
