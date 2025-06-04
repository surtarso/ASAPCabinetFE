#ifndef TOOLTIPS_H
#define TOOLTIPS_H

#include <string>
#include <unordered_map>

/// @brief Generates tooltips for the config ui
namespace Tooltips {
    inline const std::unordered_map<std::string, std::string>& getTooltips() {
        static const std::unordered_map<std::string, std::string> tooltips = {
            // VPX
            {"VPXTablesPath",
                "Defines the absolute path to the folder containing VPX table files.\n"
                "\n"
                "It must be a full path.\n"
                "(e.g., /home/user/tables/).\n"
                "\n"
                "Final command:\n"
                "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"},
            {"VPinballXPath",
                "Defines the absolute path to the VPinballX executable.\n"
                "\n"
                "Final command:\n"
                "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"},
            {"VPXIniPath",
                "Defines the absolute path to the VPinballX.ini file\n"
                "If left empty it will search the default location\n"
                "~/.vpinball/VPinballX.ini"},
            {"StartArgs",
                "Optional command-line arguments to prepend to the executable.\n"
                "\n"
                "Final command:\n"
                "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"},
            {"EndArgs",
                "Optional arguments to append after the table file in the command.\n"
                "\n"
                "Final command:\n"
                "StartArgs VPinballXPath -play VPXTablesPath/<selectedtable>.vpx EndArgs"},

            // Custom Media
            {"WheelImage",
                "Relative path to the wheel image for the table.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"TopperImage",
                "Relative path to the topper image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PlayfieldImage",
                "Relative path to the table's preview image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"BackglassImage",
                "Relative path to the backglass image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"DmdImage",
                "Relative path to the DMD image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"TopperVideo",
                "Relative path to the topper video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PlayfieldVideo",
                "Relative path to the table preview video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"BackglassVideo",
                "Relative path to the backglass video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"DmdVideo",
                "Relative path to the DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"TableMusic",
                "Relative path to the table music file.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"CustomLaunchSound",
                "Relative path to the table launch audio file.\n"
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
            {"VideoBackend",
                "Select video playback backend:\n"
                "VLC: Reliable software-based playback with broad format support. Ideal for compatibility.\n"
                "FFmpeg: Efficient software-based playback with extensive codec support. Suitable for most systems.\n"
                "GStreamer: Flexible playback with plugin-based architecture. Good for customized setups."},
            {"UseVPinballXIni",
                "Uses sizes and positions from ~/.vpinball/VPinballX.ini\n"
                "Using this options will override options bellow."},

            {"PlayfieldWidth",
                "Width of the Playfield window in pixels.\n"
                "This should be relative to your Playfield media width."},
            {"PlayfieldHeight",
                "Height of the Playfield window in pixels.\n"
                "This should be relative to your Playfield media height."},
            {"PlayfieldX",
                "X position of the Playfield window.\n"
                "You can drag and double-click a window to save it's position"},
            {"PlayfieldY",
                "Y position of the Playfield window.\n"
                "You can drag and double-click a window to save it's position"},
            
            {"ShowBackglass",
                "Show/hide the backglass window."},
            {"BackglassWidth",
                "Width of the Backglass window in pixels.\n"
                "This should be relative to your Backglass media width."},
            {"BackglassHeight",
                "Height of the Backglass window in pixels.\n"
                "This should be relative to your Backglass media height."},
            {"BackglassX",
                "X position of the Backglass window.\n"
                "You can drag and double-click a window to save it's position"},
            {"BackglassY",
                "Y position of the Backglass window.\n"
                "You can drag and double-click a window to save it's position"},
            
            {"ShowDMD",
                "Show/hide the DMD window."},
            {"DMDWidth",
                "Width of the DMD window in pixels.\n"
                "This should be relative to your DMD media width."},
            {"DMDHeight",
                "Height of the DMD window in pixels.\n"
                "This should be relative to your DMD media height."},
            {"DMDX",
                "X position of the DMD window.\n"
                "You can drag and double-click a window to save it's position"},
            {"DMDY",
                "Y position of the DMD window.\n"
                "You can drag and double-click a window to save it's position"},

            {"ShowTopper",
                "Show/hide the Topper window."},
            {"TopperWidth",
                "Width of the Topper window in pixels.\n"
                "This should be relative to your Topper media width."},
            {"TopperHeight",
                "Height of the Topper window in pixels.\n"
                "This should be relative to your Topper media height."},
            {"TopperX",
                "X position of the Topper window.\n"
                "You can drag and double-click a window to save it's position"},
            {"TopperY",
                "Y position of the Topper window.\n"
                "You can drag and double-click a window to save it's position"},

            // TitleDisplay
            {"TitleSource",
                "Select the source of the title info.\n"
                "- 'filename' to use filename as table title.\n"
                "- 'metadata' to use table title from metadata. (requires vpxtool)\n"},
            {"FetchVPSdb",
                "Fetches Virtual Pinball Spreadsheet database and\n"
                "attempts to match with vpxtool metadata."},
            {"ForceRebuild",
                "Forces re-building metadata from scratch.\n"
                "Applying will DELETE asapcabinetfe_index.json"},
            {"ShowMetadata",
                "Show/hide the metadata panel overlay on the playfield window. (requires vpxtool)\n"
                "TitleSource must be set to 'metadata' for the panel to display something."},
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
            {"WheelWindow",
                "Select the window to display the wheel art."},
            {"ShowTitle",
                "Toggle visibility of table titles in the main window.\n"
                "Set to true to show titles, false to hide them."},
            {"TitleWindow",
                "Select the window to display the table title."},
            {"TitleX",
                "X position of the table title"},
            {"TitleY",
                "Y position of the table title"},

            // MediaDimensions
            {"ForceImagesOnly", 
                "Use only images (skip videos)."},

            {"WheelMediaHeight",
                "Height of the wheel image in pixels."},
            {"WheelMediaWidth",
                "Width of the wheel image in pixels."},
            {"WheelMediaX",
                "X position of the wheel image."},
            {"WheelMediaY",
                "Y position of the wheel image."},

            {"PlayfieldMediaWidth",
                "Width of the playfield media in pixels."},
            {"PlayfieldMediaHeight",
                "Height of the playfield media in pixels."},
            {"PlayfieldMediaX",
                "X position of the playfield media.\n"
                "This position is relative to the playfield window."},
            {"PlayfieldMediaY",
                "Y position of the playfield media.\n"
                "This position is relative to the playfield window."},
            {"PlayfieldRotation",
                "Rotation of the Playfield media.\n"
                "0 = no rotation\n"
                "90, 180, 270, -90, etc"},
            
            {"BackglassMediaWidth",
                "Width of the backglass media in pixels."},
            {"BackglassMediaHeight",
                "Height of the backglass media in pixels."},
            {"BackglassMediaX",
                "X position of the backglass media.\n"
                "This position is relative to the backglass window."},
            {"BackglassMediaY",
                "Y position of the backglass media.\n"
                "This position is relative to the backglass window."},
            {"BackglassRotation",
                "Rotation of the Backglass media.\n"
                "0 = no rotation\n"
                "90, 180, 270, -90, etc"},

            {"DMDMediaWidth",
                "Width of the DMD media in pixels."},
            {"DMDMediaHeight",
                "Height of the DMD media in pixels.\n"
                "This should match your DMD window height."},
            {"DMDMediaX",
                "X position of the DMD media.\n"
                "This position is relative to the DMD window."},
            {"DMDMediaY",
                "Y position of the DMD media.\n"
                "This position is relative to the DMD window."},
            {"DMDRotation",
                "Rotation of the DMD media.\n"
                "0 = no rotation\n"
                "90, 180, 270, -90, etc"},

            {"TopperMediaWidth",
                "Width of the Topper media in pixels."},
            {"TopperMediaHeight",
                "Height of the Topper media in pixels.\n"
                "This should match your Topper window height."},
            {"TopperMediaX",
                "X position of the Topper media.\n"
                "This position is relative to the Topper window."},
            {"TopperMediaY",
                "Y position of the Topper media.\n"
                "This position is relative to the Topper window."},
            {"TopperRotation",
                "Rotation of the Topper media.\n"
                "0 = no rotation\n"
                "90, 180, 270, -90, etc"},

            // Default Media
            {"DefaultPlayfieldImage",
                "Relative path to the default table preview image.\n"
                "Used when a table has no custom image."},
            {"DefaultBackglassImage",
                "Relative path to the default backglass image.\n"
                "Used when a table has no custom backglass."},
            {"DefaultDmdImage",
                "Relative path to the default DMD image.\n"
                "Used when a table has no custom DMD image."},
            {"DefaultWheelImage",
                "Relative path to the default wheel image.\n"
                "Used when a table has no custom wheel art."},
            {"DefaultTopperImage",
                "Relative path to the default Topper image.\n"
                "Used when a table has no custom Topper art."},

            {"DefaultPlayfieldVideo",
                "Relative path to the default table preview video.\n"
                "Used when a table has no custom video."},
            {"DefaultBackglassVideo",
                "Relative path to the default backglass video.\n"
                "Used when a table has no custom backglass video."},
            {"DefaultDmdVideo",
                "Relative path to the default DMD video.\n"
                "Used when a table has no custom DMD video."},
            {"DefaultTopperVideo",
                "Relative path to the default Topper video.\n"
                "Used when a table has no custom Topper video."},

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
            {"ConfigClose",
                "Key to close the configuration menu without saving."},
            {"ScreenshotMode",
                "Key to launch a table in screenshot mode."},
            {"ScreenshotKey",
                "Key to take a screenshot while in screenshot mode."},
            {"ScreenshotQuit",
                "Key to quit screenshot mode."},
            
            // Audio Settings
            // {"MasterMute",
            //     "Mute all audio"},
            // {"MasterVol",
            //     "Adjust all volume."},
            // {"MediaAudioMute",
            //     "Mute playfield, backglass and DMD audio"},
            // {"MediaAudioVol",
            //     "Adjust playfield, backglass and DMD video volume."},
            // {"TableMusicMute",
            //     "Mute current table music."},
            // {"TableMusicVol",
            //     "Adjust current table music volume."},
            // {"InterfaceAudioMute",
            //     "Mute interface sounds."},
            // {"InterfaceAudioVol",
            //     "Adjust interface sounds volume."},
            // {"InterfaceAmbienceMute",
            //     "Mute interface ambience."},
            // {"InterfaceAmbienceVol",
            //     "Adjust interface ambience volume."},

            // Internal
            {"SubCmd",
                "VPinballX internal command to play .vpx tables.\n"
                "Use VPinballX --help command line menu to see more."},
            {"LogFile",
                "Path to the debug log file, relative to exec dir."},

            {"IndexPath",
                "Path to the main table index file, relative to exec dir."},
            {"VpxtoolIndex",
                "Path to the vpxtool index file, relative to tables folder by default."},
            {"VpsDbPath",
                "Path to the VPS database file, relative to exec dir."},
            {"VpsDbLastUpdated",
                "Path to the VPS database update file, relative to exec dir."},
            {"VpsDbUpdateFrequency",
                "Choose when to fetch for updates in VPS database.\n"
                "The only option for now is 'startup'."},
            {"ScreenshotWait",
                "Time for the screenshot tool to wait until there are visible windows in VPX."},

            // UI Sounds
            {"ScrollPrevSound",
                "Sound played when scrolling to previous table."},
            {"ScrollNextSound",
                "Sound played when scrolling to next table."},
            {"ScrollFastPrevSound",
                "Sound played when fast scrolling backward."},
            {"ScrollFastNextSound",
                "Sound played when fast scrolling forward."},
            {"ScrollJumpPrevSound",
                "Sound played when jumping to previous letter."},
            {"ScrollJumpNextSound",
                "Sound played when jumping to next letter."},
            {"ScrollRandomSound",
                "Sound played when selecting a random table."},
            {"LaunchTableSound",
                "Sound played when launching a table."},
            {"LaunchScreenshotSound",
                "Sound played when entering screenshot mode."},
            {"ConfigSaveSound",
                "Sound played when saving configuration."},
            {"ConfigToggleSound",
                "Sound played when opening or closing configuration."},
            {"ScreenshotTakeSound",
                "Sound played when taking a screenshot."},
            {"ScreenshotQuitSound",
                "Sound played when exiting screenshot mode."},
            {"AmbienceSound",
                "Sound played on the background if there is no table music."}
        };
        return tooltips;
    }
}

#endif // TOOLTIPS_H