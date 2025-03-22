#ifndef SCREENSHOT_UTILS_H
#define SCREENSHOT_UTILS_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

std::string shell_escape(const std::string& str);
bool is_window_visible_log(const std::string& title);
void capture_screenshot(const std::string& window_name, const std::string& output_path);
void capture_all_screenshots(const std::string& table_image, const std::string& backglass_image, 
                            const std::string& dmd_image, SDL_Window* window);
void launch_screenshot_mode(const std::string& vpx_file);

#endif // SCREENSHOT_UTILS_H