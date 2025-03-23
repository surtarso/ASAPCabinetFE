#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>  // For working with strings
#include <map>     // For storing config data in a key-value structure
#include <SDL2/SDL.h>  // For SDL_Keycode type

// Global variables for VPX settings
extern std::string VPX_TABLES_PATH;        // Path to the folder with VPX tables
extern std::string VPX_EXECUTABLE_CMD;     // Command to run VPX
extern std::string VPX_SUB_CMD;            // Subcommand like "-play"
extern std::string VPX_START_ARGS;         // Extra arguments before the table file
extern std::string VPX_END_ARGS;           // Extra arguments after the table file

// Default media paths (used when custom media isn’t found)
extern std::string DEFAULT_TABLE_IMAGE;     // Default table image
extern std::string DEFAULT_BACKGLASS_IMAGE; // Default backglass image
extern std::string DEFAULT_DMD_IMAGE;       // Default DMD image
extern std::string DEFAULT_WHEEL_IMAGE;     // Default wheel image
extern std::string DEFAULT_TABLE_VIDEO;     // Default table video
extern std::string DEFAULT_BACKGLASS_VIDEO; // Default backglass video
extern std::string DEFAULT_DMD_VIDEO;       // Default DMD video

// Custom media paths (relative to the table folder)
extern std::string CUSTOM_TABLE_IMAGE;     // Custom table image path
extern std::string CUSTOM_BACKGLASS_IMAGE; // Custom backglass image path
extern std::string CUSTOM_DMD_IMAGE;       // Custom DMD image path
extern std::string CUSTOM_WHEEL_IMAGE;     // Custom wheel image path
extern std::string CUSTOM_TABLE_VIDEO;     // Custom table video path
extern std::string CUSTOM_BACKGLASS_VIDEO; // Custom backglass video path
extern std::string CUSTOM_DMD_VIDEO;       // Custom DMD video path

// Window settings
extern int MAIN_WINDOW_MONITOR;    // Which monitor the main window uses
extern int MAIN_WINDOW_WIDTH;      // Width of the main window
extern int MAIN_WINDOW_HEIGHT;     // Height of the main window
extern int WHEEL_IMAGE_SIZE;       // Size of the wheel image
extern int WHEEL_IMAGE_MARGIN;     // Margin around the wheel image
extern std::string FONT_PATH;      // Path to the font file
extern int FONT_SIZE;              // Size of the font

extern int SECOND_WINDOW_MONITOR;  // Which monitor the second window uses
extern int SECOND_WINDOW_WIDTH;    // Width of the second window
extern int SECOND_WINDOW_HEIGHT;   // Height of the second window
extern int BACKGLASS_MEDIA_WIDTH;  // Width of backglass media
extern int BACKGLASS_MEDIA_HEIGHT; // Height of backglass media
extern int DMD_MEDIA_WIDTH;        // Width of DMD media
extern int DMD_MEDIA_HEIGHT;       // Height of DMD media

// Fade and sound settings
extern int FADE_DURATION_MS;       // How long fades take (in milliseconds)
extern Uint8 FADE_TARGET_ALPHA;    // Target transparency for fades (0-255)
extern std::string TABLE_CHANGE_SOUND;  // Sound when changing tables
extern std::string TABLE_LOAD_SOUND;    // Sound when loading a table

// Keybinds for the main application
extern SDL_Keycode KEY_PREVIOUS_TABLE;     // Key to go to the previous table
extern SDL_Keycode KEY_NEXT_TABLE;         // Key to go to the next table
extern SDL_Keycode KEY_FAST_PREV_TABLE;    // Key to fast-scroll previous
extern SDL_Keycode KEY_FAST_NEXT_TABLE;    // Key to fast-scroll next
extern SDL_Keycode KEY_JUMP_NEXT_LETTER;   // Key to jump to next letter
extern SDL_Keycode KEY_JUMP_PREV_LETTER;   // Key to jump to previous letter
extern SDL_Keycode KEY_LAUNCH_TABLE;       // Key to launch a table
extern SDL_Keycode KEY_TOGGLE_CONFIG;      // Key to open/close config
extern SDL_Keycode KEY_QUIT;               // Key to quit the app
extern SDL_Keycode KEY_CONFIG_SAVE;        // Key to save config
extern SDL_Keycode KEY_CONFIG_CLOSE;       // Key to close config
extern SDL_Keycode KEY_SCREENSHOT_MODE;    // Key to enter screenshot mode

// Keybinds for screenshot mode
extern SDL_Keycode KEY_SCREENSHOT_KEY;     // Key to take a screenshot
extern SDL_Keycode KEY_SCREENSHOT_QUIT;    // Key to quit screenshot mode

// Functions to load and parse the config file
std::map<std::string, std::map<std::string, std::string>> load_config(const std::string& filename);
std::string get_string(const std::map<std::string, std::map<std::string, std::string>>& config,
                       const std::string& section, const std::string& key, const std::string& default_value);
int get_int(const std::map<std::string, std::map<std::string, std::string>>& config,
            const std::string& section, const std::string& key, int default_value);
SDL_Keycode get_key(const std::map<std::string, std::map<std::string, std::string>>& config,
                    const std::string& section, const std::string& key, SDL_Keycode default_value);
void initialize_config(const std::string& filename);

#endif // CONFIG_LOADER_H