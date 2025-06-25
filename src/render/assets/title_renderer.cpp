/**
 * @file title_renderer.cpp
 * @brief Implementation of the TitleRenderer class for rendering text titles on SDL windows.
 */

#include "title_renderer.h"
#include "config/iconfig_service.h"
#include "log/logging.h"
#include <string>

TitleRenderer::TitleRenderer(IConfigService* configService)
    : font_(nullptr), configService_(configService), titleRect_{0, 0, 0, 0} {
}

void TitleRenderer::setTitlePosition(int x, int y) {
    titleRect_.x = x;
    titleRect_.y = y;
    LOG_DEBUG("Updated title position to x=" + std::to_string(x) + ", y=" + std::to_string(y));
}

void TitleRenderer::setFont(TTF_Font* font) {
    font_ = font;
    //LOG_DEBUG("Font set to " + std::to_string(reinterpret_cast<uintptr_t>(font)));
}

void TitleRenderer::reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect,
                                      SDL_Renderer* playfieldRenderer, SDL_Texture*& playfieldTitleTexture,
                                      SDL_Renderer* backglassRenderer, SDL_Texture*& backglassTitleTexture,
                                      SDL_Renderer* dmdRenderer, SDL_Texture*& dmdTitleTexture,
                                      SDL_Renderer* topperRenderer, SDL_Texture*& topperTitleTexture) {
    const Settings& settings = configService_ ? configService_->getSettings() : Settings();

    struct WindowTitleInfo {
        SDL_Renderer* renderer;
        SDL_Texture*& texture;
        const std::string& windowName;
    };

    WindowTitleInfo windows[] = {
        {playfieldRenderer, playfieldTitleTexture, "playfield"},
        {backglassRenderer, backglassTitleTexture, "backglass"},
        {dmdRenderer, dmdTitleTexture, "dmd"},
        {topperRenderer, topperTitleTexture, "topper"}
    };

    for (auto& w : windows) {
        w.texture = nullptr;
        if (w.renderer && font_ && settings.showTitle && settings.titleWindow == w.windowName) {
            titleRect_.x = titleRect.x;
            titleRect_.y = titleRect.y;
            titleRect_.w = 0;
            titleRect_.h = 0;

            w.texture = renderText(w.renderer, font_, title, color, titleRect_);
            int texWidth = 0, texHeight = 0;
            if (w.texture) {
                SDL_QueryTexture(w.texture, nullptr, nullptr, &texWidth, &texHeight);
                titleRect.w = titleRect_.w;
                titleRect.h = titleRect_.h;
            }
            LOG_DEBUG(w.windowName + " title texture reloaded, font=" + std::to_string(reinterpret_cast<uintptr_t>(font_)) +
                      ", font_height=" + std::to_string(font_ ? TTF_FontHeight(font_) : 0) +
                      ", width=" + std::to_string(texWidth) + ", height=" + std::to_string(texHeight));
        }
    }
}

SDL_Texture* TitleRenderer::renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message,
                                       SDL_Color color, SDL_Rect& textRect) {
    if (!font || !renderer || message.empty()) {
        LOG_ERROR("Invalid font, renderer, or empty message for renderText");
        return nullptr;
    }

    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        LOG_ERROR("TTF_RenderUTF8_Blended error: " + std::string(TTF_GetError()));
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture) {
        LOG_ERROR("SDL_CreateTextureFromSurface error: " + std::string(SDL_GetError()));
    } else {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textRect.w = surf->w;
        textRect.h = surf->h;
    }

    SDL_FreeSurface(surf);
    return texture;
}