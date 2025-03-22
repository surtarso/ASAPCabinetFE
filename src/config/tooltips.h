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

            // Font
            {"Path",
                "Absolute path to the font file used in the UI."
            },
            {"Size",
                "Font size in points for table title text rendering."
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

            // Internal
            {"FadeTargetAlpha",
                "Goes from 0 (transparent) to 255.\n"
                "Use 128 for ~50 percent alpha."
            },
            {"FadeDurationMs",
                "Table images switch transition time in ms.\n"
                "Set to 1 if using videos."
            },

            // Keybinds Section Tooltips
            {"PreviousTable",
                "Key to select the previous table in the list.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: LSHIFT, a, UP, 1\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"NextTable",
                "Key to select the next table in the list.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: RSHIFT, b, DOWN, 2\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"FastPrevTable",
                "Key to quickly jump to the previous table group.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: LCTRL, LEFT, 3\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"FastNextTable",
                "Key to quickly jump to the next table group.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: RCTRL, RIGHT, 4\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"JumpNextLetter",
                "Key to jump to the next table starting with a different letter.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: slash, n, 5\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"JumpPrevLetter",
                "Key to jump to the previous table starting with a different letter.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: z, p, 6\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"LaunchTable",
                "Key to launch the selected table.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: RETURN, SPACE, l\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"ToggleConfig",
                "Key to open or close the configuration menu.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: c, TAB, 7\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"Quit",
                "Key to exit the application.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: q, ESCAPE, 8\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"ConfigSave",
                "Key to save changes in the configuration menu.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: SPACE, s, 9\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"ConfigClose",
                "Key to close the configuration menu without saving.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: q, ESCAPE, 0\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            },
            {"ScreenshotMode",
                "Key to toggle screenshot mode.\n"
                "\n"
                "Uses SDL2 key names (case-sensitive, remove 'SDL_' prefix).\n"
                "Examples: s, F1, m\n"
                "See SDL documentation for full list: https://wiki.libsdl.org/SDL2/SDL_Scancode"
            }
        };
    }
}

#endif // TOOLTIPS_H