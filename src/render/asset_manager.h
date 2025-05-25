/**
 * @file asset_manager.h
 * @brief Defines the AssetManager class for managing VPX table assets in ASAPCabinetFE.
 *
 * This header provides the AssetManager class, which implements the IAssetManager interface
 * to manage textures, video players, audio, and table data for Visual Pinball X (VPX) tables.
 * It handles asset loading, caching, and rendering resources using SDL and TTF.
 */
#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "render/itable_loader.h"
#include "render/table_data.h"
#include "render/ivideo_player.h"
#include "render/iasset_manager.h"
#include "sound/isound_manager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class IWindowManager
 * @brief Interface for window management (forward declaration).
 */
class IWindowManager;

/**
 * @class AssetManager
 * @brief Manages textures, video players, audio, and table assets for VPX rendering.
 *
 * This class implements the IAssetManager interface to load, cache, and provide access
 * to textures, video players, music, and table data for playfield, backglass, and DMD displays.
 * It interfaces with IConfigService for settings, ITableLoader for table data, ISoundManager
 * for audio, and SDL/TTF for rendering resources.
 */
class AssetManager : public IAssetManager {
public:
    /**
     * @brief Constructs an AssetManager instance with specified renderers, font, and sound manager.
     *
     * Initializes the asset manager with SDL renderers for playfield, backglass, and
     * DMD displays, a TTF font for text rendering, and a sound manager for audio playback.
     *
     * @param playfield The SDL renderer for the playfield display.
     * @param backglass The SDL renderer for the backglass display.
     * @param dmd The SDL renderer for the DMD display.
     * @param f The TTF font for rendering text.
     * @param soundManager The sound manager for audio playback.
     */
    AssetManager(SDL_Renderer* playfield, SDL_Renderer* backglass, SDL_Renderer* dmd, TTF_Font* f, ISoundManager* soundManager);

    /**
     * @brief Gets the playfield texture.
     * @return The SDL texture for the playfield display.
     */
    SDL_Texture* getPlayfieldTexture() override { return playfieldTexture.get(); }

    /**
     * @brief Gets the wheel texture.
     * @return The SDL texture for the wheel display.
     */
    SDL_Texture* getWheelTexture() override { return wheelTexture.get(); }

    /**
     * @brief Gets the backglass texture.
     * @return The SDL texture for the backglass display.
     */
    SDL_Texture* getBackglassTexture() override { return backglassTexture.get(); }

    /**
     * @brief Gets the DMD texture.
     * @return The SDL texture for the DMD display.
     */
    SDL_Texture* getDmdTexture() override { return dmdTexture.get(); }

    /**
     * @brief Gets the title texture.
     * @return The SDL texture for the title text.
     */
    SDL_Texture* getTitleTexture() override { return titleTexture.get(); }

    /**
     * @brief Gets the playfield video player.
     * @return The video player for the playfield display.
     */
    IVideoPlayer* getPlayfieldVideoPlayer() override { return playfieldVideoPlayer.get(); }

    /**
     * @brief Gets the backglass video player.
     * @return The video player for the backglass display.
     */
    IVideoPlayer* getBackglassVideoPlayer() override { return backglassVideoPlayer.get(); }

    /**
     * @brief Gets the DMD video player.
     * @return The video player for the DMD display.
     */
    IVideoPlayer* getDmdVideoPlayer() override { return dmdVideoPlayer.get(); }

    /**
     * @brief Gets the configuration service.
     * @return The configuration service for accessing settings.
     */
    IConfigService* getSettingsManager() override { return configManager_; }

    /**
     * @brief Gets the title text rectangle.
     * @return The SDL rectangle defining the title text position and size.
     */
    SDL_Rect getTitleRect() override { return titleRect; }

    /**
     * @brief Sets the position of the title text.
     *
     * Updates the title rectangle's x and y coordinates.
     *
     * @param x The x-coordinate of the title text.
     * @param y The y-coordinate of the title text.
     */
    void setTitlePosition(int x, int y) override;

