#ifndef DUMMY_VIDEO_PLAYER_H
#define DUMMY_VIDEO_PLAYER_H

#include "render/ivideo_player.h" // Include the interface definition
#include <iostream>       // For std::cout
#include <string>         // For std::string

/**
 * @brief A dummy implementation of the IVideoPlayer interface for testing and debugging.
 *
 * This class provides a basic implementation of the IVideoPlayer interface,
 * primarily printing log messages to the console to indicate method calls
 * and simulate state changes. It does not perform actual video rendering or audio playback.
 */
class DummyVideoPlayer : public IVideoPlayer {
public:
    /**
     * @brief Constructor for DummyVideoPlayer.
     * Initializes the playing state to false.
     */
    DummyVideoPlayer() : m_isPlaying(false), m_volume(1.0f), m_isMuted(false) {
        std::cout << "[DummyVideoPlayer] Constructor called." << std::endl;
    }

    /**
     * @brief Destructor.
     * Logs the destruction of the dummy player.
     */
    ~DummyVideoPlayer() override {
        std::cout << "[DummyVideoPlayer] Destructor called." << std::endl;
    }

    /**
     * @brief DUMMY: Sets up the video player.
     * Logs the setup parameters and always returns true.
     * @param renderer Ignored.
     * @param path Logs the path.
     * @param width Logs the width.
     * @param height Logs the height.
     * @return true always.
     */
    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override {
        std::cout << "Renderer: " << renderer << "[DummyVideoPlayer] setup() called with path: " << path
                  << ", width: " << width << ", height: " << height << std::endl;
        // In a real implementation, you'd check if renderer is valid, load the video, etc.
        // For dummy, we just assume success.
        return true;
    }

    /**
     * @brief DUMMY: Starts video playback.
     * Sets m_isPlaying to true and logs the action.
     */
    void play() override {
        m_isPlaying = true;
        std::cout << "[DummyVideoPlayer] play() called. Video is now playing." << std::endl;
    }

    /**
     * @brief DUMMY: Stops the video playback.
     * Sets m_isPlaying to false and logs the action.
     */
    void stop() override {
        m_isPlaying = false;
        std::cout << "[DummyVideoPlayer] stop() called. Video is now stopped." << std::endl;
    }

    /**
     * @brief DUMMY: Updates the video state.
     * Logs an update message if the video is playing.
     */
    void update() override {
        if (m_isPlaying) {
            // Simulate frame progression or other updates
            std::cout << "[DummyVideoPlayer] update() called. (Video playing)" << std::endl;
        } else {
            std::cout << "[DummyVideoPlayer] update() called. (Video stopped)" << std::endl;
        }
    }

    /**
     * @brief DUMMY: Retrieves the current video texture.
     * Always returns nullptr as no actual texture is created.
     * @return nullptr always.
     */
    SDL_Texture* getTexture() const override {
        std::cout << "[DummyVideoPlayer] getTexture() called. Returning nullptr." << std::endl;
        return nullptr; // No actual texture in dummy player
    }

    /**
     * @brief DUMMY: Checks if the video is currently playing.
     * Returns the internal m_isPlaying state.
     * @return The current playing state.
     */
    bool isPlaying() const override {
        // std::cout << "[DummyVideoPlayer] isPlaying() called. Current state: " << (m_isPlaying ? "true" : "false") << std::endl;
        return m_isPlaying;
    }

    /**
     * @brief DUMMY: Sets the volume for the video's audio track.
     * Logs the desired volume.
     * @param volume The desired volume.
     */
    void setVolume(float volume) override {
        m_volume = volume;
        std::cout << "[DummyVideoPlayer] setVolume() called. Volume set to: " << volume << std::endl;
    }

    /**
     * @brief DUMMY: Sets the mute state for the video's audio track.
     * Logs the mute state.
     * @param mute True to mute, false to unmute.
     */
    void setMute(bool mute) override {
        m_isMuted = mute;
        std::cout << "[DummyVideoPlayer] setMute() called. Mute state set to: " << (mute ? "true" : "false") << std::endl;
    }

private:
    bool m_isPlaying;
    float m_volume;
    bool m_isMuted;
};

#endif // DUMMY_VIDEO_PLAYER_H