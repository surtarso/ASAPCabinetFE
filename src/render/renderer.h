#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect);

#endif