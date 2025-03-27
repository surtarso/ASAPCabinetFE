#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <SDL2/SDL.h>
#include <memory>
#include "config/settings_manager.h"

class WindowManager {
public:
    explicit WindowManager(const Settings& settings);
    ~WindowManager() = default;

    SDL_Window* getPrimaryWindow() { return primaryWindow_.get(); }
    SDL_Window* getSecondaryWindow() { return secondaryWindow_.get(); }
    SDL_Renderer* getPrimaryRenderer() { return primaryRenderer_.get(); }
    SDL_Renderer* getSecondaryRenderer() { return secondaryRenderer_.get(); }

private:
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> primaryWindow_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> secondaryWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> primaryRenderer_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> secondaryRenderer_;
};

#endif // WINDOW_MANAGER_H