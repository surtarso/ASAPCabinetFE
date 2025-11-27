#include "dmd_renderer.h"
#include "log/logging.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <vector>

namespace fs = std::filesystem;

// =========================================================================
// 1. Static DMD Font Definition
// =========================================================================

// Define the static member map exactly once in the CPP file.
// 9-high (bits 0-8) by 5-wide character bitmap data.
const std::map<char, std::array<uint16_t, 5>> DmdSDLRenderer::DMD_FONT = {
    // A (Single-line structure)
    {'A', {0x1fe, 0x011, 0x011, 0x011, 0x1fe}},
    // B
    {'B', {0x1ff, 0x111, 0x111, 0x111, 0x0fe}},
    // C
    {'C', {0x1FE, 0x101, 0x101, 0x101, 0x101}},
    // D
    {'D', {0x1ff, 0x101, 0x101, 0x101, 0x0fe}},
    // E
    {'E', {0x1ff, 0x109, 0x109, 0x109, 0x101}},
    // F
    {'F', {0x1ff, 0x009, 0x009, 0x009, 0x001}},
    // G
    {'G', {0x0ff, 0x101, 0x111, 0x111, 0x0f0}},
    // H
    {'H', {0x1ff, 0x010, 0x010, 0x010, 0x1ff}},
    // I
    {'I', {0x101, 0x101, 0x1ff, 0x101, 0x101}},
    // J
    {'J', {0x081, 0x101, 0x101, 0x0FF, 0x001}},
    // K
    {'K', {0x1FF, 0x038, 0x04C, 0x086, 0x103}},
    // L
    {'L', {0x1ff, 0x100, 0x100, 0x100, 0x100}},
    // M
    {'M', {0x1ff, 0x006, 0x018, 0x006, 0x1ff}},
    // N
    {'N', {0x1ff, 0x00c, 0x010, 0x020, 0x1ff}},
    // O
    {'O', {0x1fe, 0x101, 0x101, 0x101, 0x1fe}},
    // P
    {'P', {0x1ff, 0x011, 0x011, 0x011, 0x00e}},
    // Q
    {'Q', {0x7C, 0x183, 0x1A3, 0x43, 0x1BC}},
    // R
    {'R', {0x1ff, 0x011, 0x011, 0x011, 0x1ee}},
    // S
    {'S', {0x8E, 0x111, 0x111, 0x111, 0xE2}},
    // T
    {'T', {0x001, 0x001, 0x1ff, 0x001, 0x001}},
    // U
    {'U', {0x7F, 0x180, 0x180, 0x180, 0x7F}},
    // V
    {'V', {0x3F, 0x40, 0x180, 0x40, 0x3F}},
    // W
    {'W', {0x1FF, 0x100, 0x080, 0x100, 0x1FF}},
    // X
    {'X', {0x1C7, 0x28, 0x10, 0x28, 0x1C7}},
    // Y
    {'Y', {0x007, 0x008, 0x1f0, 0x008, 0x007}},
    // Z
    {'Z', {0x1C3, 0x1A3, 0x193, 0x18B, 0x187}},

    // Numbers
    {'0', {0x1fe, 0x101, 0x101, 0x101, 0x1fe}},
    {'1', {0x1, 0x3, 0x1FF, 0x100, 0x100}},
    {'2', {0x186, 0x141, 0x121, 0x111, 0x10E}},
    {'3', {0xC6, 0x101, 0x119, 0x119, 0xE6}},
    {'4', {0x3C, 0x22, 0x21, 0x1FF, 0x20}},
    {'5', {0x8F, 0x109, 0x109, 0x109, 0xF1}},
    {'6', {0xFE, 0x111, 0x111, 0x111, 0xE2}},
    {'7', {0x001, 0x001, 0x1c1, 0x021, 0x01f}},
    {'8', {0x1fe, 0x111, 0x111, 0x111, 0x1fe}},
    {'9', {0x8E, 0x111, 0x111, 0x111, 0xFE}},

    // Punctuation and Symbols
    {'-', {0x010, 0x010, 0x010, 0x010, 0x010}},
    {'.', {0x180, 0x180, 0x000, 0x000, 0x000}},
    {' ', {0x000, 0x000, 0x000, 0x000, 0x000}},
    {':', {0x000, 0x088, 0x088, 0x000, 0x000}},
    {'/', {0x180, 0x60, 0x18, 0x6, 0x1}}
};

