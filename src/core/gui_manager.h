#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include <SDL2/SDL.h>
#include <memory>

struct ImGuiContext;
class IWindowManager;
class IConfigService;

class GuiManager {
public:
    // Constructor for main app (uses IWindowManager)
    GuiManager(IWindowManager* windowManager, IConfigService* configService);

    // Constructor for standalone initial config (uses raw SDL window/renderer)
    GuiManager(SDL_Window* window, SDL_Renderer* renderer, IConfigService* configService);
    ~GuiManager();

    void initialize();              // Sets up ImGui context
    void newFrame();                // Starts a new ImGui frame
    void render(SDL_Renderer* renderer); // Renders ImGui draw data
    void processEvent(const SDL_Event& event); // Forwards events to ImGui

private:
    IWindowManager* windowManager_;  // Main app window manager (nullptr in config mode)
    IConfigService* configService_;  // Changed to interface pointer
    SDL_Window* configWindow_;       // Config window (nullptr in main app mode)
    SDL_Renderer* configRenderer_;   // Config renderer (nullptr in main app mode)
    ImGuiContext* context_;          // ImGui context
};

#endif // GUI_MANAGER_H
