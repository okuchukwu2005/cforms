/**
 * @file color.h
 * @brief Contains color constants and structure for RGBA colors
 */
#ifndef COLOR_H
#define COLOR_H

#include <SDL2/SDL.h>

/**
 * @struct Color
 * @brief Represents an RGBA color with 8-bit components
 */
typedef struct {
    Uint8 r;  // Red component (0-255)
    Uint8 g;  // Green component (0-255)
    Uint8 b;  // Blue component (0-255)
    Uint8 a;  // Alpha component (0-255)
} Color;

// Basic colors
#define COLOR_BLACK       (Color){0, 0, 0, 255}
#define COLOR_WHITE       (Color){255, 255, 255, 255}
#define COLOR_RED         (Color){255, 0, 0, 255}
#define COLOR_GREEN       (Color){0, 255, 0, 255}
#define COLOR_BLUE        (Color){0, 0, 255, 255}

// Extended colors
#define COLOR_YELLOW      (Color){255, 255, 0, 255}
#define COLOR_CYAN        (Color){0, 255, 255, 255}
#define COLOR_MAGENTA     (Color){255, 0, 255, 255}
#define COLOR_ORANGE      (Color){255, 165, 0, 255}
#define COLOR_PURPLE      (Color){128, 0, 128, 255}
#define COLOR_PINK        (Color){255, 192, 203, 255}
#define COLOR_BROWN       (Color){139, 69, 19, 255}
#define COLOR_GRAY        (Color){128, 128, 128, 255}
#define COLOR_LIGHT_GRAY  (Color){211, 211, 211, 255}
#define COLOR_DARK_GRAY   (Color){64, 64, 64, 255}
#define COLOR_GOLD        (Color){255, 215, 0, 255}
#define COLOR_SILVER      (Color){192, 192, 192, 255}
#define COLOR_MAROON      (Color){128, 0, 0, 255}
#define COLOR_OLIVE       (Color){128, 128, 0, 255}
#define COLOR_TEAL        (Color){0, 128, 128, 255}
#define COLOR_NAVY        (Color){0, 0, 128, 255}

#endif // COLOR_H