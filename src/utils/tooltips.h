#ifndef TOOLTIPS_H
#define TOOLTIPS_H

#include <string>
#include <unordered_map>

namespace Tooltips {
    inline std::unordered_map<std::string, std::string> getTooltips() {
        return {
            // VPX
            {"TablesPath",
                "Specifies the absolute path to the folder containing VPX table files.\n"
                "\n"
                "It must be a full path.\n"
                "(e.g., /home/user/tables/).\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"},
            {"ExecutableCmd",
                "Defines the absolute path to the VPinballX executable.\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"},
            {"StartArgs",
                "Optional command-line arguments to prepend to the executable.\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"},
            {"EndArgs",
                "Optional arguments to append after the table file in the command.\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"},

            // Custom Media
            {"WheelImage",
                "Relative path to the wheel image for the table.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPTopperImage",
                "Relative path to the PuP topper image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"TableImage",
                "Relative path to the table's preview image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPTableImage",
                "Relative path to the PuP table image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"BackglassImage",
                "Relative path to the backglass image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPBackglassImage",
                "Relative path to the PuP backglass image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"DmdImage",
                "Relative path to the DMD or marquee image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPDmdImage",
                "Relative path to the PuP DMD image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPFullDmdImage",
                "Relative path to the PuP full DMD image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPTopperVideo",
                "Relative path to the PuP topper video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"TableVideo",
                "Relative path to the table preview video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPTableVideo",
                "Relative path to the PuP table video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"BackglassVideo",
                "Relative path to the backglass video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPBackglassVideo",
                "Relative path to the PuP backglass video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"DmdVideo",
                "Relative path to the DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPDmdVideo",
                "Relative path to the PuP DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPFullDmdVideo",
                "Relative path to the PuP full DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},

            // DPI Settings
            {"EnableDpiScaling",
                "Enable automatic DPI scaling based on system settings.\n"
                "When enabled, the frontend will scale according to your monitor's DPI.\n"
                "Disable for manual control via DpiScale."},
            {"DpiScale",
                "Manual DPI scale override.\n"
                "Only used when EnableDpiScaling is false.\n"
                "1.0 = 100%, 1.5 = 150%, etc."},

            // WindowSettings
            {"PlayfieldMonitor",
                "Index of the monitor for the table Playfield window.\n"
                "You can use 'xrandr' to get yours."},
            {"PlayfieldWidth",
                "Width of the main window in pixels.\n"
                "This should be relative to your Playfield media width."},
            {"PlayfieldHeight",
                "Height of the main window in pixels.\n"
                "This should be relative to your Playfield media height."},
            {"BackglassMonitor",
                "Index of the monitor for the Backglass window.\n"
                "You can use 'xrandr' to get yours."},
            {"BackglassWidth",
                "Width of the Backglass window in pixels.\n"
                "This should be relative to your Backglass media width."},
            {"BackglassHeight",
                "Height of the Backglass window in pixels.\n"
                "This should be relative to your Backglass media height."},
            {"DMDMonitor",
                "Index of the monitor for the DMD window.\n"
                "You can use 'xrandr' to get yours."},
            {"DMDWidth",
                "Width of the DMD window in pixels.\n"
                "This should be relative to your DMD media width."},
            {"DMDHeight",
                "Height of the DMD window in pixels.\n"
                "This should be relative to your DMD media height."},

            // TitleDisplay
            {"FontPath",
                "Select a font for the table title display."},
            {"FontColor",
                "Color of the table title display text."},
            {"FontBgColor",
                "Background color behind the table title."},
            {"FontSize",
                "Font size in points for table title text rendering."},
            {"ShowWheel",
                "Toggle visibility of the wheel image in the main window.\n"
                "Set to true to show the wheel, false to hide it."},
            {"ShowTitle",
                "Toggle visibility of table titles in the main window.\n"
                "Set to true to show titles, false to hide them."},

            // MediaDimensions
            {"WheelMediaSize",
                "Size of the wheel image in pixels.\n"
                "This considers a square image."},
            {"WheelMediaMargin",
                "Margin around the wheel image in pixels."},
            {"BackglassMediaWidth",
                "Width of the backglass media in pixels."},
            {"BackglassMediaHeight",
                "Height of the backglass media in pixels."},
            {"DMDMediaWidth",
                "Width of the DMD media in pixels."},
            {"DMDMediaHeight",
                "Height of the DMD media in pixels.\n"
                "This should match your DMD window height."},

            // Default Media
            {"DefaultTableImage",
                "Relative path to the default table preview image.\n"
                "Used when a table has no custom image."},
            {"DefaultBackglassImage",
                "Relative path to the default backglass image.\n"
                "Used when a table has no custom backglass."},
            {"DefaultDmdImage",
                "Relative path to the default DMD/marquee image.\n"
                "Used when a table has no custom DMD image."},
            {"DefaultWheelImage",
                "Relative path to the default wheel image.\n"
                "Used when a table has no custom wheel art."},
            {"DefaultTableVideo",
                "Relative path to the default table preview video.\n"
                "Used when a table has no custom video."},
            {"DefaultBackglassVideo",
                "Relative path to the default backglass video.\n"
                "Used when a table has no custom backglass video."},
            {"DefaultDmdVideo",
                "Relative path to the default DMD video.\n"
                "Used when a table has no custom DMD video."},

            // Keybinds
            {"PreviousTable",
                "Key to select the previous table in the list."},
            {"NextTable",
                "Key to select the next table in the list."},
            {"FastPrevTable",
                "Key to quickly jump back 10 tables."},
            {"FastNextTable",
                "Key to quickly jump forward 10 tables."},
            {"JumpNextLetter",
                "Key to jump to the next table starting with a different letter."},
            {"JumpPrevLetter",
                "Key to jump to the previous table starting with a different letter."},
            {"RandomTable",
                "Key to jump to a random table."},
            {"LaunchTable",
                "Key to launch the selected table."},
            {"ToggleConfig",
                "Key to open or close the configuration menu."},
            {"Quit",
                "Key to exit menus and application."},
            {"ConfigSave",
                "Key to save changes in the configuration menu."},
            {"ConfigClose",
                "Key to close the configuration menu without saving."},
            {"ScreenshotMode",
                "Key to launch a table in screenshot mode."},
            {"ScreenshotKey",
                "Key to take a screenshot while in screenshot mode."},
            {"ScreenshotQuit",
                "Key to quit screenshot mode."},

            // Internal
            {"SubCmd",
                "VPinballX internal command to play .vpx tables.\n"
                "Use VPinballX --help command line menu to see more."},
            {"LogFile", "Path to the debug log file, relative to exec dir."},

            // UI Sounds
            {"ScrollPrevSound", "Sound played when scrolling to previous table."},
            {"ScrollNextSound", "Sound played when scrolling to next table."},
            {"ScrollFastPrevSound", "Sound played when fast scrolling backward."},
            {"ScrollFastNextSound", "Sound played when fast scrolling forward."},
            {"ScrollJumpPrevSound", "Sound played when jumping to previous letter."},
            {"ScrollJumpNextSound", "Sound played when jumping to next letter."},
            {"ScrollRandomSound", "Sound played when selecting a random table."},
            {"LaunchTableSound", "Sound played when launching a table."},
            {"LaunchScreenshotSound", "Sound played when entering screenshot mode."},
            {"ConfigSaveSound", "Sound played when saving configuration."},
            {"ConfigCloseSound", "Sound played when closing configuration."},
            {"QuitSound", "Sound played when quitting the application."},
            {"ScreenshotTakeSound", "Sound played when taking a screenshot."},
            {"ScreenshotQuitSound", "Sound played when exiting screenshot mode."}
        };
    }
}

#endif // TOOLTIPS_H
