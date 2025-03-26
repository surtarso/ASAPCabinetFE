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
            {"TableImage",
                "Relative path to the table's preview image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"BackglassImage",
                "Relative path to the backglass image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"WheelImage",
                "Relative path to the wheel image for the table.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"DmdImage",
                "Relative path to the DMD or marquee image.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"TableVideo",
                "Relative path to the table preview video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"BackglassVideo",
                "Relative path to the backglass video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },
            {"DmdVideo",
                "Relative path to the DMD video.\n"
                "These are relative to your table folder.\n"
                "/path/to/tables/<table_folder>/"
            },

            // Screen/Monitor
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
            {"Path",
                "Absolute path to the font file used in the UI."
            },
            {"Size",
                "Font size in points for table title text rendering."
            },
            {"FontColor",
                "Color of the table title text in R,G,B,A format (0-255).\nExample: 255,255,255,255 for white."
            },
            {"FontBgColor",
                "Background color behind the table title in R,G,B,A format (0-255).\nExample: 0,0,0,128 for semi-transparent black."
            },

            // Media Dimensions
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

            // Keybinds Section Tooltips
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
                "Key to quickly jump foward 10 tables."   
            },
            {"JumpNextLetter",
                "Key to jump to the next table starting with a different letter." 
            },
            {"JumpPrevLetter",
                "Key to jump to the previous table starting with a different letter." 
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
            {"RandomTable",
                "Key to jump to a random table."
            },
            
            // Internal
            {"SubCmd",
                "VPinballX internal command to play .vpx tables.\n"
                "Use VPinballX --help command line menu to see more."
            },
            {"FadeTargetAlpha",
                "Goes from 0 (transparent) to 255.\n"
                "Use 128 for ~50 percent alpha."
            },
            {"FadeDurationMs",
                "Table images switch transition time in ms.\n"
                "Set to 1 if using videos."
            },
        };
    }
}

#endif // TOOLTIPS_H