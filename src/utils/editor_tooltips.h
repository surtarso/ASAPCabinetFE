#pragma once
#include <unordered_map>
#include <string>

namespace Tooltips {
    static const std::unordered_map<std::string, std::string> BUTTON_TOOLTIPS = {
        {"Settings", "Open the Editor Settings panel."},
        {"Exit Editor", "Close the Editor"},
        {"Rescan Tables", "Rescan the table folder and refresh the list.\nUse the dropdown to select scan mode."},
        {"Refresh", "Refresh the current table list."},
        {"Play Selected", "Launch the current selection in VPinballX."},
        {"Extract VBS", "Extract the VBS script from the selected table.\nOpen the script in external editor if already extracted."},
        {"Open Folder", "Open the table folder.\nOpen tables root folder if no table selected."},
        {"View Metadata", "View detailed metadata for the selected table."},
        {"Apply Patch", "Apply community patches to the selected table.\nApply to all tables if none selected."},
        {"Download Media", "Download media (images) for selected table.\nDownloads for all tables if none selected.\n\nMedia files are: Playfield, Backglass, DMD, Wheel (VPinMDB)\nFlyers and Logos (Launchbox DB)"},
        {"Screenshot", "Take a screenshot of the selected table.\nTakes screenshots for all tables if none selected."},
        {"Browse Tables", "Open Virtual Pinball Spreadsheet database browser.\nBrowse and download tables from VPSDB."}
    };
}
