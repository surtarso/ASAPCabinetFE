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
                "Specifies the absolute path to the folder containing VPX table files.\n"
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
            {"PuPTopperImage",
                "Relative path to the PuP topper image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PlayfieldImage",
                "Relative path to the table's preview image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPPlayfieldImage",
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
            {"PlayfieldVideo",
                "Relative path to the table preview video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"},
            {"PuPPlayfieldVideo",
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
            {"TableMusic",
                "Relative path to the table music file.\n"
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

            // TitleDisplay
            {"TitleSource",
                "Select the source of the title info.\n"
                "- 'filename' to use filename as title.\n"
                "- 'metadata' to use file metadata as title. (requires vpxtool)\n"},
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
            {"ShowTitle",
                "Toggle visibility of table titles in the main window.\n"
                "Set to true to show titles, false to hide them."},
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

            // Default Media
            {"DefaultPlayfieldImage",
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

            {"DefaultPlayfieldVideo",
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
            {"ConfigClose",
                "Key to close the configuration menu without saving."},
            {"ScreenshotMode",
                "Key to launch a table in screenshot mode."},
            {"ScreenshotKey",
                "Key to take a screenshot while in screenshot mode."},
            {"ScreenshotQuit",
                "Key to quit screenshot mode."},
            
            // Audio Settings
            {"MediaAudioMute",
                "Mute playfield, backglass and DMD audio"},
            {"MediaAudioVol",
                "Adjust playfield, backglass and DMD video volume."},
            {"TableMusicMute",
                "Mute current table music."},
            {"TableMusicVol",
                "Adjust current table music volume."},
            {"InterfaceAudioMute",
                "Mute interface sounds."},
            {"InterfaceAudioVol",
                "Adjust interface sounds volume."},
            {"InterfaceAmbienceMute",
                "Mute interface ambience."},
            {"InterfaceAmbienceVol",
                "Adjust interface ambience volume."},

            // Internal
            {"SubCmd",
                "VPinballX internal command to play .vpx tables.\n"
                "Use VPinballX --help command line menu to see more."},
            {"LogFile",
                "Path to the debug log file, relative to exec dir."},
            {"VideoBackend",
                "Select video playback backend:\n"
                "VLC: Software Decoding\n"
                "FFMpeg: Hardware Accelerated"},

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
                "Sound played when exiting screenshot mode."}
        };
        return tooltips;
    }
}

#endif // TOOLTIPS_H