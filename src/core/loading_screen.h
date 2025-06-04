// core/loading_screen.h
#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <memory>
#include <string>
#include <SDL2/SDL.h> // For SDL_DisplayMode
#include "imgui.h" // For ImGui rendering
#include "core/loading_progress.h" // Include the progress data structure

class LoadingScreen {
public:
    /**
     * @brief Constructs a LoadingScreen instance.
     * @param progress Shared pointer to the LoadingProgress data.
     */
    LoadingScreen(std::shared_ptr<LoadingProgress> progress);

    /**
     * @brief Renders the loading screen with progress and stats.
     * This method should be called within the ImGui rendering loop.
     */
    void render();

private:
    std::shared_ptr<LoadingProgress> loadingProgress_;
};

#endif // LOADING_SCREEN_H