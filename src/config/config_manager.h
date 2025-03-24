#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <memory>
#include "SDL.h"
#include "SDL_ttf.h"
#include "table/asset_manager.h"
#include "config/config_loader.h"
#include "logging.h"

// Manages configuration changes and updates application state accordingly.
class ConfigManager {
public:
    ConfigManager(const std::string& configPath,
                  SDL_Window* primaryWindow,
                  SDL_Renderer* primaryRenderer,
                  SDL_Window* secondaryWindow,
                  SDL_Renderer* secondaryRenderer,
                  std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>& font,
                  AssetManager& assets,
                  size_t currentIndex,
                  const std::vector<Table>& tables)
        : configPath_(configPath),
          primaryWindow_(primaryWindow),
          primaryRenderer_(primaryRenderer),
          secondaryWindow_(secondaryWindow),
          secondaryRenderer_(secondaryRenderer),
          font_(font),
          assets_(assets),
          currentIndex_(currentIndex),
          tables_(tables),
          changesPending_(false) {}

    // Called by IniEditor when config changes are saved.
    void notifyConfigChanged() {
        changesPending_ = true;
    }

    // Checks and applies config changes if pending.
    void applyConfigChanges() {
        if (!changesPending_) return;

        LOG_DEBUG("Reloading UI due to config changes...");
        // Reload config using config_loader.
        initialize_config(configPath_);

        // Update windows with new dimensions and positions.
        SDL_SetWindowSize(primaryWindow_, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
        SDL_SetWindowPosition(primaryWindow_, SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED);
        SDL_SetWindowSize(secondaryWindow_, SECOND_WINDOW_WIDTH, SECOND_WINDOW_HEIGHT);
        SDL_SetWindowPosition(secondaryWindow_, SDL_WINDOWPOS_CENTERED_DISPLAY(SECOND_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED);

        // Reload font with new size.
        font_.reset(TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE));
        if (!font_) std::cerr << "Failed to reload font: " << TTF_GetError() << std::endl;

        // Update assets with new font and dimensions.
        assets_ = AssetManager(primaryRenderer_, secondaryRenderer_, font_.get());
        assets_.loadTableAssets(currentIndex_, tables_);

        changesPending_ = false;
        LOG_DEBUG("UI reloaded: MainWidth=" << MAIN_WINDOW_WIDTH << ", MainHeight=" << MAIN_WINDOW_HEIGHT
                  << ", SecondWidth=" << SECOND_WINDOW_WIDTH << ", SecondHeight=" << SECOND_WINDOW_HEIGHT
                  << ", FontSize=" << FONT_SIZE << ", WheelImageSize=" << WHEEL_IMAGE_SIZE);
    }

private:
    std::string configPath_;
    SDL_Window* primaryWindow_;
    SDL_Renderer* primaryRenderer_;
    SDL_Window* secondaryWindow_;
    SDL_Renderer* secondaryRenderer_;
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>& font_;
    AssetManager& assets_;
    size_t currentIndex_;
    const std::vector<Table>& tables_;
    bool changesPending_;
};

#endif // CONFIG_MANAGER_H