//file: embedded_fallback.h
#pragma once
#include <cstdint>
#include <cstddef>
#include <SDL.h>
#include <SDL_image.h>


SDL_Texture* loadEmbeddedPNG(SDL_Renderer* renderer,
                             const unsigned char* data,
                             size_t size);

// 128x32 PNGs embedded binary data
extern const unsigned char EMBED_TOPPER_PNG[];
extern const size_t EMBED_TOPPER_PNG_SIZE;

extern const unsigned char EMBED_DMD_PNG[];
extern const size_t EMBED_DMD_PNG_SIZE;