// =========================================================================
// 2. Class Method Implementations (Init, Load, Get)
// =========================================================================

DmdSDLRenderer::DmdSDLRenderer() {}

DmdSDLRenderer::~DmdSDLRenderer() {
    assetCache_.clear();
}

void DmdSDLRenderer::loadAssetsFromDirectory(const std::string& directoryPath, SDL_Renderer* renderer) {
    if (!renderer) {
        LOG_ERROR("Cannot load assets: Renderer is null.");
        return;
    }

    LOG_DEBUG("Loading DMD assets from directory: " + directoryPath);

    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string filename = entry.path().filename().string();

                if (assetCache_.count(filename)) {
                    LOG_WARN("Skipping duplicate DMD asset: " + filename);
                    continue;
                }

                // 1. Load the image into an SDL_Surface
                SDL_Surface* tempSurface = IMG_Load(path.c_str());
                if (!tempSurface) {
                    LOG_ERROR("Failed to load surface for asset " + path + ": " + std::string(IMG_GetError()));
                    continue;
                }

                // 2. Convert the surface to a known, 32-bit format. RGBA8888 is a good robust choice.
                SDL_Surface* surface = SDL_ConvertSurfaceFormat(tempSurface, SDL_PIXELFORMAT_RGBA8888, 0);
                SDL_FreeSurface(tempSurface);
                if (!surface) {
                    LOG_ERROR("Failed to convert surface format for asset " + path + ": " + std::string(SDL_GetError()));
                    continue;
                }

                // 3. Create a new SDL_Texture explicitly in a known, lockable format
                SDL_Texture* tex = SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_RGBA8888,   // Force a guaranteed 32-bit format
                    SDL_TEXTUREACCESS_STREAMING,
                    surface->w,
                    surface->h
                );

                if (!tex) {
                    LOG_ERROR("Failed to create streaming texture for asset " + path + ": " + std::string(SDL_GetError()));
                    SDL_FreeSurface(surface);
                    continue;
                }

                // Convert source surface to RGBA8888 if needed
                SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
                SDL_FreeSurface(surface);

                if (!converted) {
                    LOG_ERROR("Failed to convert surface to RGBA8888 for " + path + ": " + std::string(SDL_GetError()));
                    SDL_DestroyTexture(tex);
                    continue;
                }

                // 4. Copy pixel data into the streaming texture
                if (SDL_UpdateTexture(tex, NULL, converted->pixels, converted->pitch) != 0) {
                    LOG_ERROR("Failed to update texture with surface data for " + filename + ": " + std::string(SDL_GetError()));
                }
                // else {
                //     LOG_DEBUG("Texture updated successfully for " + filename);
                // }

                SDL_FreeSurface(converted);

                // Final cache insertion
                bool isAnimated = (filename.length() > 4 && filename.substr(filename.length() - 4) == ".gif");

                assetCache_.emplace(filename, std::make_pair(SdlTexturePtr(tex, SDL_DestroyTexture), isAnimated));
                LOG_DEBUG("Loaded DMD asset: " + filename + (isAnimated ? " (Animated)" : " (Still)"));
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Filesystem error while loading DMD assets from " + directoryPath + ": " + std::string(e.what()));
    }
}

SDL_Texture* DmdSDLRenderer::getAsset(const std::string& name) const {
    auto it = assetCache_.find(name);
    if (it != assetCache_.end()) {
        return it->second.first.get();
    }
    return nullptr;
}

// =========================================================================
// 3. Asset Masking Implementation
// =========================================================================

