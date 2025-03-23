#include "render/renderer.h"
#include <iostream>
// #include <stdio.h>  // Remove if we ditch stderr redirection

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    // Log the path we’re trying to load
    // std::cout << "Attempting to load texture: " << path << std::endl;

    
    FILE* redirected;
#ifdef _WIN32
    redirected = freopen("nul", "w", stderr);
    if (!redirected) {
        std::cerr << "Warning: Failed to redirect stderr to nul" << std::endl;
    }
#else
    redirected = freopen("/dev/null", "w", stderr);
    if (!redirected) {
        std::cerr << "Warning: Failed to redirect stderr to /dev/null" << std::endl;
    }
#endif
    

    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());

    
    // Restore stderr
    FILE* restored;
#ifdef _WIN32
    restored = freopen("CON", "w", stderr);
    if (!restored) {
        std::cerr << "Warning: Failed to restore stderr to CON" << std::endl;
    }
#else
    restored = freopen("/dev/tty", "w", stderr);
    if (!restored) {
        std::cerr << "Warning: Failed to restore stderr to /dev/tty" << std::endl;
    }
#endif
    

    if (!tex) {
        std::cerr << "Failed to load texture " << path << ": " << IMG_GetError() << std::endl;
    } 
    // else {
    //     std::cout << "Successfully loaded texture: " << path << std::endl;
    // }
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