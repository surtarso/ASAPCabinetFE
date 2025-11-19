// playfield_renderer.h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"

#ifndef PLAYFIELD_RENDERER_H
#define PLAYFIELD_RENDERER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <cmath>

class PlayfieldSDLRenderer {
public:
    PlayfieldSDLRenderer() = default;

    ~PlayfieldSDLRenderer() {
        if (apronFont_) {
            TTF_CloseFont(apronFont_);
            apronFont_ = nullptr;
        }
    }

    // render: main entry
    void render(SDL_Renderer* r,
                const std::string& displayText,
                const std::string& fontPath,
                int w, int h,
                float t,
                const std::string& defaultText)
    {
        if (!r) return;
        const std::string title = displayText.empty() ? defaultText : displayText;

        drawStarfield(r, w, h, t);

        // Playfield rect
        SDL_Rect pf{ w / 12, h / 20, w * 10 / 12, h * 18 / 20 };

        // Glow + outline
        drawGlowRect(r, pf, 50, SDL_Color{255, 180, 60, 0});
        drawGlowRect(r, pf, 16, SDL_Color{255, 220, 100, 80});
        drawRoundedOutline(r, pf, 5, SDL_Color{255, 230, 120, 255});

        drawApronText(r, title, w, h, fontPath);
    }

private:
    // float PI (avoid double-to-float conversions)
    static constexpr float PI_F = 3.14159265358979323846f;

    // apronFont_ caches the loaded font; will reload if fontPath changes
    TTF_Font* apronFont_ = nullptr;
    std::string apronFontPath_;

    // -------------------------
    // Starfield
    // -------------------------
    void drawStarfield(SDL_Renderer* r, int w, int h, float t)
    {
        SDL_SetRenderDrawColor(r, 5, 0, 15, 255);
        SDL_RenderClear(r);

        for (int i = 0; i < 120; ++i) {
            float ti = t * 0.3f + float(i) * 0.7f;
            int x = int(w * (0.5f + 0.5f * std::sin(ti)));
            int y = int(h * (0.5f + 0.5f * std::cos(ti * 1.1f)));
            // clamp alpha explicitly then cast to Uint8
            int alpha = int(80 + 175 * std::sin(ti * 0.4f));
            Uint8 a = static_cast<Uint8>(std::clamp(alpha, 0, 255));
            SDL_SetRenderDrawColor(r, 255, 255, 220, a);
            SDL_RenderDrawPoint(r, x, y);
        }
    }

    // -------------------------
    // Glow & rounded outline
    // -------------------------
    void drawGlowRect(SDL_Renderer* r, SDL_Rect rect, int thick, SDL_Color base)
    {
        if (thick <= 0) return;
        for (int i = thick; i > 0; --i) {
            // clamp computed alpha, avoid narrowing warnings
            int alpha = i * 4;
            base.a = static_cast<Uint8>(std::clamp(alpha, 0, 255));
            SDL_SetRenderDrawColor(r, base.r, base.g, base.b, base.a);
            SDL_Rect big{ rect.x - i, rect.y - i, rect.w + i * 2, rect.h + i * 2 };
            SDL_RenderDrawRect(r, &big);
        }
    }