void DmdSDLRenderer::drawDmdAssetMasked(SDL_Renderer* renderer, SDL_Texture* assetTexture, int width, int height, float time)
{
    if (!assetTexture) return;

    // --- Setup: Query Original Texture Dimensions and Format ---
    // ... (unchanged)
    Uint32 textureFormat;
    int texW = 0, texH = 0;
    if (SDL_QueryTexture(assetTexture, &textureFormat, nullptr, &texW, &texH) != 0) {
        LOG_ERROR("Failed to query asset texture: " + std::string(SDL_GetError()));
        return;
    }

    // LOG_DEBUG("DMD Asset dimensions queried: W=" + std::to_string(texW) + ", H=" + std::to_string(texH));
    if (texW == 0 || texH == 0) return;

    // --- CRITICAL CHANGE: Create a target surface/buffer for sampling in a known format (ARGB8888) ---
    SDL_Surface* tempSurface = SDL_CreateRGBSurfaceWithFormat(0, texW, texH, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!tempSurface) {
        LOG_ERROR("Failed to create temporary surface: " + std::string(SDL_GetError()));
        return;
    }

    // --- Step 1: Clear Background ---
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);


    // --- Step 2: Sample the original asset texture into the temporary surface ---

    // Copy the texture content onto the rendering target (THIS RENDERS THE SMALL LOGO)
    SDL_Rect src_rect = {0, 0, texW, texH};
    SDL_RenderCopy(renderer, assetTexture, &src_rect, &src_rect);

    // Read the pixels from the renderer target back into the temporary surface (FOR SAMPLING)
    if (SDL_RenderReadPixels(renderer, &src_rect, tempSurface->format->format,
                             tempSurface->pixels, tempSurface->pitch) != 0) {
        LOG_ERROR("Failed to read pixels from renderer: " + std::string(SDL_GetError()));
        SDL_FreeSurface(tempSurface);
        return;
    }

    // *** CLEAR THE RENDERER AGAIN TO WIPE THE SMALL LOGO COPY ***
    // We must reset the renderer to the background color before we draw the dots.
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // --- Step 3: Draw the DMD Dots by sampling the surface data ---

    const int ROWS = 32, COLS = 128;
    const int BRIGHTNESS_THRESHOLD = 25;

    int ppd = std::max(1, std::min(width / COLS, height / ROWS));
    int radius = std::max(1, (ppd / 2) - 2);
    int startX = width  / 2 - COLS * ppd / 2;
    int startY = height / 2 - ROWS * ppd / 2;

    float pulse = (std::sin(time * 7.0f) + 1.0f) * 0.5f;
    Uint8 glow = 60 + static_cast<Uint8>(pulse * 100);

    Uint32* surface_pixels = (Uint32*)tempSurface->pixels;
    int pixelPitch = tempSurface->pitch / sizeof(Uint32);

    bool litDotFound = false;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int cx = startX + c * ppd + ppd / 2;
            int cy = startY + r * ppd + ppd / 2;

            // 1. Unlit background dot (This is the background, so it draws over the previous clear)
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            drawFilledCircle(renderer, cx, cy, radius);

            // 2. Sample the pixel from the temporary surface
            int tx = c;
            int ty = r;
            if (tx >= texW || ty >= texH || tx < 0 || ty < 0) { continue; }

            Uint32 pix = surface_pixels[ty * pixelPitch + tx];
            Uint8 pr, pg, pb, pa;
            SDL_GetRGBA(pix, tempSurface->format, &pr, &pg, &pb, &pa);
            int brightness = (pr + pg + pb) / 3;

            // *** CRITICAL DEBUGGING LOG *** (Leave this for now)
            // if (r == 16 && c == 64) {
            //     LOG_DEBUG("CENTER PIXEL SAMPLE (READPIXELS) - R:" + std::to_string(pr) +
            //               ", G:" + std::to_string(pg) +
            //               ", B:" + std::to_string(pb) +
            //               ", A:" + std::to_string(pa) +
            //               ", Brightness:" + std::to_string(brightness));
            // }

            // 3. Draw lit dot and glow if the pixel is bright enough/opaque
            if (pa > 0 && brightness > BRIGHTNESS_THRESHOLD) {
                litDotFound = true;
                SDL_Color col = {pr, pg, pb, 255};
                if (pr == pg && pg == pb && brightness > 100) { col = {255, 140, 0, 255}; }

                // Draw Glow
                SDL_SetRenderDrawColor(renderer, col.r/2, col.g/2, col.b/2, glow);
                drawFilledCircle(renderer, cx, cy, radius + 2);

                // Draw Main Dot
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                drawFilledCircle(renderer, cx, cy, radius);
            }
        }
    }

    // --- Step 4: Cleanup ---
    SDL_FreeSurface(tempSurface);

    if (!litDotFound) {
        LOG_WARN("DMD Asset Masking failed to light any dot. Pixels may be too dark or transparent.");
    }
}

