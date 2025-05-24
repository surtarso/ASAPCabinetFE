#ifndef IVIDEO_PLAYER_H
#define IVIDEO_PLAYER_H

#include <SDL2/SDL.h>
#include <string>

/**
 * @brief Interface for a video player.
 *
 * This abstract class defines the necessary methods for any video player implementation.
 * It ensures that any concrete class provides functionality to load, play, stop, update, and retrieve
 * video textures along with the playing state. It also includes methods for audio control.
 *
 * @note All implementations should handle resource management appropriately.
 *
 * @interface IVideoPlayer
 * @see SDL_Renderer, SDL_Texture
 */
class IVideoPlayer {
public:
    /**
     * @brief Destructor.
     *
     * Virtual destructor to ensure derived class destructors are called correctly.
     */
    virtual ~IVideoPlayer() = default;

    /**
     * @brief Sets up the video player.
     *
     * Configures the video player with the given renderer, video file path, and display dimensions.
     *
     * @param renderer Pointer to an SDL_Renderer used for rendering.
     * @param path File path of the video to be played.
     * @param width Desired width of the video.
     * @param height Desired height of the video.
     * @return true if the setup is successful; false otherwise.
     */
    virtual bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) = 0;

    /**
     * @brief Starts video playback.
     *
     * Initiates the video playback. The video player should begin rendering as soon as play is called.
     */
    virtual void play() = 0;

    /**
     * @brief Stops the video playback.
     *
     * Halts any video activity and can reset player states if necessary.
     */
    virtual void stop() = 0;

    /**
     * @brief Updates the video state.
     *
     * Handles the periodic updates for the video player, such as frame progression.
     * Should be called in the game/render loop.
     */
    virtual void update() = 0;

    /**
     * @brief Retrieves the current video texture.
     *
     * Returns the SDL_Texture representing the current frame of the video.
     *
     * @return SDL_Texture pointer if available; otherwise, may return nullptr.
     */
    virtual SDL_Texture* getTexture() const = 0;

    /**
     * @brief Checks if the video is currently playing.
     *
     * Evaluates whether the video player is actively playing the video.
     *
     * @return true if the video is being played; false otherwise.
     */
    virtual bool isPlaying() const = 0;

    /**
     * @brief Sets the volume for the video's audio track.
     * @param volume The desired volume, typically a float between 0.0 (silent) and 1.0 (full volume).
     */
    virtual void setVolume(float volume) = 0;

    /**
     * @brief Sets the mute state for the video's audio track.
     * @param mute True to mute the audio, false to unmute.
     */
    virtual void setMute(bool mute) = 0;
};

#endif // IVIDEO_PLAYER_H
