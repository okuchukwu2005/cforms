/**
 * @file graphics.h
 * @brief Contains all rendering logic for SDL2-based drawing operations
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#include "color.h"   // Access Color struct

/**
 * @brief Clears the screen to the specified color
 * @param base Pointer to the Base struct containing the renderer
 * @param color The background color to set
 */
static inline void clear_screen_(Base* base, Color color) {
    SDL_SetRenderDrawColor(base->sdl_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(base->sdl_renderer);
}

/**
 * @brief Presents the rendered content to the screen
 * @param base Pointer to the Base struct containing the renderer
 */
static inline void present_(Base* base) {
    SDL_RenderPresent(base->sdl_renderer);
}

// ______________DRAW FUNCTIONS_____________

/**
 * @brief Draws a filled rectangle at the specified position and size
 * @param base Pointer to the Base struct containing the renderer
 * @param x X-coordinate of the top-left corner
 * @param y Y-coordinate of the top-left corner
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param color The fill color of the rectangle
 */
static inline void draw_rect_(Base* base, int x, int y, int w, int h, Color color) {
    SDL_SetRenderDrawColor(base->sdl_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(base->sdl_renderer, &rect);
}

/**
 * @brief Draws a filled circle at the specified center with the given radius
 * @param base Pointer to the Base struct containing the renderer
 * @param x X-coordinate of the circle's center
 * @param y Y-coordinate of the circle's center
 * @param radius Radius of the circle
 * @param color The fill color of the circle
 */
static inline void draw_circle_(Base* base, int x, int y, int radius, Color color) {
    SDL_SetRenderDrawColor(base->sdl_renderer, color.r, color.g, color.b, color.a);
    // Midpoint circle algorithm (approximation for filled circle)
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w * w + h * h <= radius * radius) {
                SDL_RenderDrawPoint(base->sdl_renderer, x + w, y + h);
            }
        }
    }
}

/**
 * @brief Draws a filled triangle with the specified vertices
 * @param base Pointer to the Base struct containing the renderer
 * @param x1 X-coordinate of the first vertex
 * @param y1 Y-coordinate of the first vertex
 * @param x2 X-coordinate of the second vertex
 * @param y2 Y-coordinate of the second vertex
 * @param x3 X-coordinate of the third vertex
 * @param y3 Y-coordinate of the third vertex
 * @param color The fill color of the triangle
 */
static inline void draw_triangle_(Base* base, int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    SDL_SetRenderDrawColor(base->sdl_renderer, color.r, color.g, color.b, color.a);

    // Sort vertices by y-coordinate (v1 at top, v3 at bottom)
    int temp_x, temp_y;
    if (y1 > y2) {
        temp_x = x1; temp_y = y1;
        x1 = x2; y1 = y2;
        x2 = temp_x; y2 = temp_y;
    }
    if (y2 > y3) {
        temp_x = x2; temp_y = y2;
        x2 = x3; y2 = y3;
        x3 = temp_x; y3 = temp_y;
    }
    if (y1 > y2) {
        temp_x = x1; temp_y = y1;
        x1 = x2; y1 = y2;
        x2 = temp_x; y2 = temp_y;
    }

    // Handle degenerate cases (same y-coordinates)
    if (y1 == y3) {
        return;
    }

    // Linear interpolation for edges
    float dx12 = (y2 != y1) ? (float)(x2 - x1) / (y2 - y1) : 0;
    float dx13 = (y3 != y1) ? (float)(x3 - x1) / (y3 - y1) : 0;
    float dx23 = (y3 != y2) ? (float)(x3 - x2) / (y3 - y2) : 0;

    // Top to middle (y1 to y2)
    for (int y = y1; y <= y2 && y <= y3; y++) {
        int x_start = x1 + (int)((y - y1) * dx13);
        int x_end = (y < y2) ? x1 + (int)((y - y1) * dx12) : x2 + (int)((y - y2) * dx23);
        if (x_start > x_end) {
            int temp = x_start;
            x_start = x_end;
            x_end = temp;
        }
        SDL_RenderDrawLine(base->sdl_renderer, x_start, y, x_end, y);
    }

    // Middle to bottom (y2 to y3)
    for (int y = y2 + 1; y <= y3; y++) {
        int x_start = x1 + (int)((y - y1) * dx13);
        int x_end = x2 + (int)((y - y2) * dx23);
        if (x_start > x_end) {
            int temp = x_start;
            x_start = x_end;
            x_end = temp;
        }
        SDL_RenderDrawLine(base->sdl_renderer, x_start, y, x_end, y);
    }
}

/**
 * @brief Draws a filled rounded rectangle (similar to raylib's DrawRectangleRounded)
 * @param base Pointer to the Base struct containing the renderer
 * @param x X-coordinate of the top-left corner
 * @param y Y-coordinate of the top-left corner
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param roundness Rounding factor (0.0f = rectangle, 1.0f = full rounded)
 * @param color The fill color of the rounded rectangle
 */