    /**
     * @brief Sets the font for text rendering.
     *
     * Updates the TTF font used for rendering title text.
     *
     * @param font The TTF font to use.
     */
    void setFont(TTF_Font* font) override;

    /**
     * @brief Reloads the title texture with new text and color.
     *
     * Generates a new texture for the title text with the specified message and color,
     * updating the title rectangle.
     *
     * @param title The text to render.
     * @param color The SDL color for the text.
     * @param titleRect The rectangle to store the text's position and size.
     */
    void reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) override;

    /**
     * @brief Reloads all assets for a set of tables.
     *
     * Loads textures and video players for the specified table index from the provided
     * table list, using the window manager and font for rendering.
     *
     * @param windowManager The window manager for renderer access.
     * @param font The TTF font for text rendering.
     * @param tables The list of table data.
     * @param index The index of the table to load assets for.
     */
    void reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) override;

    /**
     * @brief Sets the configuration service.
     *
     * Updates the configuration service used for accessing settings.
     *
     * @param cm The configuration service to set.
     */
    void setSettingsManager(IConfigService* cm) override;

    /**
     * @brief Loads assets for a specific table.
     *
     * Loads textures and video players for the table at the specified index from the
     * provided table list.
     *
     * @param index The index of the table to load assets for.
     * @param tables The list of table data.
     */
    void loadTableAssets(size_t index, const std::vector<TableData>& tables) override;

    /**
     * @brief Clears old video players from the cache.
     *
     * Removes all video players stored in the old video players cache.
     */
    void clearOldVideoPlayers() override;

    /**
     * @brief Cleans up all video players.
     *
     * Releases all active video players (playfield, backglass, DMD) and clears the cache.
     */
    void cleanupVideoPlayers() override;

    /**
     * @brief Sets the sound manager for audio playback.
     *
     * Updates the sound manager used for playing table music.
     *
     * @param soundManager The sound manager to set.
     */
    void setSoundManager(ISoundManager* soundManager) override;

    /**
     * @brief Plays table-specific music.
     *
     * Plays the music associated with the specified table index, looping as needed.
     * If the music path is empty, stops table music and resumes ambience music.
     *
     * @param index The index of the table to play music for.
     * @param tables The vector of table data with music paths.
     */
    void playTableMusic(size_t index, const std::vector<TableData>& tables) override;

    /**
     * @brief Applies audio settings to all active video players.
     *
     * Updates the volume and mute state of playfield, backglass, and DMD video players
     * based on the current mediaAudioVol and mediaAudioMute settings.
     */
    void applyVideoAudioSettings() override;

    /**
     * @brief Adds a video player to the old players cache.
     *
     * Stores a video player in the cache for later cleanup.
     *
     * @param player The video player to add.
     */
    void addOldVideoPlayer(std::unique_ptr<IVideoPlayer> player);

    /**
     * @brief Clears the video cache.
     *
     * Resets cached video paths and dimensions to force reloading of video assets.
     */
    void clearVideoCache();

    /**
     * @brief Loads an SDL texture from a file.
     *
     * Creates an SDL texture from the specified image file using the provided renderer.
     *
     * @param renderer The SDL renderer to use.
     * @param path The file path to the image.
     * @return The loaded SDL texture, or nullptr on failure.
     */
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);

    /**
     * @brief Renders text to an SDL texture.
     *
     * Creates an SDL texture from the specified text using the provided font and color,
     * updating the text rectangle with the texture's dimensions.
     *
     * @param renderer The SDL renderer to use.
     * @param font The TTF font for rendering.
     * @param message The text to render.
     * @param color The SDL color for the text.
     * @param textRect The rectangle to store the text's position and size.
     * @return The rendered SDL texture, or nullptr on failure.
     */
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);

    /**
     * @brief Gets the playfield renderer.
     * @return The SDL renderer for the playfield display.
     */
    SDL_Renderer* getPlayfieldRenderer() { return playfieldRenderer; }

    /**
     * @brief Gets the backglass renderer.
     * @return The SDL renderer for the backglass display.
     */
    SDL_Renderer* getBackglassRenderer() { return backglassRenderer; }

    /**
     * @brief Gets the DMD renderer.
     * @return The SDL renderer for the DMD display.
     */
    SDL_Renderer* getDMDRenderer() { return dmdRenderer; }

    /**
     * @brief Sets the playfield renderer.
     * @param renderer The SDL renderer to set for the playfield.
     */
    void setPlayfieldRenderer(SDL_Renderer* renderer) { playfieldRenderer = renderer; }

    /**
     * @brief Sets the backglass renderer.
     * @param renderer The SDL renderer to set for the backglass.
     */
    void setBackglassRenderer(SDL_Renderer* renderer) { backglassRenderer = renderer; }

    /**
     * @brief Sets the DMD renderer.
     * @param renderer The SDL renderer to set for the DMD.
     */
    void setDMDRenderer(SDL_Renderer* renderer) { dmdRenderer = renderer; }

    // Smart pointers for textures
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> playfieldTexture; ///< Texture for the playfield display.
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> wheelTexture;     ///< Texture for the wheel display.
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> backglassTexture; ///< Texture for the backglass display.
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> dmdTexture;       ///< Texture for the DMD display.
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> titleTexture;     ///< Texture for the title text.

    SDL_Rect titleRect; ///< Rectangle defining the title text position and size.
    std::unique_ptr<IVideoPlayer> playfieldVideoPlayer;  ///< Video player for the playfield display.
    std::unique_ptr<IVideoPlayer> backglassVideoPlayer;  ///< Video player for the backglass display.
    std::unique_ptr<IVideoPlayer> dmdVideoPlayer;        ///< Video player for the DMD display.

