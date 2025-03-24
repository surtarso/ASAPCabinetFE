#include "config/config_loader.h"
#include "logging.h"
#include <fstream>  // For reading the config file
#include <sstream>
#include <iostream>  // For printing messages
#include <algorithm> // For std::transform

// Define all the global variables declared in config_loader.h
std::string VPX_TABLES_PATH;
std::string VPX_EXECUTABLE_CMD;
std::string VPX_SUB_CMD;
std::string VPX_START_ARGS;
std::string VPX_END_ARGS;

std::string DEFAULT_TABLE_IMAGE;
std::string DEFAULT_BACKGLASS_IMAGE;
std::string DEFAULT_DMD_IMAGE;
std::string DEFAULT_WHEEL_IMAGE;
std::string DEFAULT_TABLE_VIDEO;
std::string DEFAULT_BACKGLASS_VIDEO;
std::string DEFAULT_DMD_VIDEO;

std::string CUSTOM_TABLE_IMAGE;
std::string CUSTOM_BACKGLASS_IMAGE;
std::string CUSTOM_DMD_IMAGE;
std::string CUSTOM_WHEEL_IMAGE;
std::string CUSTOM_TABLE_VIDEO;
std::string CUSTOM_BACKGLASS_VIDEO;
std::string CUSTOM_DMD_VIDEO;

int MAIN_WINDOW_MONITOR;
int MAIN_WINDOW_WIDTH;
int MAIN_WINDOW_HEIGHT;
int WHEEL_IMAGE_SIZE;
int WHEEL_IMAGE_MARGIN;

std::string FONT_PATH;
SDL_Color FONT_COLOR;
SDL_Color FONT_BG_COLOR;
int FONT_SIZE;

int SECOND_WINDOW_MONITOR;
int SECOND_WINDOW_WIDTH;
int SECOND_WINDOW_HEIGHT;
int BACKGLASS_MEDIA_WIDTH;
int BACKGLASS_MEDIA_HEIGHT;
int DMD_MEDIA_WIDTH;
int DMD_MEDIA_HEIGHT;

int FADE_DURATION_MS;
Uint8 FADE_TARGET_ALPHA;
std::string TABLE_CHANGE_SOUND;
std::string TABLE_LOAD_SOUND;

// Keybinds for the main application
SDL_Keycode KEY_PREVIOUS_TABLE;
SDL_Keycode KEY_NEXT_TABLE;
SDL_Keycode KEY_FAST_PREV_TABLE;
SDL_Keycode KEY_FAST_NEXT_TABLE;
SDL_Keycode KEY_JUMP_NEXT_LETTER;
SDL_Keycode KEY_JUMP_PREV_LETTER;
SDL_Keycode KEY_LAUNCH_TABLE;
SDL_Keycode KEY_TOGGLE_CONFIG;
SDL_Keycode KEY_QUIT;
SDL_Keycode KEY_CONFIG_SAVE;
SDL_Keycode KEY_CONFIG_CLOSE;
SDL_Keycode KEY_SCREENSHOT_MODE;

// Keybinds for screenshot mode
SDL_Keycode KEY_SCREENSHOT_KEY;    // Added for taking screenshots
SDL_Keycode KEY_SCREENSHOT_QUIT;   // Added for quitting screenshot mode

// load_config: Reads the config.ini file and organizes it into a map.
std::map<std::string, std::map<std::string, std::string>> load_config(const std::string& filename) {
    std::map<std::string, std::map<std::string, std::string>> config;  // A nested map: sections -> keys -> values
    std::ifstream file(filename);  // Open the config file
    std::string current_section;  // Track the current [Section]

    if (!file.is_open()) {  // If the file didn’t open
        std::cerr << "Could not open " << filename << ". Using defaults." << std::endl;
        return config;  // Return an empty config
    }

    std::string line;  // Store each line of the file
    while (std::getline(file, line)) {  // Read the file line by line
        if (line.empty() || line[0] == ';') continue;  // Skip empty lines or comments
        if (line[0] == '[') {  // If it’s a section header
            size_t end = line.find(']');  // Find the closing bracket
            if (end != std::string::npos) {
                current_section = line.substr(1, end - 1);  // Extract the section name
                config[current_section];  // Create an empty map for this section
            }
            continue;
        }
        size_t eq_pos = line.find('=');  // Find the equals sign
        if (eq_pos != std::string::npos && !current_section.empty()) {  // If it’s a key=value line
            std::string key = line.substr(0, eq_pos);  // Everything before the =
            std::string value = line.substr(eq_pos + 1);  // Everything after the =
            key.erase(0, key.find_first_not_of(" \t"));  // Trim leading whitespace
            key.erase(key.find_last_not_of(" \t") + 1);  // Trim trailing whitespace
            value.erase(0, value.find_first_not_of(" \t"));  // Trim leading whitespace
            value.erase(value.find_last_not_of(" \t") + 1);  // Trim trailing whitespace
            config[current_section][key] = value;  // Store the key-value pair
        }
    }
    file.close();  // Close the file
    return config;  // Return the parsed config
}

