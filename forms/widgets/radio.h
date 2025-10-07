/**
 * @file radio.h
 * @brief Contains logic for radio button widgets using SDL2
 */

#ifndef RADIO_H
#define RADIO_H

#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <SDL2/SDL.h> // for SDL_Event, etc.
#include <math.h>   // For roundf in scaling

typedef struct {
    Parent* parent;      // Parent container or window
    int x, y;            // Position (relative to parent) (logical)
    int w, h;            // Size (logical)
    char* label;         // Label text
    bool selected;       // Is selected?
    int group_id;        // Group ID (1 group → only 1 selected)
    bool is_hovered;     // For potential hover effects
    // Theme overrides (NULL = use theme)
    Color* custom_outer_color;   // Outer circle color
    Color* custom_inner_color;   // Inner circle color when selected
    Color* custom_label_color;   // Label text color
} Radio;

#define MAX_RADIOS 100
static Radio* radio_widgets[MAX_RADIOS];
static int radios_count = 0;

// -------- Register --------
static inline void register_widget_radio(Radio* radio) {
    if (radios_count < MAX_RADIOS) {
        radio_widgets[radios_count++] = radio;
    }
}

// -------- Create --------
static inline Radio* new_radio_button_(Parent* parent, int x, int y, int w, int h,
                                       const char* label, int group_id) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    Radio* radio = (Radio*)malloc(sizeof(Radio));
    if (!radio) {
        printf("Failed to allocate Radio\n");
        return NULL;
    }

    radio->parent = parent;
    radio->x = x;
    radio->y = y;
    radio->w = w;
    radio->h = h;
    radio->label = strdup(label);
    if (!radio->label) {
        free(radio);
        printf("Failed to allocate memory for label\n");
        return NULL;
    }
    radio->selected = false;
    radio->group_id = group_id;
    radio->is_hovered = false;
    // Init overrides to NULL (use theme)
    radio->custom_outer_color = NULL;
    radio->custom_inner_color = NULL;
    radio->custom_label_color = NULL;

    register_widget_radio(radio);
    return radio;
}

// Setters for overrides
static inline void set_radio_outer_color(Radio* radio, Color color) {
    if (radio) {
        if (!radio->custom_outer_color) {
            radio->custom_outer_color = (Color*)malloc(sizeof(Color));
        }
        *radio->custom_outer_color = color;
    }
}

static inline void set_radio_inner_color(Radio* radio, Color color) {
    if (radio) {
        if (!radio->custom_inner_color) {
            radio->custom_inner_color = (Color*)malloc(sizeof(Color));
        }
        *radio->custom_inner_color = color;
    }
}

static inline void set_radio_label_color(Radio* radio, Color color) {
    if (radio) {
        if (!radio->custom_label_color) {
            radio->custom_label_color = (Color*)malloc(sizeof(Color));
        }
        *radio->custom_label_color = color;
    }
}

// -------- Render --------
static inline void render_radio_(Radio* radio) {
    if (!radio || !radio->parent || !radio->parent->base.sdl_renderer || !radio->parent->is_open) {
        printf("Invalid radio, renderer, or parent is not open\n");
        return;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }

    float dpi = radio->parent->base.dpi_scale;
    // Calculate absolute position relative to parent (logical), then scale
    int abs_x = radio->x + radio->parent->x;
    int abs_y = radio->y + radio->parent->y + radio->parent->title_height;
    int sx = (int)roundf(abs_x * dpi);
    int sy = (int)roundf(abs_y * dpi);
    int sh = (int)roundf(radio->h * dpi);  // Assuming square, w ignored or = h
    int radius = (int)roundf((radio->h / 2) * dpi);
    int inner_margin = (int)roundf(4 * dpi);  // Scale the inner margin
    int inner_radius = radius - inner_margin;
    int font_size = (int)roundf(current_theme->default_font_size * dpi);  // Scaled from theme
    int pad = (int)roundf(current_theme->padding * dpi);  // Scaled padding for label

    Base* base = &radio->parent->base;

    // Get colors from theme/overrides
    Color outer_color = radio->custom_outer_color ? *radio->custom_outer_color : current_theme->bg_secondary;
    if (radio->is_hovered) {
        outer_color = radio->custom_outer_color ? lighten_color(*radio->custom_outer_color, 0.1f) : current_theme->button_hovered;
    }
    Color inner_color = radio->custom_inner_color ? *radio->custom_inner_color : current_theme->accent;
    Color label_color = radio->custom_label_color ? *radio->custom_label_color : current_theme->text_primary;

    // Outer circle (centered at sx, sy)
    draw_circle_(base, sx, sy, radius, outer_color);

    // Inner circle if selected
    if (radio->selected) {
        draw_circle_(base, sx, sy, inner_radius, inner_color);
    }

    // Label text (to the right, vertically centered)
    int label_y = sy - (int)roundf((radio->h / 6) * dpi);  // Adjusted for better centering (was /3)
    draw_text_(base, radio->label, font_size,
               sx + sh + pad / 2, label_y,
               label_color);
}

// -------- Update --------
static inline void update_radio_(Radio* radio, SDL_Event event) {
    if (!radio || !radio->parent || !radio->parent->is_open) {
        printf("Invalid radio, parent, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent (logical)
    int abs_x = radio->x + radio->parent->x;
    int abs_y = radio->y + radio->parent->y + radio->parent->title_height;

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Check hover (bounding box around circle)
    bool over = (mouse_x >= abs_x - radio->h/2 && mouse_x <= abs_x + radio->h/2 &&
                 mouse_y >= abs_y - radio->h/2 && mouse_y <= abs_y + radio->h/2);
    radio->is_hovered = over;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;

        // Hitbox: circle bounding box
        if (mx >= abs_x - radio->h/2 && mx <= abs_x + radio->h/2 &&
            my >= abs_y - radio->h/2 && my <= abs_y + radio->h/2) {

            // Deselect others in same group
            for (int i = 0; i < radios_count; i++) {
                if (radio_widgets[i] && radio_widgets[i]->group_id == radio->group_id) {
                    radio_widgets[i]->selected = false;
                }
            }
            radio->selected = true;
        }
    }
}

// -------- Free --------
static inline void free_radio_(Radio* radio) {
    if (radio) {
        free(radio->label);
        if (radio->custom_outer_color) free(radio->custom_outer_color);
        if (radio->custom_inner_color) free(radio->custom_inner_color);
        if (radio->custom_label_color) free(radio->custom_label_color);
        free(radio);
    }
}

// -------- Helpers for all radios --------
static inline void render_all_registered_radios(void) {
    for (int i = 0; i < radios_count; i++) {
        if (radio_widgets[i]) {
            render_radio_(radio_widgets[i]);
        }
    }
}

static inline void update_all_registered_radios(SDL_Event event) {
    for (int i = 0; i < radios_count; i++) {
        if (radio_widgets[i]) {
            update_radio_(radio_widgets[i], event);
        }
    }
}

#endif // RADIO_H
