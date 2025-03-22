#include "config/config_loader.h"
#include <fstream>
#include <iostream>

// Configuration Variable Definitions
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

// Keybinds Definitions
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

std::map<std::string, std::map<std::string, std::string>> load_config(const std::string& filename) {
    std::map<std::string, std::map<std::string, std::string>> config;
    std::ifstream file(filename);
    std::string current_section;

    if (!file.is_open()) {
        std::cerr << "Could not open " << filename << ". Using defaults." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                current_section = line.substr(1, end - 1);
                config[current_section];
            }
            continue;
        }
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos && !current_section.empty()) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            config[current_section][key] = value;
        }
    }
    file.close();
    return config;
}

std::string get_string(const std::map<std::string, std::map<std::string, std::string>>& config,
                       const std::string& section, const std::string& key, const std::string& default_value) {
    if (config.count(section) && config.at(section).count(key)) {
        return config.at(section).at(key);
    }
    return default_value;
}

int get_int(const std::map<std::string, std::map<std::string, std::string>>& config,
            const std::string& section, const std::string& key, int default_value) {
    if (config.count(section) && config.at(section).count(key)) {
        try {
            return std::stoi(config.at(section).at(key));
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

SDL_Keycode get_key(const std::map<std::string, std::map<std::string, std::string>>& config,
                    const std::string& section, const std::string& key, SDL_Keycode default_value) {
    if (config.count(section) && config.at(section).count(key)) {
        SDL_Keycode sdlKey = SDL_GetKeyFromName(config.at(section).at(key).c_str());
        if (sdlKey != SDLK_UNKNOWN) {
            // std::cout << "Loaded " << section << ":" << key << " = " << config.at(section).at(key)
                    //   << " (SDL Key: " << sdlKey << ")" << std::endl;
            return sdlKey;
        } //else {
        //     std::cerr << "Unknown key: " << config.at(section).at(key) << " for " << key << std::endl;
        // }
    }
    return default_value;
}

void initialize_config(const std::string& filename) {
    // auto config = load_config(filename);
    std::string exeDir = filename.substr(0, filename.find_last_of('/') + 1);  // Extract dir from config path
    auto config = load_config(filename);

    VPX_TABLES_PATH        = get_string(config, "VPX", "TablesPath", "/home/tarso/Games/vpinball/build/tables/");
    VPX_EXECUTABLE_CMD     = get_string(config, "VPX", "ExecutableCmd", "/home/tarso/Games/vpinball/build/VPinballX_GL");
    VPX_SUB_CMD            = get_string(config, "Internal", "SubCmd", "-Play");
    VPX_START_ARGS         = get_string(config, "VPX", "StartArgs", "");
    VPX_END_ARGS           = get_string(config, "VPX", "EndArgs", "");

    CUSTOM_TABLE_IMAGE     = get_string(config, "CustomMedia", "TableImage", "images/table.png");
    CUSTOM_BACKGLASS_IMAGE = get_string(config, "CustomMedia", "BackglassImage", "images/backglass.png");
    CUSTOM_DMD_IMAGE       = get_string(config, "CustomMedia", "DmdImage", "images/marquee.png");
    CUSTOM_WHEEL_IMAGE     = get_string(config, "CustomMedia", "WheelImage", "images/wheel.png");
    CUSTOM_TABLE_VIDEO     = get_string(config, "CustomMedia", "TableVideo", "video/table.mp4");
    CUSTOM_BACKGLASS_VIDEO = get_string(config, "CustomMedia", "BackglassVideo", "video/backglass.mp4");
    CUSTOM_DMD_VIDEO       = get_string(config, "CustomMedia", "DmdVideo", "video/dmd.mp4");

    MAIN_WINDOW_MONITOR    = get_int(config, "WindowSettings", "MainMonitor", 1);
    MAIN_WINDOW_WIDTH      = get_int(config, "WindowSettings", "MainWidth", 1080);
    MAIN_WINDOW_HEIGHT     = get_int(config, "WindowSettings", "MainHeight", 1920);
    SECOND_WINDOW_MONITOR  = get_int(config, "WindowSettings", "SecondMonitor", 0);
    SECOND_WINDOW_WIDTH    = get_int(config, "WindowSettings", "SecondWidth", 1024);
    SECOND_WINDOW_HEIGHT   = get_int(config, "WindowSettings", "SecondHeight", 1024);

    FONT_PATH              = get_string(config, "Internal", "FontPath", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    FONT_SIZE              = get_int(config, "Font", "Size", 28);

    WHEEL_IMAGE_SIZE       = get_int(config, "MediaDimensions", "WheelImageSize", 300);
    WHEEL_IMAGE_MARGIN     = get_int(config, "MediaDimensions", "WheelImageMargin", 24);
    BACKGLASS_MEDIA_WIDTH  = get_int(config, "MediaDimensions", "BackglassWidth", 1024);
    BACKGLASS_MEDIA_HEIGHT = get_int(config, "MediaDimensions", "BackglassHeight", 768);
    DMD_MEDIA_WIDTH        = get_int(config, "MediaDimensions", "DmdWidth", 1024);
    DMD_MEDIA_HEIGHT       = get_int(config, "MediaDimensions", "DmdHeight", 256);

    DEFAULT_TABLE_IMAGE     = get_string(config, "Internal", "DefaultTableImage", "img/default_table.png");
    DEFAULT_BACKGLASS_IMAGE = get_string(config, "Internal", "DefaultBackglassImage", "img/default_backglass.png");
    DEFAULT_DMD_IMAGE       = get_string(config, "Internal", "DefaultDmdImage", "img/default_dmd.png");
    DEFAULT_WHEEL_IMAGE     = get_string(config, "Internal", "DefaultWheelImage", "img/default_wheel.png");
    DEFAULT_TABLE_VIDEO     = get_string(config, "Internal", "DefaultTableVideo", "img/default_table.mp4");
    DEFAULT_BACKGLASS_VIDEO = get_string(config, "Internal", "DefaultBackglassVideo", "img/default_backglass.mp4");
    DEFAULT_DMD_VIDEO       = get_string(config, "Internal", "DefaultDmdVideo", "img/default_dmd.mp4");
    FADE_DURATION_MS        = get_int(config, "Internal", "FadeDurationMs", 1);
    FADE_TARGET_ALPHA       = static_cast<Uint8>(get_int(config, "Internal", "FadeTargetAlpha", 255));
    TABLE_CHANGE_SOUND      = get_string(config, "Internal", "TableChangeSound", "snd/table_change.mp3");
    TABLE_LOAD_SOUND        = get_string(config, "Internal", "TableLoadSound", "snd/table_load.mp3");

    // Load keybinds
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
    
    // Debug keybinds
    // std::cout << "KEY_PREVIOUS_TABLE: " << KEY_PREVIOUS_TABLE << std::endl;
    // std::cout << "KEY_NEXT_TABLE: " << KEY_NEXT_TABLE << std::endl;
    // std::cout << "KEY_LAUNCH_TABLE: " << KEY_LAUNCH_TABLE << std::endl;
}