// get_string: Gets a string value from the config, or a default if not found.
std::string get_string(const std::map<std::string, std::map<std::string, std::string>>& config,
                       const std::string& section, const std::string& key, const std::string& default_value) {
    if (config.count(section) && config.at(section).count(key)) {  // If the section and key exist
        return config.at(section).at(key);  // Return the value
    }
    return default_value;  // Otherwise, return the default
}

// get_int: Gets an integer value from the config, or a default if not found or invalid.
int get_int(const std::map<std::string, std::map<std::string, std::string>>& config,
            const std::string& section, const std::string& key, int default_value) {
    if (config.count(section) && config.at(section).count(key)) {  // If the section and key exist
        try {
            return std::stoi(config.at(section).at(key));  // Convert string to int
        } catch (const std::exception&) {  // If conversion fails
            return default_value;  // Return the default
        }
    }
    return default_value;  // Return the default if not found
}

// get_key: Gets an SDL keycode from the config, or a default if not found or invalid.
SDL_Keycode get_key(const std::map<std::string, std::map<std::string, std::string>>& config,
                    const std::string& section, const std::string& key, SDL_Keycode default_value) {
    if (config.count(section) && config.at(section).count(key)) {  // If the section and key exist
        SDL_Keycode sdlKey = SDL_GetKeyFromName(config.at(section).at(key).c_str());  // Convert name to keycode
        if (sdlKey != SDLK_UNKNOWN) {  // If it’s a valid key
            return sdlKey;  // Return it
        }
    }
    return default_value;  // Return the default if not found or invalid
}

// parse a color string in "R,G,B,A" format into an SDL_Color
SDL_Color parse_color(const std::string& colorStr, const SDL_Color& defaultColor) {
    SDL_Color color = defaultColor;
    std::stringstream ss(colorStr);
    std::string token;
    int values[4] = {0, 0, 0, 255}; // Default to 0,0,0,255 (opaque black) if parsing fails
    int i = 0;

    while (std::getline(ss, token, ',') && i < 4) {
        try {
            values[i] = std::stoi(token);
            // Clamp values to 0-255
            values[i] = std::max(0, std::min(255, values[i]));
        } catch (...) {
            LOG_DEBUG("Invalid color component in: " << colorStr << ", using default");
            return defaultColor;
        }
        i++;
    }

    if (i >= 3) { // At least R,G,B must be provided
        color.r = static_cast<Uint8>(values[0]);
        color.g = static_cast<Uint8>(values[1]);
        color.b = static_cast<Uint8>(values[2]);
        if (i == 4) {
            color.a = static_cast<Uint8>(values[3]);
        } else {
            color.a = 255; // Default to fully opaque if alpha not specified
        }
    } else {
        LOG_DEBUG("Invalid color format in: " << colorStr << ", using default");
        return defaultColor;
    }

    return color;
}

