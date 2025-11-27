// alternative_media_player.h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#ifndef ALTERNATIVE_MEDIA_PLAYER_H
#define ALTERNATIVE_MEDIA_PLAYER_H

#include "render/ivideo_player.h"
#include "dmd_renderer.h"
#include "playfield_renderer.h"
#include "backglass_renderer.h"
#include "embedded_fallbacks.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <cmath>
#include <iostream>

class AlternativeMediaPlayer : public IVideoPlayer {
public:
    AlternativeMediaPlayer(SDL_Renderer* renderer,
                           int width,
                           int height,
                           const std::string& fontPath,
                           const std::string& screenName,
                           const std::string& displayText,
                           DmdSDLRenderer* sharedRenderer = nullptr,
                           int fontSize = 24)
        : renderer_(renderer),
          width_(width),
          height_(height),
          fontPath_(fontPath),
          screenName_(screenName),
          displayText_(displayText),
          defaultText_(),
          isPlaying_(false),
          font_(nullptr),
          texture_(nullptr),
          last_update_time_(0.0f),
          dmdRendererPtr_(sharedRenderer),
          last_counter_(0)
    {
        if (renderer_) {
            texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width_, height_);
        }

        if (TTF_WasInit() == 0) {
            if (TTF_Init() == -1) {
                std::cerr << "TTF_Init failed: " << TTF_GetError() << "\n";
            }
        }

        if (!fontPath_.empty()) {
            font_ = TTF_OpenFont(fontPath_.c_str(), fontSize);
            if (!font_) {
                std::cerr << "Failed to load font: " << fontPath_ << " - " << TTF_GetError() << "\n";
            }
        }