private:
    SDL_Renderer* playfieldRenderer; ///< SDL renderer for the playfield display.
    SDL_Renderer* backglassRenderer; ///< SDL renderer for the backglass display.
    SDL_Renderer* dmdRenderer;       ///< SDL renderer for the DMD display.
    ISoundManager* soundManager_;    ///< Sound manager for audio playback.
    std::string currentPlayfieldVideoPath_;  ///< Cached path to the current playfield video.
    std::string currentBackglassVideoPath_;  ///< Cached path to the current backglass video.
    std::string currentDmdVideoPath_;        ///< Cached path to the current DMD video.
    TTF_Font* font;                          ///< TTF font for text rendering.
    IConfigService* configManager_;          ///< Configuration service for settings.
    std::unique_ptr<ITableLoader> tableLoader_; ///< Table loader for accessing table data.
    std::vector<std::unique_ptr<IVideoPlayer>> oldVideoPlayers_; ///< Cache of old video players for cleanup.

    // Caching for image textures
    std::string currentPlayfieldImagePath_;  ///< Cached path to the current playfield image.
    std::string currentWheelImagePath_;      ///< Cached path to the current wheel image.
    std::string currentBackglassImagePath_;  ///< Cached path to the current backglass image.
    std::string currentDmdImagePath_;        ///< Cached path to the current DMD image.

    // Track video settings for reuse
    int currentPlayfieldMediaWidth_ = 0;     ///< Cached width of the playfield video.
    int currentPlayfieldMediaHeight_ = 0;    ///< Cached height of the playfield video.
    int currentBackglassMediaWidth_ = 0;     ///< Cached width of the backglass video.
    int currentBackglassMediaHeight_ = 0;    ///< Cached height of the backglass video.
    int currentDmdMediaWidth_ = 0;           ///< Cached width of the DMD video.
    int currentDmdMediaHeight_ = 0;          ///< Cached height of the DMD video.
};

#endif // ASSET_MANAGER_H