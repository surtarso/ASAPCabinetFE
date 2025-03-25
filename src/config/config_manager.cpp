#include "config/config_manager.h"
#include "config/config_loader.h"
#include "config/settings.h"
#include "table/asset_manager.h"
#include "table/table_manager.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Constructor
ConfigManager::ConfigManager(const std::string& configPath, SDL_Window* mainWindow,
                             SDL_Renderer* mainRenderer, SDL_Window* playfieldWindow,
                             SDL_Renderer* playfieldRenderer,
                             std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>& font,
                             AssetManager& assetManager, size_t& selectedTableIndex,
                             std::vector<Table>& tables)
    : configPath_(configPath),
      mainWindow_(mainWindow),
      mainRenderer_(mainRenderer),
      playfieldWindow_(playfieldWindow),
      playfieldRenderer_(playfieldRenderer),
      font_(font),
      assetManager_(assetManager),
      selectedTableIndex_(selectedTableIndex),
      tables_(tables) {
    loadConfig(configPath);
}

void ConfigManager::loadConfig(const std::string& configFile) {
    initialize_config(configFile);
    settings.vpxTablesPath = VPX_TABLES_PATH;
    settings.vpxExecutableCmd = VPX_EXECUTABLE_CMD;
    settings.vpxSubCmd = VPX_SUB_CMD;
    settings.vpxStartArgs = VPX_START_ARGS;
    settings.vpxEndArgs = VPX_END_ARGS;

    settings.defaultTableImage = DEFAULT_TABLE_IMAGE;
    settings.defaultBackglassImage = DEFAULT_BACKGLASS_IMAGE;
    settings.defaultDmdImage = DEFAULT_DMD_IMAGE;
    settings.defaultWheelImage = DEFAULT_WHEEL_IMAGE;
    settings.defaultTableVideo = DEFAULT_TABLE_VIDEO;
    settings.defaultBackglassVideo = DEFAULT_BACKGLASS_VIDEO;
    settings.defaultDmdVideo = DEFAULT_DMD_VIDEO;

    settings.customTableImage = CUSTOM_TABLE_IMAGE;
    settings.customBackglassImage = CUSTOM_BACKGLASS_IMAGE;
    settings.customDmdImage = CUSTOM_DMD_IMAGE;
    settings.customWheelImage = CUSTOM_WHEEL_IMAGE;
    settings.customTableVideo = CUSTOM_TABLE_VIDEO;
    settings.customBackglassVideo = CUSTOM_BACKGLASS_VIDEO;
    settings.customDmdVideo = CUSTOM_DMD_VIDEO;

    settings.mainWindowMonitor = MAIN_WINDOW_MONITOR;
    settings.mainWindowWidth = MAIN_WINDOW_WIDTH;
    settings.mainWindowHeight = MAIN_WINDOW_HEIGHT;
    settings.wheelImageSize = WHEEL_IMAGE_SIZE;
    settings.wheelImageMargin = WHEEL_IMAGE_MARGIN;

    settings.fontPath = FONT_PATH;
    settings.fontColor = FONT_COLOR;
    settings.fontBgColor = FONT_BG_COLOR;
    settings.fontSize = FONT_SIZE;

    settings.secondWindowMonitor = SECOND_WINDOW_MONITOR;
    settings.secondWindowWidth = SECOND_WINDOW_WIDTH;
    settings.secondWindowHeight = SECOND_WINDOW_HEIGHT;
    settings.backglassMediaWidth = BACKGLASS_MEDIA_WIDTH;
    settings.backglassMediaHeight = BACKGLASS_MEDIA_HEIGHT;
    settings.dmdMediaWidth = DMD_MEDIA_WIDTH;
    settings.dmdMediaHeight = DMD_MEDIA_HEIGHT;

    settings.fadeDurationMs = FADE_DURATION_MS;
    settings.fadeTargetAlpha = FADE_TARGET_ALPHA;
    settings.tableChangeSound = TABLE_CHANGE_SOUND;
    settings.tableLoadSound = TABLE_LOAD_SOUND;

    settings.keyPreviousTable = KEY_PREVIOUS_TABLE;
    settings.keyNextTable = KEY_NEXT_TABLE;
    settings.keyFastPrevTable = KEY_FAST_PREV_TABLE;
    settings.keyFastNextTable = KEY_FAST_NEXT_TABLE;
    settings.keyJumpNextLetter = KEY_JUMP_NEXT_LETTER;
    settings.keyJumpPrevLetter = KEY_JUMP_PREV_LETTER;
    settings.keyLaunchTable = KEY_LAUNCH_TABLE;
    settings.keyToggleConfig = KEY_TOGGLE_CONFIG;
    settings.keyQuit = KEY_QUIT;
    settings.keyConfigSave = KEY_CONFIG_SAVE;
    settings.keyConfigClose = KEY_CONFIG_CLOSE;
    settings.keyScreenshotMode = KEY_SCREENSHOT_MODE;
    settings.keyScreenshotKey = KEY_SCREENSHOT_KEY;
    settings.keyScreenshotQuit = KEY_SCREENSHOT_QUIT;
}

const Settings& ConfigManager::getSettings() const {
    return settings;
}

void ConfigManager::notifyConfigChanged() {
    loadConfig(configPath_);
    assetManager_.loadTableAssets(selectedTableIndex_, tables_);
}

void ConfigManager::applyConfigChanges() {
    SDL_SetWindowFullscreen(mainWindow_, settings.mainWindowWidth == 0 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_SetWindowSize(mainWindow_, settings.mainWindowWidth, settings.mainWindowHeight);
}