        dmdRendererPtr_ = sharedRenderer;
    }

    ~AlternativeMediaPlayer() override
    {
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
        if (font_) {
            TTF_CloseFont(font_);
            font_ = nullptr;
        }
    }

    bool setup(SDL_Renderer*, const std::string&, int, int) override { return true; }

    void play() override { isPlaying_ = true; }
    void stop() override { isPlaying_ = false; }

    void update() override
    {
        if (!isPlaying_ || !texture_ || !renderer_) return;

        Uint64 now = SDL_GetPerformanceCounter();
        if (last_counter_ == 0) last_counter_ = now;

        float delta = float(now - last_counter_) / float(SDL_GetPerformanceFrequency());
        last_counter_ = now;
        last_update_time_ += delta;

        // render to texture target
        SDL_SetRenderTarget(renderer_, texture_);

        SDL_SetRenderDrawColor(renderer_, 20, 20, 20, 255);
        SDL_RenderClear(renderer_);

        // --- START NEW/MODIFIED LOGIC: Pre-Check Asset Existence ---
        bool assetLookupFailed = false;

        if (screenName_ == "dmd" || screenName_ == "topper") {
            // Check if displayText is an asset ID (i.e., not empty/sentinel)
            if (!displayText_.empty() &&
                displayText_ != "__ALTERNATIVE_MEDIA__" &&
                displayText_ != "__ALTERNATIVE_MEDIA__:")
            {
                DmdSDLRenderer& dmd = dmdRendererPtr_ ? *dmdRendererPtr_ : dmdRenderer_;

                // 1. Try exact lookup (like render does for toppers/non-manufacturer)
                SDL_Texture* foundAsset = dmd.getAsset(displayText_ + ".png");
                if (!foundAsset) {
                    foundAsset = dmd.getAsset(displayText_ + ".gif");
                }

                // 2. Try manufacturer logic (like render does for DMD)
                if (!foundAsset) {
                    std::string manufacturer = displayText_;
                    std::transform(manufacturer.begin(), manufacturer.end(), manufacturer.begin(), ::tolower);

                    foundAsset = dmd.getAsset(manufacturer + ".png");
                    if (!foundAsset) {
                        foundAsset = dmd.getAsset(manufacturer + ".gif");
                    }
                }

                // If after all lookups, the texture is still NULL, the asset is missing.
                if (!foundAsset) {
                    assetLookupFailed = true;
                }
            }
        }
        // --- END NEW/MODIFIED LOGIC ---

        // Embedded fallback images
        if ((screenName_ == "dmd" || screenName_ == "topper") &&
            (displayText_.empty() || displayText_ == "__ALTERNATIVE_MEDIA__" ||
            displayText_ == "__ALTERNATIVE_MEDIA__:" || assetLookupFailed))
        {
            const unsigned char* data = (screenName_ == "dmd") ? EMBED_DMD_PNG : EMBED_TOPPER_PNG;
            size_t size               = (screenName_ == "dmd") ? EMBED_DMD_PNG_SIZE : EMBED_TOPPER_PNG_SIZE;

            SDL_Texture* raw = loadEmbeddedPNG(renderer_, data, size);
            if (raw) {
                DmdSDLRenderer& dmd = dmdRendererPtr_ ? *dmdRendererPtr_ : dmdRenderer_;
                dmd.renderTextureAsDmd(renderer_, raw, width_, height_, last_update_time_);

                SDL_DestroyTexture(raw);
                SDL_SetRenderTarget(renderer_, nullptr);
                return; // Successful embedded image render.
            }
            // If we reach here, we tried to load the embedded image (because assetLookupFailed was true),
            // but loadEmbeddedPNG failed (raw is nullptr).
            // The *only* way to prevent the ID from being printed is to clear the ID now.
            if (assetLookupFailed) {
                displayText_.clear();
            }
        }
        // Embedding end

        if (screenName_ == "dmd") {
            defaultText_ = "INSERT COINS";

            if (dmdRendererPtr_) {
                // Use the injected DmdSDLRenderer to handle asset lookup and rendering
                dmdRendererPtr_->render(renderer_, displayText_, width_, height_, last_update_time_, defaultText_);
            } else {
                dmdRenderer_.render(renderer_, displayText_, width_, height_, last_update_time_, defaultText_);
            }
        }
        else if (screenName_ == "topper") {
            defaultText_ = "ASAPCabinetFE";

            if (dmdRendererPtr_) {
                dmdRendererPtr_->render(renderer_, displayText_, width_, height_, last_update_time_, defaultText_);
            } else {
                dmdRenderer_.render(renderer_, displayText_, width_, height_, last_update_time_, defaultText_);
            }
        }
        else if (screenName_ == "backglass") {
            defaultText_ = "ASAPCabinetFE";
            backglassRenderer_.render(renderer_, last_update_time_, font_, displayText_, width_, height_, defaultText_);
        }
        else if (screenName_ == "playfield") {
            defaultText_ = "ASAPCabinetFE";
            playfieldRenderer_.render(renderer_, displayText_, fontPath_, width_, height_, last_update_time_, defaultText_);
        }
        else {
            SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
            SDL_Rect box{ 40, 40, 80, 80 };
            SDL_RenderDrawRect(renderer_, &box);
        }

        SDL_SetRenderTarget(renderer_, nullptr);
    }

    SDL_Texture* getTexture() const override { return texture_; }
    bool isPlaying() const override { return isPlaying_; }
    void setVolume(float) override {}
    void setMute(bool) override {}

private:
    SDL_Renderer* renderer_;
    int width_, height_;
    std::string fontPath_;
    std::string screenName_;
    std::string displayText_;
    std::string defaultText_;
    bool isPlaying_;
    TTF_Font* font_;
    SDL_Texture* texture_;
    float last_update_time_;
    DmdSDLRenderer* dmdRendererPtr_;
    Uint64 last_counter_;

    DmdSDLRenderer dmdRenderer_;
    PlayfieldSDLRenderer playfieldRenderer_;
    BackglassSDLRenderer backglassRenderer_;
};

#pragma GCC diagnostic pop
#endif // ALTERNATIVE_MEDIA_PLAYER_H
