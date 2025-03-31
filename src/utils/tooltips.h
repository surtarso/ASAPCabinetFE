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
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"
            },
            {"ExecutableCmd",
                "Defines the absolute path to the VPinballX executable.\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"
            },
            {"StartArgs",
                "Optional command-line arguments to prepend to the executable.\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"
            },
            {"EndArgs",
                "Optional arguments to append after the table file in the command.\n"
                "\n"
                "Final command:\n"
                "StartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs"
            },

            // Custom Media
            {"WheelImage",
                "Relative path to the wheel image for the table.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPTopperImage",
                "Relative path to the PuP topper image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"TableImage",
                "Relative path to the table's preview image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPTableImage",
                "Relative path to the PuP table image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"BackglassImage",
                "Relative path to the backglass image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPBackglassImage",
                "Relative path to the PuP backglass image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"DmdImage",
                "Relative path to the DMD or marquee image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPDmdImage",
                "Relative path to the PuP DMD image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPFullDmdImage",
                "Relative path to the PuP full DMD image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPTopperVideo",
                "Relative path to the PuP topper video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"TableVideo",
                "Relative path to the table preview video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPTableVideo",
                "Relative path to the PuP table video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"BackglassVideo",
                "Relative path to the backglass video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPBackglassVideo",
                "Relative path to the PuP backglass video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"DmdVideo",
                "Relative path to the DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPDmdVideo",
                "Relative path to the PuP DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"PuPFullDmdVideo",
                "Relative path to the PuP full DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },

            // WindowSettings
            {"MainMonitor",
                "Index of the monitor for the table playfield window.\n"
                "You can use 'xrandr' to get yours."
            },
            {"MainWidth",
                "Width of the main window in pixels.\n"
                "This should be relative to your playfield media width."
            },
            {"MainHeight",
                "Height of the main window in pixels.\n"
                "This should be relative to your playfield media height."
            },
            {"SecondMonitor",
                "Index of the monitor for the backglass/DMD window.\n"
                "You can use 'xrandr' to get yours."
            },
            {"SecondWidth",
                "Width of the secondary window in pixels.\n"
                "This should be relative to your backglass + DMD media width."
            },
            {"SecondHeight",
                "Height of the secondary window in pixels.\n"
                "This should be relative to your backglass + DMD media height."
            },

            // TitleDisplay
            {"FontPath",
                "Absolute path to the font file used in the UI."
            },
            {"FontColor",
                "Color of the table title text in R,G,B,A format (0-255).\nExample: 255,255,255,255 for white."
            },
            {"FontBgColor",
                "Background color behind the table title in R,G,B,A format (0-255).\nExample: 0,0,0,128 for semi-transparent black."
            },
            {"FontSize",
                "Font size in points for table title text rendering."
            },

            // MediaDimensions
            {"WheelImageSize",
                "Size of the wheel image in pixels.\n"
                "This considers a square image."
            },
            {"WheelImageMargin",
                "Margin around the wheel image in pixels."
            },
            {"BackglassWidth",
                "Width of the backglass media in pixels."
            },
            {"BackglassHeight",
                "Height of the backglass media in pixels."
            },
            {"DmdWidth",
                "Width of the DMD media in pixels."
            },
            {"DmdHeight",
                "Height of the DMD media in pixels."
            },

            // Keybinds
            {"PreviousTable",
                "Key to select the previous table in the list."
            },
            {"NextTable",
                "Key to select the next table in the list."
            },
            {"FastPrevTable",
                "Key to quickly jump back 10 tables."
            },
            {"FastNextTable",
                "Key to quickly jump forward 10 tables."
            },
            {"JumpNextLetter",
                "Key to jump to the next table starting with a different letter."
            },
            {"JumpPrevLetter",
                "Key to jump to the previous table starting with a different letter."
            },
            {"RandomTable",
                "Key to jump to a random table."
            },
            {"LaunchTable",
                "Key to launch the selected table."
            },
            {"ToggleConfig",
                "Key to open or close the configuration menu."
            },
            {"Quit",
                "Key to exit menus and application."
            },
            {"ConfigSave",
                "Key to save changes in the configuration menu."
            },
            {"ConfigClose",
                "Key to close the configuration menu without saving."
            },
            {"ScreenshotMode",
                "Key to launch a table in screenshot mode."
            },
            {"ScreenshotKey",
                "Key to take a screenshot while in screenshot mode."
            },
            {"ScreenshotQuit",
                "Key to quit screenshot mode."
            },

            // Internal
            {"SubCmd",
                "VPinballX internal command to play .vpx tables.\n"
                "Use VPinballX --help command line menu to see more."
            },

            // DefaultMedia
            {"DefaultTableImage",
                "Default relative path to the table image."
            },
            {"DefaultPuPTableImage",
                "Default relative path to the PuP table image."
            },
            {"DefaultBackglassImage",
                "Default relative path to the backglass image."
            },
            {"DefaultPuPBackglassImage",
                "Default relative path to the PuP backglass image."
            },
            {"DefaultDmdImage",
                "Default relative path to the DMD image."
            },
            {"DefaultPuPDmdImage",
                "Default relative path to the PuP DMD image."
            },
            {"DefaultPuPFullDmdImage",
                "Default relative path to the PuP full DMD image."
            },
            {"DefaultPupTopperImage",
                "Default relative path to the PuP topper image."
            },
            {"DefaultWheelImage",
                "Default relative path to the wheel image."
            },
            {"DefaultTableVideo",
                "Default relative path to the table video."
            },
            {"DefaultPuPTableVideo",
                "Default relative path to the PuP table video."
            },
            {"DefaultBackglassVideo",
                "Default relative path to the backglass video."
            },
            {"DefaultPuPBackglassVideo",
                "Default relative path to the PuP backglass video."
            },
            {"DefaultDmdVideo",
                "Default relative path to the DMD video."
            },
            {"DefaultPuPDmdVideo",
                "Default relative path to the PuP DMD video."
            },
            {"DefaultPuPFullDmdVideo",
                "Default relative path to the PuP full DMD video."
            },
            {"DefaultPuPTopperVideo",
                "Default relative path to the PuP topper video."
            },

            // UISounds
            {"ScrollPrevSound",
                "Relative path to the previous scroll sound file."
            },
            {"ScrollNextSound",
                "Relative path to the next scroll sound file."
            },
            {"ScrollFastPrevSound",
                "Relative path to the fast previous scroll sound file."
            },
            {"ScrollFastNextSound",
                "Relative path to the fast next scroll sound file."
            },
            {"ScrollJumpPrevSound",
                "Relative path to the jump previous scroll sound file."
            },
            {"ScrollJumpNextSound",
                "Relative path to the jump next scroll sound file."
            },
            {"ScrollRandomSound",
                "Relative path to the random scroll sound file."
            },
            {"LaunchTableSound",
                "Relative path to the launch table sound file."
            },
            {"LaunchScreenshotSound",
                "Relative path to the launch screenshot sound file."
            },
            {"ConfigSaveSound",
                "Relative path to the config save sound file."
            },
            {"ConfigCloseSound",
                "Relative path to the config close sound file."
            },
            {"QuitSound",
                "Relative path to the quit sound file."
            },
            {"ScreenshotTakeSound",
                "Relative path to the screenshot take sound file."
            },
            {"ScreenshotQuitSound",
                "Relative path to the screenshot quit sound file."
            }
        };
    }
}

#endif // TOOLTIPS_H