#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
#include <SDL.h>

extern std::string VPX_TABLES_PATH;
extern std::string VPX_EXECUTABLE_CMD;
extern std::string VPX_SUB_CMD;
extern std::string VPX_START_ARGS;
extern std::string VPX_END_ARGS;

extern std::string DEFAULT_TABLE_IMAGE;
extern std::string DEFAULT_BACKGLASS_IMAGE;
extern std::string DEFAULT_DMD_IMAGE;
extern std::string DEFAULT_WHEEL_IMAGE;
extern std::string DEFAULT_TABLE_VIDEO;
extern std::string DEFAULT_BACKGLASS_VIDEO;
extern std::string DEFAULT_DMD_VIDEO;

extern std::string CUSTOM_TABLE_IMAGE;
extern std::string CUSTOM_BACKGLASS_IMAGE;
extern std::string CUSTOM_DMD_IMAGE;
extern std::string CUSTOM_WHEEL_IMAGE;
extern std::string CUSTOM_TABLE_VIDEO;
extern std::string CUSTOM_BACKGLASS_VIDEO;
extern std::string CUSTOM_DMD_VIDEO;

extern int MAIN_WINDOW_MONITOR;
extern int MAIN_WINDOW_WIDTH;
extern int MAIN_WINDOW_HEIGHT;
extern int WHEEL_IMAGE_SIZE;
extern int WHEEL_IMAGE_MARGIN;
extern std::string FONT_PATH;
extern int FONT_SIZE;

extern int SECOND_WINDOW_MONITOR;
extern int SECOND_WINDOW_WIDTH;
extern int SECOND_WINDOW_HEIGHT;
extern int BACKGLASS_MEDIA_WIDTH;
extern int BACKGLASS_MEDIA_HEIGHT;
extern int DMD_MEDIA_WIDTH;
extern int DMD_MEDIA_HEIGHT;

extern int FADE_DURATION_MS;
extern Uint8 FADE_TARGET_ALPHA;
extern std::string TABLE_CHANGE_SOUND;
extern std::string TABLE_LOAD_SOUND;

// Keybinds
extern SDL_Keycode KEY_PREVIOUS_TABLE;
extern SDL_Keycode KEY_NEXT_TABLE;
extern SDL_Keycode KEY_FAST_PREV_TABLE;
extern SDL_Keycode KEY_FAST_NEXT_TABLE;
extern SDL_Keycode KEY_JUMP_NEXT_LETTER;
extern SDL_Keycode KEY_JUMP_PREV_LETTER;
extern SDL_Keycode KEY_LAUNCH_TABLE;
extern SDL_Keycode KEY_TOGGLE_CONFIG;
extern SDL_Keycode KEY_QUIT;
extern SDL_Keycode KEY_CONFIG_SAVE;  // New: Save config
extern SDL_Keycode KEY_CONFIG_CLOSE; // New: Close config

void initialize_config(const std::string& filename);
std::map<std::string, std::map<std::string, std::string>> load_config(const std::string& filename);
std::string get_string(const std::map<std::string, std::map<std::string, std::string>>& config,
                      const std::string& section, const std::string& key, const std::string& default_value);
int get_int(const std::map<std::string, std::map<std::string, std::string>>& config,
            const std::string& section, const std::string& key, int default_value);
SDL_Keycode get_key(const std::map<std::string, std::map<std::string, std::string>>& config,
                    const std::string& section, const std::string& key, SDL_Keycode default_value);

#endif // CONFIG_H