// =========================================================================
// 4. Main Render Function and Procedural Helper Functions
// =========================================================================

void DmdSDLRenderer::render(SDL_Renderer* renderer, const std::string& displayText,
                            int width, int height, float time, std::string defaultText)
{
    if (!renderer || width <= 0 || height <= 0) return;

    // First try exact lookup for toppers and other non-manufacturer assets
    SDL_Texture* assetTexture = getAsset(displayText + ".png");
    if (!assetTexture) {
        assetTexture = getAsset(displayText + ".gif");
    }

    if (assetTexture) {
        drawDmdAssetMasked(renderer, assetTexture, width, height, time);
        return;
    }

    // ORIGINAL manufacturer logic (for DMD)
    std::string manufacturer = displayText;
    std::transform(manufacturer.begin(), manufacturer.end(), manufacturer.begin(), ::tolower);

    assetTexture = getAsset(manufacturer + ".png");
    if (!assetTexture) {
        assetTexture = getAsset(manufacturer + ".gif");
    }

    if (assetTexture) {
        drawDmdAssetMasked(renderer, assetTexture, width, height, time);
        return;
    }

    // Fallback to text
    std::string textToDisplay = displayText.empty() ? defaultText : displayText;
    renderProceduralText(renderer, textToDisplay, width, height, time);
}


