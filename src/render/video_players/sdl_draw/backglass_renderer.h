// backglass_renderer.h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#ifndef BACKGLASS_RENDERER_H
#define BACKGLASS_RENDERER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <cmath>
#include <algorithm>

class BackglassSDLRenderer {
public:
    void render(SDL_Renderer* r,
                float t,
                TTF_Font* font,
                const std::string& displayText,
                int w,
                int h,
                const std::string& defaultText)
    {
        if (!r) return;
        const std::string title = displayText.empty() ? defaultText : displayText;

        drawStarfield(r, w, h, t);

        SDL_Rect glass{ w / 12, h / 12, w * 10 / 12, h * 10 / 12 };

        drawGlowRect(r, glass, 50, SDL_Color{255, 180, 60, 0});
        drawGlowRect(r, glass, 16, SDL_Color{255, 220, 100, 80});
        drawRoundedRect(r, glass, 5, SDL_Color{255, 230, 120, 255});

        drawText(r, title.c_str(), w / 2, int(h * 0.28f), font, SDL_Color{255, 220, 80, 255});

        if (fmod(t, 1.3f) < 0.7f) {
            drawText(r, "INSERT COIN", w / 2, int(h * 0.75f), font, SDL_Color{255, 180, 40, 255});
        }
    }

private:
    static constexpr float PI_F = 3.14159265358979323846f;

    void drawStarfield(SDL_Renderer* r, int w, int h, float t)
    {
        SDL_SetRenderDrawColor(r, 5, 0, 15, 255);
        SDL_RenderClear(r);

        for (int i = 0; i < 120; ++i) {
            float ti = t * 0.3f + float(i) * 0.7f;
            int x = int(w * (0.5f + 0.5f * std::sin(ti)));
            int y = int(h * (0.5f + 0.5f * std::cos(ti * 1.1f)));
            int alpha = int(80 + 175 * std::sin(ti * 0.4f));
            Uint8 a = static_cast<Uint8>(std::clamp(alpha, 0, 255));
            SDL_SetRenderDrawColor(r, 255, 255, 220, a);
            SDL_RenderDrawPoint(r, x, y);
        }
    }

    void drawText(SDL_Renderer* r, const char* txt, int x, int y, TTF_Font* f, SDL_Color c)
    {
        if (!f) return;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(f, txt, c);
        if (!surf) return;

        SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
        SDL_Rect dst{ x - surf->w / 2, y - surf->h / 2, surf->w, surf->h };
        SDL_FreeSurface(surf);

        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }

    void drawGlowRect(SDL_Renderer* r, SDL_Rect rect, int thick, SDL_Color base)
    {
        if (thick <= 0) return;
        for (int i = thick; i > 0; --i) {
            int alpha = i * 5;
            base.a = static_cast<Uint8>(std::clamp(alpha, 0, 255));
            SDL_SetRenderDrawColor(r, base.r, base.g, base.b, base.a);
            SDL_Rect big{ rect.x - i, rect.y - i, rect.w + i * 2, rect.h + i * 2 };
            SDL_RenderDrawRect(r, &big);
        }
    }

    void drawRoundedRect(SDL_Renderer* r, SDL_Rect rect, int thick, SDL_Color c)
    {
        if (thick <= 0) return;
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        for (int i = 0; i < thick; ++i) {
            SDL_Rect rc{ rect.x + i, rect.y + i, rect.w - i * 2, rect.h - i * 2 };
            SDL_RenderDrawRect(r, &rc);
        }
    }
};

#pragma GCC diagnostic pop
#endif // BACKGLASS_RENDERER_H
