#ifndef TITLE_RENDERER_H
#define TITLE_RENDERER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <memory>

class IConfigService;

class TitleRenderer {
public:
    explicit TitleRenderer(IConfigService* configService);
    ~TitleRenderer() = default;

    void setTitlePosition(int x, int y);
    void setFont(TTF_Font* font);
    void reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect,
                            SDL_Renderer* playfieldRenderer, SDL_Texture*& playfieldTitleTexture,
                            SDL_Renderer* backglassRenderer, SDL_Texture*& backglassTitleTexture,
                            SDL_Renderer* dmdRenderer, SDL_Texture*& dmdTitleTexture,
                            SDL_Renderer* topperRenderer, SDL_Texture*& topperTitleTexture);
    SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message,
                            SDL_Color color, SDL_Rect& textRect);
    SDL_Rect getTitleRect() const { return titleRect_; }

private:
    TTF_Font* font_;
    IConfigService* configService_;
    SDL_Rect titleRect_;
};

#endif // TITLE_RENDERER_H