#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class IConfigService;
class IKeybindProvider;

class ScreenshotWindow {
public:
    ScreenshotWindow(IConfigService* configManager, IKeybindProvider* keybindProvider);
    ~ScreenshotWindow();
    bool initialize(int width, int height);
    void render();
    void raiseAndFocus();
    void cleanup();  // Added
    SDL_Window* getWindow() const { return window_; }

private:
    IConfigService* configManager_;
    IKeybindProvider* keybindProvider_;
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    TTF_Font* font_;
    SDL_Texture* textTexture_;
    SDL_Rect buttonRect_;
    std::string buttonText_;
};

#endif // SCREENSHOT_WINDOW_H