static inline void draw_rounded_rect_(Base* base, int x, int y, int w, int h, float roundness, Color color) {
    SDL_SetRenderDrawColor(base->sdl_renderer, color.r, color.g, color.b, color.a);

    if (w <= 0 || h <= 0) {
        return;
    }

    if (roundness <= 0.0f) {
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(base->sdl_renderer, &rect);
        return;
    }

    float min_dim = (w < h) ? w : h;
    int radius = (int)(roundness * min_dim / 2.0f);
    if (radius <= 0) {
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(base->sdl_renderer, &rect);
        return;
    }

    // Clamp radius to avoid over-rounding
    if (2 * radius > w) radius = w / 2;
    if (2 * radius > h) radius = h / 2;

    int radius_sq = radius * radius;

    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            bool inside = false;

            // Inner rectangle
            if (dx >= radius && dy >= radius && dx < w - radius && dy < h - radius) {
                inside = true;
            }
            // Left strip
            else if (dx < radius && dy >= radius && dy < h - radius) {
                inside = true;
            }
            // Right strip
            else if (dx >= w - radius && dy >= radius && dy < h - radius) {
                inside = true;
            }
            // Top strip
            else if (dy < radius && dx >= radius && dx < w - radius) {
                inside = true;
            }
            // Bottom strip
            else if (dy >= h - radius && dx >= radius && dx < w - radius) {
                inside = true;
            }
            // Top-left corner
            else if (dx < radius && dy < radius) {
                int dx_corner = dx - radius;
                int dy_corner = dy - radius;
                if (dx_corner * dx_corner + dy_corner * dy_corner <= radius_sq) {
                    inside = true;
                }
            }
            // Top-right corner
            else if (dx >= w - radius && dy < radius) {
                int dx_corner = dx - (w - radius);
                int dy_corner = dy - radius;
                if (dx_corner * dx_corner + dy_corner * dy_corner <= radius_sq) {
                    inside = true;
                }
            }
            // Bottom-left corner
            else if (dx < radius && dy >= h - radius) {
                int dx_corner = dx - radius;
                int dy_corner = dy - (h - radius);
                if (dx_corner * dx_corner + dy_corner * dy_corner <= radius_sq) {
                    inside = true;
                }
            }
            // Bottom-right corner
            else if (dx >= w - radius && dy >= h - radius) {
                int dx_corner = dx - (w - radius);
                int dy_corner = dy - (h - radius);
                if (dx_corner * dx_corner + dy_corner * dy_corner <= radius_sq) {
                    inside = true;
                }
            }

            if (inside) {
                SDL_RenderDrawPoint(base->sdl_renderer, x + dx, y + dy);
            }
        }
    }
}

/**
 * @brief Text alignment options for draw_text_from_font_
 */
typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} TextAlign;

/**
 * @brief Draws text at the specified position using a provided TTF font
 * @param base Pointer to the Base struct containing the renderer
 * @param font Pointer to the loaded TTF_Font
 * @param text The text string to render
 * @param x X-coordinate for the text (depends on align)
 * @param y Y-coordinate for the top of the text
 * @param color The color of the text
 * @param align Text alignment (ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT)
 */
static inline void draw_text_from_font_(Base* base, TTF_Font* font, const char* text, int x, int y, Color color, TextAlign align) {
    if (!font) {
        printf("No font provided for text rendering\n");
        return;
    }

    SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, sdl_color);
    if (!surface) {
        printf("Failed to render text: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(base->sdl_renderer, surface);
    if (!texture) {
        printf("Failed to create texture from text surface: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    int text_width = surface->w;
    int text_height = surface->h;
    int adjusted_x = x;

    // Adjust x-coordinate based on alignment
    switch (align) {
        case ALIGN_CENTER:
            adjusted_x = x - text_width / 2;
            break;
        case ALIGN_RIGHT:
            adjusted_x = x - text_width;
            break;
        case ALIGN_LEFT:
        default:
            // No adjustment for left alignment
            break;
    }

    SDL_Rect dst_rect = {adjusted_x, y, text_width, text_height};
    SDL_RenderCopy(base->sdl_renderer, texture, NULL, &dst_rect);

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

/**
 * @brief Draws text at the specified position with the given font size and color
 * @param base Pointer to the Base struct containing the renderer
 * @param text The text string to render
 * @param font_size The size of the font in points
 * @param x X-coordinate for the top-left corner of the text
 * @param y Y-coordinate for the top-left corner of the text
 * @param color The color of the text
 */
static char *FONT_FILE = "FreeMono.ttf";

static inline void draw_text_(Base* base, const char* text, int font_size, int x, int y, Color color) {
    TTF_Font* font = TTF_OpenFont(FONT_FILE, font_size);
    if (!font) {
        printf("Failed to load font '%s': %s\n", FONT_FILE, TTF_GetError());
        return;
    }

    draw_text_from_font_(base, font, text, x, y, color, ALIGN_LEFT);
    TTF_CloseFont(font);
}

#endif // GRAPHICS_H
