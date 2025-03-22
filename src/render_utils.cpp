#include "render_utils.h"
#include <iostream>
#include <stdio.h>  // For stderr redirection

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    // Redirect stderr to /dev/null (Unix) or nul (Windows) during loading
#ifdef _WIN32
    freopen("nul", "w", stderr);  // Windows
#else
    freopen("/dev/null", "w", stderr);  // Linux/Mac
#endif

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    
    // Restore stderr to console
#ifdef _WIN32
    freopen("CON", "w", stderr);  // Windows
#else
    freopen("/dev/tty", "w", stderr);  // Linux/Mac
#endif

    if (!tex) {
        std::cerr << "Failed to load texture " << path << ": " << IMG_GetError() << std::endl;
    }
    return tex;
}

SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        std::cerr << "TTF_RenderUTF8_Blended error: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    textRect.w = surf->w;
    textRect.h = surf->h;
    SDL_FreeSurface(surf);
    return texture;
}