// initialize_config: Loads the config file and sets all global variables.
void initialize_config(const std::string& filename) {
    std::string exeDir = filename.substr(0, filename.find_last_of('/') + 1);  // Get the directory of config.ini
    auto config = load_config(filename);  // Load the config into a map

    // Set VPX settings
    VPX_TABLES_PATH        = get_string(config, "VPX", "TablesPath", "/home/tarso/Games/vpinball/build/tables/");
    VPX_EXECUTABLE_CMD     = get_string(config, "VPX", "ExecutableCmd", "/home/tarso/Games/vpinball/build/VPinballX_GL");
    VPX_SUB_CMD            = get_string(config, "Internal", "SubCmd", "-Play");
    VPX_START_ARGS         = get_string(config, "VPX", "StartArgs", "");
    VPX_END_ARGS           = get_string(config, "VPX", "EndArgs", "");

    // Set custom media paths
    CUSTOM_TABLE_IMAGE     = get_string(config, "CustomMedia", "TableImage", "images/table.png");
    CUSTOM_BACKGLASS_IMAGE = get_string(config, "CustomMedia", "BackglassImage", "images/backglass.png");
    CUSTOM_DMD_IMAGE       = get_string(config, "CustomMedia", "DmdImage", "images/marquee.png");
    CUSTOM_WHEEL_IMAGE     = get_string(config, "CustomMedia", "WheelImage", "images/wheel.png");
    CUSTOM_TABLE_VIDEO     = get_string(config, "CustomMedia", "TableVideo", "video/table.mp4");
    CUSTOM_BACKGLASS_VIDEO = get_string(config, "CustomMedia", "BackglassVideo", "video/backglass.mp4");
    CUSTOM_DMD_VIDEO       = get_string(config, "CustomMedia", "DmdVideo", "video/dmd.mp4");

    // Set window settings
    MAIN_WINDOW_MONITOR    = get_int(config, "WindowSettings", "MainMonitor", 1);
    MAIN_WINDOW_WIDTH      = get_int(config, "WindowSettings", "MainWidth", 1080);
    MAIN_WINDOW_HEIGHT     = get_int(config, "WindowSettings", "MainHeight", 1920);
    SECOND_WINDOW_MONITOR  = get_int(config, "WindowSettings", "SecondMonitor", 0);
    SECOND_WINDOW_WIDTH    = get_int(config, "WindowSettings", "SecondWidth", 1024);
    SECOND_WINDOW_HEIGHT   = get_int(config, "WindowSettings", "SecondHeight", 1024);

    // Set font settings
    FONT_PATH              = get_string(config, "TitleDisplay", "FontPath", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    FONT_COLOR = parse_color(get_string(config, "TitleDisplay", "FontColor", "255,255,255,255"), {255, 255, 255, 255}); // Default: white, fully opaque
    FONT_BG_COLOR = parse_color(get_string(config, "TitleDisplay", "FontBgColor", "0,0,0,128"), {0, 0, 0, 128}); // Default: semi-transparent black
    FONT_SIZE              = get_int(config, "TitleDisplay", "Size", 28);

    // Set media dimensions
    WHEEL_IMAGE_SIZE       = get_int(config, "MediaDimensions", "WheelImageSize", 300);
    WHEEL_IMAGE_MARGIN     = get_int(config, "MediaDimensions", "WheelImageMargin", 24);
    BACKGLASS_MEDIA_WIDTH  = get_int(config, "MediaDimensions", "BackglassWidth", 1024);
    BACKGLASS_MEDIA_HEIGHT = get_int(config, "MediaDimensions", "BackglassHeight", 768);
    DMD_MEDIA_WIDTH        = get_int(config, "MediaDimensions", "DmdWidth", 1024);
    DMD_MEDIA_HEIGHT       = get_int(config, "MediaDimensions", "DmdHeight", 256);

    // Set default media paths (relative to exeDir)
    DEFAULT_TABLE_IMAGE     = exeDir + get_string(config, "Internal", "DefaultTableImage", "img/default_table.png");
    DEFAULT_BACKGLASS_IMAGE = exeDir + get_string(config, "Internal", "DefaultBackglassImage", "img/default_backglass.png");
    DEFAULT_DMD_IMAGE       = exeDir + get_string(config, "Internal", "DefaultDmdImage", "img/default_dmd.png");
    DEFAULT_WHEEL_IMAGE     = exeDir + get_string(config, "Internal", "DefaultWheelImage", "img/default_wheel.png");
    DEFAULT_TABLE_VIDEO     = exeDir + get_string(config, "Internal", "DefaultTableVideo", "img/default_table.mp4");
    DEFAULT_BACKGLASS_VIDEO = exeDir + get_string(config, "Internal", "DefaultBackglassVideo", "img/default_backglass.mp4");
    DEFAULT_DMD_VIDEO       = exeDir + get_string(config, "Internal", "DefaultDmdVideo", "img/default_dmd.mp4");

    // Set fade and sound settings
    FADE_DURATION_MS        = get_int(config, "Internal", "FadeDurationMs", 1);
    FADE_TARGET_ALPHA       = static_cast<Uint8>(get_int(config, "Internal", "FadeTargetAlpha", 255));
    TABLE_CHANGE_SOUND      = get_string(config, "Internal", "TableChangeSound", "snd/table_change.mp3");
    TABLE_LOAD_SOUND        = get_string(config, "Internal", "TableLoadSound", "snd/table_load.mp3");

    // Load keybinds for the main application
    KEY_PREVIOUS_TABLE     = get_key(config, "Keybinds", "PreviousTable", SDLK_LSHIFT);
    KEY_NEXT_TABLE         = get_key(config, "Keybinds", "NextTable", SDLK_RSHIFT);
    KEY_FAST_PREV_TABLE    = get_key(config, "Keybinds", "FastPrevTable", SDLK_LCTRL);
    KEY_FAST_NEXT_TABLE    = get_key(config, "Keybinds", "FastNextTable", SDLK_RCTRL);
    KEY_JUMP_NEXT_LETTER   = get_key(config, "Keybinds", "JumpNextLetter", SDLK_SLASH);
    KEY_JUMP_PREV_LETTER   = get_key(config, "Keybinds", "JumpPrevLetter", SDLK_z);
    KEY_LAUNCH_TABLE       = get_key(config, "Keybinds", "LaunchTable", SDLK_RETURN);
    KEY_TOGGLE_CONFIG      = get_key(config, "Keybinds", "ToggleConfig", SDLK_c);
    KEY_QUIT               = get_key(config, "Keybinds", "Quit", SDLK_q);
    KEY_CONFIG_SAVE        = get_key(config, "Keybinds", "ConfigSave", SDLK_SPACE);
    KEY_CONFIG_CLOSE       = get_key(config, "Keybinds", "ConfigClose", SDLK_q);
    KEY_SCREENSHOT_MODE    = get_key(config, "Keybinds", "ScreenshotMode", SDLK_s);

    // Load keybinds for screenshot mode
    KEY_SCREENSHOT_KEY     = get_key(config, "Keybinds", "ScreenshotKey", SDLK_s);  // Default to 'S'
    KEY_SCREENSHOT_QUIT    = get_key(config, "Keybinds", "ScreenshotQuit", SDLK_q); // Default to 'Q'
}