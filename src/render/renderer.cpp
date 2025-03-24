#include "render/renderer.h"
#include "logging.h"
#include <iostream>
#include <stdio.h>  // stderr redirection

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    // Log the path we’re trying to load
    LOG_DEBUG("Attempting to load texture: " << path);

    
    FILE* redirected;
#ifdef _WIN32
    redirected = freopen("nul", "w", stderr);
    if (!redirected) {
        std::cerr << "Warning: Failed to redirect stderr to nul" << std::endl;
    }
#else
    redirected = freopen("/dev/null", "w", stderr);
    if (!redirected) {
        LOG_DEBUG("Warning: Failed to redirect stderr to /dev/null"); 
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
        LOG_DEBUG("Warning: Failed to restore stderr to /dev/tty"); 
    }
#endif
    

    if (!tex) {
        LOG_DEBUG("Failed to load texture " << path << ": " << IMG_GetError()); 
    } 
    else {
        LOG_DEBUG("Successfully loaded texture: " << path);
    }
    return tex;
}

SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& message, SDL_Color color, SDL_Rect& textRect) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        LOG_DEBUG("TTF_RenderUTF8_Blended error: " << TTF_GetError()); 
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    textRect.w = surf->w;
    textRect.h = surf->h;
    SDL_FreeSurface(surf);
    return texture;
}