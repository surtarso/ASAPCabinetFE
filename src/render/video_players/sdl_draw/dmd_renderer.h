#ifndef DMD_RENDERER_H
#define DMD_RENDERER_H

#include <SDL.h>
#include <string>
#include <map>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

/**
 * @brief Handles all procedural rendering and animation for the Dot Matrix Display (DMD) screen.
 */
class DmdSDLRenderer {
public:
    DmdSDLRenderer() = default;

    /**
     * @brief Renders the given text as a dot matrix display, dynamically sizing the dots
     * to fit the screen dimensions. The background grid now fills the entire screen.
     * @param renderer The SDL renderer.
     * @param displayText The string to display.
     * @param width The screen width.
     * @param height The screen height.
     * @param time Accumulated time for simple animations (like glow/flicker).
     */
    void render(SDL_Renderer* renderer, const std::string& displayText, int width, int height, float time, std::string defaultText) {
        if (!renderer || width <= 0 || height <= 0) return;

        // 1. Text Selection and Pre-processing
        std::string textToDisplay = displayText;
        if (textToDisplay.empty()) {
            textToDisplay = defaultText; // Fallback text
        }
        std::transform(textToDisplay.begin(), textToDisplay.end(), textToDisplay.begin(), ::toupper);

        // 2. Full-Screen Dynamic Sizing Calculation
        const int VIRTUAL_DOT_ROWS = 32;
        const int VIRTUAL_DOT_COLS = 128;

        // Calculate maximum PIXEL_PER_DOT (this is the grid cell size: Dot Size + Spacing)
        int pixelPerDotX = (width + 1) / VIRTUAL_DOT_COLS;
        int pixelPerDotY = (height + 1) / VIRTUAL_DOT_ROWS;

        const int PIXEL_PER_DOT = std::max(1, std::min(pixelPerDotX, pixelPerDotY));

        // Dot radius is set to create a noticeable gap (1 pixel less than half the cell size)
        const int DOT_RADIUS = std::max(1, (PIXEL_PER_DOT / 2) - 2);

        // Calculate the actual total pixel size of the rendered grid
        const int RENDERED_GRID_WIDTH = VIRTUAL_DOT_COLS * PIXEL_PER_DOT;// - 1;
        const int RENDERED_GRID_HEIGHT = VIRTUAL_DOT_ROWS * PIXEL_PER_DOT;// - 1;

        // Calculate the starting position (top-left) to center the entire grid
        const int START_X = width / 2 - RENDERED_GRID_WIDTH / 2;
        const int START_Y = height / 2 - RENDERED_GRID_HEIGHT / 2;


        // --- Text Layout Calculations ---

        const int CHAR_WIDTH_DOTS = 5;
        const int CHAR_SPACING_DOTS = 2;

        // Text width in pixels
        const int CHAR_WIDTH_PIXELS = CHAR_WIDTH_DOTS * PIXEL_PER_DOT;
        const int CHAR_SPACING_PIXELS = CHAR_SPACING_DOTS * PIXEL_PER_DOT;

        int textLength = static_cast<int>(textToDisplay.length());

        int totalTextWidth = textLength * CHAR_WIDTH_PIXELS + (textLength > 0 ? (textLength - 1) : 0) * CHAR_SPACING_PIXELS;

        // Text start position (centered on screen)
        int textStartX = width / 2 - totalTextWidth / 2;

        // Calculate the starting ROW index in the virtual grid for vertical centering
        // Use class constant CHAR_HEIGHT_DOTS here
        int startGridRow = (VIRTUAL_DOT_ROWS / 2) - (this->CHAR_HEIGHT_DOTS / 2);

        // Convert that row index into a pixel position (relative to screen top)
        int textStartY = START_Y + startGridRow * PIXEL_PER_DOT;


        // 3. Dynamic Aesthetics (Color and Glow)
        const SDL_Color AMBER_LIT = {255, 150, 0, 255}; // Lit color
        const SDL_Color UNLIT_GRAY = {40, 40, 40, 255}; // Dark gray unlit color

        // Animation for subtle flicker/glow
        float pulse = (std::sin(time * 7.0f) + 1.0f) / 2.0f; // 0.0 to 1.0 (time * higher/faster, lower/slower)
        Uint8 glowAlpha = static_cast<Uint8>(60 + pulse * 100); // base * pulse * range glow intensity

        // 4. Background and Unlit Dots (The screen itself)
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        // Draw the UNLIT dot grid across the entire calculated display area
        for (int r = 0; r < VIRTUAL_DOT_ROWS; ++r) {
            for (int c = 0; c < VIRTUAL_DOT_COLS; ++c) {
                 // Calculate center position for the unlit dot
                 int currentDotCenterX = START_X + c * PIXEL_PER_DOT + PIXEL_PER_DOT / 2;
                 int currentDotCenterY = START_Y + r * PIXEL_PER_DOT + PIXEL_PER_DOT / 2;

                 SDL_SetRenderDrawColor(renderer, UNLIT_GRAY.r, UNLIT_GRAY.g, UNLIT_GRAY.b, UNLIT_GRAY.a);
                 drawFilledCircle(renderer, currentDotCenterX, currentDotCenterY, DOT_RADIUS);
            }
        }

        // 5. Render Text (Lit Dots)
        int currentX = textStartX;
        for (char c : textToDisplay) {
            drawDmdChar(renderer, c, currentX, textStartY, DOT_RADIUS, PIXEL_PER_DOT, AMBER_LIT, glowAlpha);
            currentX += CHAR_WIDTH_PIXELS + CHAR_SPACING_PIXELS;
        }

        // 6. Render a border aroud the window
        {
            SDL_Color borderColor = AMBER_LIT;

            // thickness in DOTS (not pixels)
            const int BORDER_THICKNESS_DOTS = 2;

            // --- TOP + BOTTOM border, multiple rows ---
            for (int t = 0; t < BORDER_THICKNESS_DOTS; t++) {
                for (int c = 0; c < VIRTUAL_DOT_COLS; ++c) {

                    int cx = START_X + c * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                    // top row shifted downward by t rows
                    int cy_top = START_Y + t * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                    // bottom row shifted upward by t rows
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

            // --- LEFT + RIGHT border, multiple columns ---
            for (int t = 0; t < BORDER_THICKNESS_DOTS; t++) {
                for (int r = BORDER_THICKNESS_DOTS; r < VIRTUAL_DOT_ROWS - BORDER_THICKNESS_DOTS; ++r) {

                    int cy = START_Y + r * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                    // left column shifted right by t columns
                    int cx_left = START_X + t * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                    // right column shifted left by t columns
                    int cx_right = START_X + (VIRTUAL_DOT_COLS - 1 - t) * PIXEL_PER_DOT + PIXEL_PER_DOT/2;

                    // Glow + main dot left
                    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, glowAlpha);
                    drawFilledCircle(renderer, cx_left, cy, DOT_RADIUS + 2);
                    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                    drawFilledCircle(renderer, cx_left, cy, DOT_RADIUS);

                    // Glow + main dot right
                    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, glowAlpha);
                    drawFilledCircle(renderer, cx_right, cy, DOT_RADIUS + 2);
                    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                    drawFilledCircle(renderer, cx_right, cy, DOT_RADIUS);
                }
            }
        }
    }

private:
    // Function to draw a filled circle
    void drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
        // Simple circle drawing using Bresenham's circle algorithm for efficiency in SDL
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
                }
            }
        }
    }

    // 5x9 font subset (bits 0-8 represent the 9 rows vertically)
    // The bit array is now of size 5 (columns) with 16-bit integers (uint16_t) to accommodate 9 rows.
    static const std::map<char, std::array<uint16_t, 5>> DMD_FONT;
    const int CHAR_HEIGHT_DOTS = 9; // Define once as a class member

    /**
     * @brief Draws a single character using the DMD_FONT bitmap data with glow/shadow.
     * @param pixelPerDot The pixel size of a single grid cell (dot + spacing).
     */
    void drawDmdChar(SDL_Renderer* renderer, char c, int startX, int startY, int dotRadius, int pixelPerDot, SDL_Color color, Uint8 glowAlpha) {
        char upperC = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        auto it = DMD_FONT.find(upperC);

        if (it == DMD_FONT.end() || dotRadius <= 0) return;

        const std::array<uint16_t, 5>& data = it->second;

        for (int col = 0; col < 5; ++col) {
            uint16_t colData = data[col];
            for (int row = 0; row < CHAR_HEIGHT_DOTS; ++row) { // Use class constant CHAR_HEIGHT_DOTS
                // Check if the bit for this row is set (Lit Dot)
                if (colData & (1 << row)) {
                    // Calculate center of the dot within its grid cell
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
};

// --- Static DMD Font Definition ---
// 9-high (bits 0-8) by 5-wide character bitmap data.
// Standard, clean, single-dot stroke 5x9 font.
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


#endif // DMD_RENDERER_H
