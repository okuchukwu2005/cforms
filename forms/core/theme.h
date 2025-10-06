/**
 * @file theme.h
 * @brief Defines themes for the GUI library, including color palettes and styles
 */

#ifndef THEME_H
#define THEME_H

#include "color.h"  // For Color struct

/**
 * @brief Theme structure holding visual properties for the GUI
 */
typedef struct {
    // Background colors
    Color bg_primary;       // Main background (e.g., window bg)
    Color bg_secondary;     // Secondary backgrounds (e.g., panels, containers)

    // Text colors
    Color text_primary;     // Default text color
    Color text_secondary;   // Muted or secondary text (e.g., hints)

    // Accent and interactive colors
    Color accent;           // Highlights, borders, or active elements
    Color accent_hovered;   // Hover state for accents
    Color accent_pressed;   // Pressed state for accents

    // Button-specific colors
    Color button_normal;    // Default button background
    Color button_hovered;   // Button hover background
    Color button_pressed;   // Button pressed background
    Color button_text;      // Button text color

    // Container-specific colors
    Color container_bg;     // Background for containers
    Color container_title_bg; // Background for container title bars

    // Metrics and styles
    int default_font_size;  // Default font size in points
    char* font_file;        // Path to the default font file
    int padding;            // Default padding for widgets
    float roundness;        // Rounding factor for rounded rectangles (0.0 to 1.0)
} Theme;

const Theme* current_theme = NULL;  // Global definition (initialized to NULL; set in init)
// Function to set the current theme
void set_theme(const Theme* theme);

// Predefined themes

/**
 * @brief Light theme preset
 */
static const Theme THEME_LIGHT = {
    .bg_primary = {255, 255, 255, 255},      // White
    .bg_secondary = {240, 240, 240, 255},    // Light gray
    .text_primary = {0, 0, 0, 255},          // Black
    .text_secondary = {100, 100, 100, 255},  // Dark gray
    .accent = {0, 122, 255, 255},            // Blue
    .accent_hovered = {0, 142, 255, 255},    // Lighter blue
    .accent_pressed = {0, 102, 255, 255},    // Darker blue
    .button_normal = {200, 200, 200, 255},   // Gray
    .button_hovered = {220, 220, 220, 255},  // Lighter gray
    .button_pressed = {180, 180, 180, 255},  // Darker gray
    .button_text = {0, 0, 0, 255},           // Black
    .container_bg = {250, 250, 250, 255},    // Slightly whiter for contrast
    .container_title_bg = {220, 220, 220, 255}, // Lighter gray for title bar
    .default_font_size = 16,
    .font_file = "FreeMono.ttf",
    .padding = 10,
    .roundness = 0.2f
};

/**
 * @brief Dark theme preset
 */
static const Theme THEME_DARK = {
    .bg_primary = {30, 30, 30, 255},         // Dark gray
    .bg_secondary = {45, 45, 45, 255},       // Slightly lighter dark gray
    .text_primary = {255, 255, 255, 255},    // White
    .text_secondary = {170, 170, 170, 255},  // Light gray
    .accent = {0, 122, 255, 255},            // Blue (same as light for consistency)
    .accent_hovered = {0, 142, 255, 255},    // Lighter blue
    .accent_pressed = {0, 102, 255, 255},    // Darker blue
    .button_normal = {60, 60, 60, 255},      // Dark gray
    .button_hovered = {80, 80, 80, 255},     // Lighter dark gray
    .button_pressed = {40, 40, 40, 255},     // Darker dark gray
    .button_text = {255, 255, 255, 255},     // White
    .container_bg = {35, 35, 35, 255},       // Slightly darker for contrast
    .container_title_bg = {50, 50, 50, 255}, // Lighter dark gray for title bar
    .default_font_size = 16,
    .font_file = "FreeMono.ttf",
    .padding = 10,
    .roundness = 0.2f
};

/**
 * @brief Hacker theme preset (green on black, for fun)
 */
static const Theme THEME_HACKER = {
    .bg_primary = {0, 0, 0, 255},            // Black
    .bg_secondary = {20, 20, 20, 255},       // Very dark gray
    .text_primary = {0, 255, 0, 255},        // Green
    .text_secondary = {0, 200, 0, 255},      // Muted green
    .accent = {0, 255, 0, 255},              // Green
    .accent_hovered = {100, 255, 100, 255},  // Lighter green
    .accent_pressed = {0, 200, 0, 255},      // Darker green
    .button_normal = {0, 50, 0, 255},        // Dark green
    .button_hovered = {0, 100, 0, 255},      // Medium green
    .button_pressed = {0, 30, 0, 255},       // Darker green
    .button_text = {0, 255, 0, 255},         // Green
    .container_bg = {10, 10, 10, 255},       // Even darker gray for contrast
    .container_title_bg = {30, 30, 30, 255}, // Lighter dark for title bar
    .default_font_size = 16,
    .font_file = "FreeMono.ttf",
    .padding = 10,
    .roundness = 0.1f                        // Less rounded for a "techy" feel
};

// Implementation of set_theme (can be in a .c file or inline here)
void set_theme(const Theme* theme) {
    current_theme = theme;
    // Optional: Trigger a redraw if needed (e.g., set a flag for your main loop)
}

#endif // THEME_H
