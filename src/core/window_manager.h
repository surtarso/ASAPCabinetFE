#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "core/iwindow_manager.h" // Inherit from interface
#include <SDL2/SDL.h>
#include <memory>
#include "config/iconfig_service.h"

class WindowManager : public IWindowManager { // Implement IWindowManager
public:
    explicit WindowManager(const Settings& settings);
    ~WindowManager() = default;

    SDL_Window* getPrimaryWindow() override { return primaryWindow_.get(); }
    SDL_Window* getSecondaryWindow() override { return secondaryWindow_.get(); }
    SDL_Renderer* getPrimaryRenderer() override { return primaryRenderer_.get(); }
    SDL_Renderer* getSecondaryRenderer() override { return secondaryRenderer_.get(); }

private:
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> primaryWindow_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> secondaryWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> primaryRenderer_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> secondaryRenderer_;
};

#endif // WINDOW_MANAGER_H