    void drawRoundedOutline(SDL_Renderer* r, SDL_Rect rect, int thick, SDL_Color c)
    {
        if (thick <= 0) return;
        for (int i = 0; i < thick; ++i) {
            SDL_Color col = c;
            if (i > 0) {
                float alphaScale = 1.0f - float(i) / float(thick);
                int alpha = int(c.a * alphaScale);
                col.a = static_cast<Uint8>(std::clamp(alpha, 0, 255));
            }
            SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);

            SDL_Rect rc{ rect.x - i, rect.y - i, rect.w + i * 2, rect.h + i * 2 };

            // straight edges
            SDL_RenderDrawLine(r, rc.x + 8, rc.y, rc.x + rc.w - 8, rc.y);
            SDL_RenderDrawLine(r, rc.x + 8, rc.y + rc.h - 1, rc.x + rc.w - 8, rc.y + rc.h - 1);
            SDL_RenderDrawLine(r, rc.x, rc.y + 8, rc.x, rc.y + rc.h - 8);
            SDL_RenderDrawLine(r, rc.x + rc.w - 1, rc.y + 8, rc.x + rc.w - 1, rc.y + rc.h - 8);

            // corners
            drawArc(r, rc.x + 8, rc.y + 8, 8, 180.0f, 270.0f);
            drawArc(r, rc.x + rc.w - 9, rc.y + 8, 8, 270.0f, 360.0f);
            drawArc(r, rc.x + rc.w - 9, rc.y + rc.h - 9, 8, 0.0f, 90.0f);
            drawArc(r, rc.x + 8, rc.y + rc.h - 9, 8, 90.0f, 180.0f);
        }
    }

    // -------------------------
    // Arc / circle utilities
    // -------------------------
    void drawArc(SDL_Renderer* r, int cx, int cy, int radius, float startDeg, float endDeg)
    {
        int steps = std::max(6, radius / 2);
        float s = startDeg * (PI_F / 180.0f);
        float e = endDeg   * (PI_F / 180.0f);

        for (int i = 0; i <= steps; ++i) {
            float tt = float(i) / float(steps);
            float a = s + (e - s) * tt;
            int px = int(cx + std::cos(a) * radius);
            int py = int(cy + std::sin(a) * radius);
            SDL_RenderDrawPoint(r, px, py);
        }
    }

    void circle(SDL_Renderer* r, int cx, int cy, int rad)
    {
        for (int i = 0; i <= 64; ++i) {
            float a = (float(i) * PI_F) / 32.0f;
            int x = int(cx + std::cos(a) * rad);
            int y = int(cy + std::sin(a) * rad);
            SDL_RenderDrawPoint(r, x, y);
        }
    }

    void drawGlowCircle(SDL_Renderer* r, int cx, int cy, int rad, SDL_Color c)
    {
        for (int t = rad; t > rad - 20; --t) {
            int alpha = (rad - t) * 12;
            c.a = static_cast<Uint8>(std::clamp(alpha, 0, 255));
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
            circle(r, cx, cy, t);
        }
    }

    void drawStarBumper(SDL_Renderer* r, int cx, int cy, int rad, float pulse)
    {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        circle(r, cx, cy, rad);

        SDL_SetRenderDrawColor(r, 255, 100, 200, 255);
        circle(r, cx, cy, rad - 4);

        float spokes = rad * 1.4f * pulse;
        for (int i = 0; i < 8; ++i) {
            float a = (float(i) * PI_F) / 4.0f;
            int x = int(cx + std::cos(a) * spokes);
            int y = int(cy + std::sin(a) * spokes);
            SDL_RenderDrawLine(r, cx, cy, x, y);
        }
    }

    // -------------------------
    // Text (apron)
    // -------------------------
    void drawApronText(SDL_Renderer* r,
                       const std::string& text,
                       int w, int h,
                       const std::string& fontPath)
    {
        // reload font when fontPath changes
        if (fontPath != apronFontPath_) {
            if (apronFont_) {
                TTF_CloseFont(apronFont_);
                apronFont_ = nullptr;
            }
            apronFontPath_ = fontPath;
        }

        if (!apronFont_) {
            apronFont_ = TTF_OpenFont(fontPath.c_str(), std::max(18, h / 28));
            if (!apronFont_) return;
        }

        SDL_Color c{230, 190, 90, 220};
        SDL_Surface* surf = TTF_RenderUTF8_Blended(apronFont_, text.c_str(), c);
        if (!surf) return;

        SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
        int tw = surf->w, th = surf->h;
        SDL_FreeSurface(surf);

        SDL_Rect dst{ (w - tw) / 2, int(h * 0.08f), tw, th };
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
};

#pragma GCC diagnostic pop
#endif // PLAYFIELD_RENDERER_H
