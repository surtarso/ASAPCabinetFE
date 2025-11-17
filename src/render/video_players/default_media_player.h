#ifndef DEFAULT_MEDIA_PLAYER_H
#define DEFAULT_MEDIA_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <cmath>
#include <iostream>
#include <cctype>

/**
 * @brief A procedural "default media" video player with SDL_ttf text rendering.
 * Renders a placeholder frame with border, screen name, NO MEDIA, and a pulsating square.
 */
class DefaultMediaPlayer : public IVideoPlayer {
public:
    DefaultMediaPlayer(SDL_Renderer* renderer, int width, int height,
                       const std::string& fontPath,
                       const std::string& screenName, int fontSize = 24)
        : renderer_(renderer), width_(width), height_(height),
          isPlaying_(false), screenName_(screenName), font_(nullptr)
    {
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width_, height_);

        if (TTF_Init() == -1) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        }

        font_ = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (!font_) {
            std::cerr << "Failed to load font: " << fontPath << " - " << TTF_GetError() << std::endl;
        }

        //Capitalize screen name
        if (!screenName_.empty()) {
            screenName_[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(screenName_[0])));
        }
    }

    ~DefaultMediaPlayer() override {
        if (texture_) SDL_DestroyTexture(texture_);
        if (font_) TTF_CloseFont(font_);
        TTF_Quit();
    }

    bool setup(SDL_Renderer* /*renderer*/, const std::string& /*path*/, int /*w*/, int /*h*/) override {
        return true;
    }

    void play() override { isPlaying_ = true; }
    void stop() override { isPlaying_ = false; }

    void update() override {
        if (!isPlaying_ || !texture_ || !renderer_) return;

        // Set render target to our texture
        SDL_SetRenderTarget(renderer_, texture_);

        // Background
        SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
        SDL_RenderClear(renderer_);

        // Yellow border
        SDL_SetRenderDrawColor(renderer_, 255, 255, 0, 255);
        SDL_Rect border = {0, 0, width_, height_};
        SDL_RenderDrawRect(renderer_, &border);

        // Draw texts first (so square is on top or below)
        if (font_) {
            drawText(screenName_, width_/2, 40, {255, 255, 0, 255}, true);      // center-top
            drawText("NO MEDIA", width_/2, height_/2, {255, 255, 255, 255}, true); // dead-center
        }

        // Bouncing ball animation

        // Calculate responsive floor positions
        float noMediaY = static_cast<float>(height_) / 2.0f;

        const float REFERENCE_HEIGHT = 1080.0f; // Use a typical desktop height as a baseline
        float heightScale = static_cast<float>(height_) / REFERENCE_HEIGHT;

        // Scale the gravity and max bounce height
        const float scaledMaxBounceH = 80.0f * heightScale;
        // const float scaledGravity = 980.0f / heightScale; // Use an inverse scale to keep time consistent

        float floorOffset = 40.0f; // this is the FLOOR (shadow + ball bounce)
        float floorY = noMediaY + scaledMaxBounceH + floorOffset; // Use the scaled value
        const float ballRadius = 20.0f;

        Uint64 now = SDL_GetPerformanceCounter();

        if (init_) {
            // Set initial position and timer, then exit this frame.
            ballY_ = floorY - ballRadius - scaledMaxBounceH; // Start at max height
            velocity_ = 0.0f;
            last_ = now; // Initialize the timer reference

            init_ = false;

            // Immediately reset render target and return to avoid partial draw/physics corruption
            SDL_SetRenderTarget(renderer_, nullptr);
            return;
        }

        // --- Timing and Delta Calculation ---
        float delta = static_cast<float>(now - last_) / static_cast<float>(SDL_GetPerformanceFrequency());
        last_ = now;

        // Cap delta to prevent large jumps after a pause/stutter
        if (delta > 0.1f) delta = 0.1f;
        // ------------------------------------

        const float scaledGravity = 980.0f;  // pixels/s^2
        const float damping = 0.8f;  // energy loss

        // Physics update
        velocity_ += scaledGravity * delta;
        ballY_ += velocity_ * delta;

        // Collision check - uses the same floorY as the shadow
        if (ballY_ > floorY - ballRadius) {
            ballY_ = floorY - ballRadius;
            velocity_ = -velocity_ * damping;
            // The restart velocity should also be scaled inversely
            if (std::abs(velocity_) < 10.0f / heightScale) velocity_ = -500.0f; // / heightScale;
        }

        int centerX = width_/2; // screen center
        int radius = 20;  // ball radius

        // Fake shadow on floor
        float distance = floorY - ballY_;  // larger when higher
        float shadowWidth = 10.0f + (distance / scaledMaxBounceH) * 3.0f;
        float shadowHeight = 5.0f;
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 128);
        for (int dx = -static_cast<int>(shadowWidth); dx <= static_cast<int>(shadowWidth); ++dx) {
            for (int dy = -static_cast<int>(shadowHeight); dy <= static_cast<int>(shadowHeight); ++dy) {
                if ((static_cast<float>(dx*dx) / (shadowWidth*shadowWidth)) + (static_cast<float>(dy*dy) / (shadowHeight*shadowHeight)) <= 1.0f) {
                    SDL_RenderDrawPoint(renderer_, centerX + dx, static_cast<int>(floorY) + dy);
                }
            }
        }

        // Draw silver ball with shine
        SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);  // silver
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx*dx + dy*dy <= radius*radius) {
                    SDL_RenderDrawPoint(renderer_, centerX + dx, static_cast<int>(ballY_) + dy);
                }
            }
        }

        // Simple shine
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_Rect shine = {centerX - 5, static_cast<int>(ballY_) - 10, 10, 5};
        SDL_RenderFillRect(renderer_, &shine);

        // Reset render target to default
        SDL_SetRenderTarget(renderer_, nullptr);
    }

    SDL_Texture* getTexture() const override { return texture_; }
    bool isPlaying() const override { return isPlaying_; }

    void setVolume(float /*volume*/) override {}
    void setMute(bool /*mute*/) override {}

private:
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    int width_, height_;
    bool isPlaying_;
    std::string screenName_;
    TTF_Font* font_;

    float ballY_ = 0.0f;
    float velocity_ = 0.0f;
    bool init_ = true;
    Uint64 last_ = 0;

    void drawText(const std::string& text, int x, int y, SDL_Color color, bool center = true) {
        if (!font_) return;

        SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, text.c_str(), color);
        if (!surface) return;

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_FreeSurface(surface);
        if (!textTexture) return;

        // Make sure texture blend mode is set to blend
        SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);

        SDL_Rect dst;
        dst.w = 0; dst.h = 0;
        SDL_QueryTexture(textTexture, nullptr, nullptr, &dst.w, &dst.h);
        dst.x = center ? x - dst.w / 2 : x;
        dst.y = center ? y - dst.h / 2 : y;

        SDL_RenderCopy(renderer_, textTexture, nullptr, &dst);
        SDL_DestroyTexture(textTexture);
    }
};

#endif // DEFAULT_MEDIA_PLAYER_H
