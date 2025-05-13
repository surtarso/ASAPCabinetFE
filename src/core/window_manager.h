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

    SDL_Window* getPlayfieldWindow() override { return playfieldWindow_.get(); }
    SDL_Window* getBackglassWindow() override { return backglassWindow_.get(); }
    SDL_Window* getDMDWindow() override { return dmdWindow_.get(); }
    SDL_Renderer* getPlayfieldRenderer() override { return playfieldRenderer_.get(); }
    SDL_Renderer* getBackglassRenderer() override { return backglassRenderer_.get(); }
    SDL_Renderer* getDMDRenderer() override { return dmdRenderer_.get(); }
    void updateWindows(const Settings& settings) override;

private:
    void createOrUpdateWindow(std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>& window,
                             std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>& renderer,
                             const char* title, int monitor, int width, int height,
                             float dpiScale, bool enableDpiScaling);

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> playfieldWindow_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> backglassWindow_;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> dmdWindow_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> playfieldRenderer_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> backglassRenderer_;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> dmdRenderer_;
};

#endif // WINDOW_MANAGER_H