void DmdSDLRenderer::renderProceduralText(SDL_Renderer* renderer, const std::string& textToDisplay, int width, int height, float time) {

    std::string text = textToDisplay;
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);

    // 2. Full-Screen Dynamic Sizing Calculation
    const int VIRTUAL_DOT_ROWS = 32;
    const int VIRTUAL_DOT_COLS = 128;

    int pixelPerDotX = (width + 1) / VIRTUAL_DOT_COLS;
    int pixelPerDotY = (height + 1) / VIRTUAL_DOT_ROWS;
    const int PIXEL_PER_DOT = std::max(1, std::min(pixelPerDotX, pixelPerDotY));
    const int DOT_RADIUS = std::max(1, (PIXEL_PER_DOT / 2) - 2);

    const int RENDERED_GRID_WIDTH = VIRTUAL_DOT_COLS * PIXEL_PER_DOT;
    const int RENDERED_GRID_HEIGHT = VIRTUAL_DOT_ROWS * PIXEL_PER_DOT;

    const int START_X = width / 2 - RENDERED_GRID_WIDTH / 2;
    const int START_Y = height / 2 - RENDERED_GRID_HEIGHT / 2;

    // --- Text Layout Calculations ---
    const int CHAR_WIDTH_DOTS = 5;
    const int CHAR_SPACING_DOTS = 2;

    const int CHAR_WIDTH_PIXELS = CHAR_WIDTH_DOTS * PIXEL_PER_DOT;
    const int CHAR_SPACING_PIXELS = CHAR_SPACING_DOTS * PIXEL_PER_DOT;

    int textLength = static_cast<int>(text.length());
    int totalTextWidth = textLength * CHAR_WIDTH_PIXELS + (textLength > 0 ? (textLength - 1) : 0) * CHAR_SPACING_PIXELS;
    int textStartX = width / 2 - totalTextWidth / 2;

    int startGridRow = (VIRTUAL_DOT_ROWS / 2) - (CHAR_HEIGHT_DOTS / 2);
    int textStartY = START_Y + startGridRow * PIXEL_PER_DOT;

    // 3. Dynamic Aesthetics (Color and Glow)
    const SDL_Color AMBER_LIT = {255, 150, 0, 255};
    const SDL_Color UNLIT_GRAY = {40, 40, 40, 255};

    float pulse = (std::sin(time * 6.0f) + 1.0f) / 2.0f;
    Uint8 glowAlpha = static_cast<Uint8>(50 + pulse * 100);

    // 4. Background and Unlit Dots
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    for (int r = 0; r < VIRTUAL_DOT_ROWS; ++r) {
        for (int c = 0; c < VIRTUAL_DOT_COLS; ++c) {
            int currentDotCenterX = START_X + c * PIXEL_PER_DOT + PIXEL_PER_DOT / 2;
            int currentDotCenterY = START_Y + r * PIXEL_PER_DOT + PIXEL_PER_DOT / 2;

            SDL_SetRenderDrawColor(renderer, UNLIT_GRAY.r, UNLIT_GRAY.g, UNLIT_GRAY.b, UNLIT_GRAY.a);
            drawFilledCircle(renderer, currentDotCenterX, currentDotCenterY, DOT_RADIUS);
        }
    }

    // 5. Render Text (Lit Dots)
    int currentX = textStartX;
    for (char c : text) {
        drawDmdChar(renderer, c, currentX, textStartY, DOT_RADIUS, PIXEL_PER_DOT, AMBER_LIT, glowAlpha);
        currentX += CHAR_WIDTH_PIXELS + CHAR_SPACING_PIXELS;
    }

    // 6. Render Border
    {
        SDL_Color borderColor = AMBER_LIT;
        const int BORDER_THICKNESS_DOTS = 2;

        for (int t = 0; t < BORDER_THICKNESS_DOTS; t++) {
            for (int c = 0; c < VIRTUAL_DOT_COLS; ++c) {
                int cx = START_X + c * PIXEL_PER_DOT + PIXEL_PER_DOT/2;
                int cy_top = START_Y + t * PIXEL_PER_DOT + PIXEL_PER_DOT/2;
                int cy_bot = START_Y + (VIRTUAL_DOT_ROWS - 1 - t) * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, glowAlpha);
                drawFilledCircle(renderer, cx, cy_top, DOT_RADIUS + 2);
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                drawFilledCircle(renderer, cx, cy_top, DOT_RADIUS);

                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, glowAlpha);
                drawFilledCircle(renderer, cx, cy_bot, DOT_RADIUS + 2);
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                drawFilledCircle(renderer, cx, cy_bot, DOT_RADIUS);
            }
        }

        for (int t = 0; t < BORDER_THICKNESS_DOTS; t++) {
            for (int r = BORDER_THICKNESS_DOTS; r < VIRTUAL_DOT_ROWS - BORDER_THICKNESS_DOTS; ++r) {
                int cy = START_Y + r * PIXEL_PER_DOT + PIXEL_PER_DOT/2;
                int cx_left = START_X + t * PIXEL_PER_DOT + PIXEL_PER_DOT/2;
                int cx_right = START_X + (VIRTUAL_DOT_COLS - 1 - t) * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, glowAlpha);
                drawFilledCircle(renderer, cx_left, cy, DOT_RADIUS + 2);
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                drawFilledCircle(renderer, cx_left, cy, DOT_RADIUS);

                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, glowAlpha);
                drawFilledCircle(renderer, cx_right, cy, DOT_RADIUS + 2);
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                drawFilledCircle(renderer, cx_right, cy, DOT_RADIUS);
            }
        }
    }
}

// Midpoint circle algorithm
void DmdSDLRenderer::drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w <= radius * 2; w++) {
        for (int h = 0; h <= radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

// Draws a single character using the DMD_FONT bitmap data
void DmdSDLRenderer::drawDmdChar(SDL_Renderer* renderer, char c, int startX, int startY, int dotRadius, int pixelPerDot, SDL_Color color, Uint8 glowAlpha) {
    char upperC = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    auto it = DMD_FONT.find(upperC);

    if (it == DMD_FONT.end() || dotRadius <= 0) return;

    const std::array<uint16_t, 5>& data = it->second;

    for (int col = 0; col < 5; ++col) {
        uint16_t colData = data[col];
        for (int row = 0; row < CHAR_HEIGHT_DOTS; ++row) {
            if (colData & (1 << row)) {
                int dotCenterX = startX + col * pixelPerDot + pixelPerDot / 2;
                int dotCenterY = startY + row * pixelPerDot + pixelPerDot / 2;

                // 1. Draw Subtle Glow
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, glowAlpha);
                drawFilledCircle(renderer, dotCenterX, dotCenterY, dotRadius + 2);

                // 2. Draw Main Dot
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                drawFilledCircle(renderer, dotCenterX, dotCenterY, dotRadius);
            }
        }
    }
}
