#ifndef METADATA_PANEL_H
#define METADATA_PANEL_H

#include "imgui.h"
#include "tables/table_data.h"
#include "config/settings.h"

class MetadataPanel {
public:
    void render(const TableData& currentTable, int playfieldWidth, int playfieldHeight, const Settings& settings);
};

#endif // METADATA_PANEL_H