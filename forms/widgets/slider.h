#ifndef SLIDER_H
#define SLIDER_H

#include <SDL2/SDL.h>
#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <math.h>   // For roundf in scaling

typedef struct {
    Parent* parent;      // Parent container or window
    int x, y;            // Position (relative to parent) (logical)
    int w, h;            // Width and height (assume horizontal slider, w > h) (logical)
    int min, max;        // Range of values
    int value;           // Current value
    char* label;         // Optional label text
    bool dragging;       // Flag to track if thumb is being dragged
    bool is_hovered;     // Flag for hover state (for color variants)
    Color* custom_track_color;   // Optional override for track color (NULL = use theme)
    Color* custom_thumb_color;   // Optional override for thumb color (NULL = use theme)
    Color* custom_label_color;   // Optional override for label color (NULL = use theme)
} Slider;


// -------- Create --------
static inline Slider new_slider(Parent* parent, int x, int y, int w, int h, int min, int max, int start_value, const char* label) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
    }

    Slider slider;
    
    slider.parent = parent;
    slider.x = x;
    slider.y = y;
    slider.w = w;
    slider.h = h;
    slider.min = min;
    slider.max = max;
    slider.value = start_value;
    slider.label = label ? strdup(label) : NULL;  // Copy label if provided
    slider.dragging = false;
    slider.is_hovered = false;
    slider.custom_track_color = NULL;
    slider.custom_thumb_color = NULL;
    slider.custom_label_color = NULL;

    return slider;
}

// Setters for overrides
static inline void set_slider_track_color(Slider* slider, Color color) {
    if (slider) {
        if (!slider->custom_track_color) {
            slider->custom_track_color = (Color*)malloc(sizeof(Color));
        }
        *slider->custom_track_color = color;
    }
}

static inline void set_slider_thumb_color(Slider* slider, Color color) {
    if (slider) {
        if (!slider->custom_thumb_color) {
            slider->custom_thumb_color = (Color*)malloc(sizeof(Color));
        }
        *slider->custom_thumb_color = color;
    }
}

static inline void set_slider_label_color(Slider* slider, Color color) {
    if (slider) {
        if (!slider->custom_label_color) {
            slider->custom_label_color = (Color*)malloc(sizeof(Color));
        }
        *slider->custom_label_color = color;
    }
}

// -------- Render --------
static inline void render_slider(Slider* slider) {
    if (!slider || !slider->parent || !slider->parent->base.sdl_renderer || !slider->parent->is_open) {
        printf("Invalid slider, renderer, or parent is not open\n");
        return;
    }
	//set container clipping
	if(slider->parent->is_window == false){
	SDL_Rect parent_bounds = get_parent_rect(slider->parent);
	SDL_RenderSetClipRect(slider->parent->base.sdl_renderer, &parent_bounds);
	}

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }

    float dpi = slider->parent->base.dpi_scale;
    // Calculate absolute position relative to parent (logical), then scale
    int abs_x = slider->x + slider->parent->x;
    int abs_y = slider->y + slider->parent->y + slider->parent->title_height;
    int sx = (int)roundf(abs_x * dpi);
    int sy = (int)roundf(abs_y * dpi);
    int sw = (int)roundf(slider->w * dpi);
    int sh = (int)roundf(slider->h * dpi);
    int track_height = (int)roundf(4 * dpi);  // Scaled thin track
    int thumb_width = (int)roundf(10 * dpi);  // Scaled fixed width for rectangular thumb
    int label_pad = (int)roundf(10 * dpi);    // Scaled padding for label
    int label_v_offset = (int)roundf(8 * dpi); // Scaled vertical offset for label
    int font_size = (int)roundf(current_theme->default_font_size * dpi);  // Scaled from theme

    Base* base = &slider->parent->base;

    // Draw track (horizontal bar)
    Color track_color = slider->custom_track_color ? *slider->custom_track_color : current_theme->bg_secondary;
    draw_rect_(base, sx, sy + (sh / 2) - (track_height / 2), sw, track_height, track_color);

    // Calculate thumb position
    float range = slider->max - slider->min;
    float pos_ratio = (slider->value - slider->min) / range;
    int thumb_x_logical = abs_x + (int)(pos_ratio * slider->w);
    int sthumb_x = (int)roundf(thumb_x_logical * dpi);

    // Determine thumb color: custom > theme accent with state variants
    Color thumb_color = slider->custom_thumb_color ? *slider->custom_thumb_color : current_theme->accent;
    if (slider->dragging) {
        thumb_color = slider->custom_thumb_color ? darken_color(*slider->custom_thumb_color, 0.2f) : current_theme->accent_pressed;
    } else if (slider->is_hovered) {
        thumb_color = slider->custom_thumb_color ? lighten_color(*slider->custom_thumb_color, 0.1f) : current_theme->accent_hovered;
    }

    // Draw thumb as rectangle
    draw_rect_(base, sthumb_x - (thumb_width / 2), sy, thumb_width, sh, thumb_color);

    // Draw label if exists (positioned to the right of the slider, centered vertically)
    if (slider->label) {
        Color label_color = slider->custom_label_color ? *slider->custom_label_color : current_theme->text_secondary;
        draw_text_(base, slider->label, font_size, sx + sw + label_pad, sy + (sh / 2) - label_v_offset, label_color);
    }
    // Reset clipping
    SDL_RenderSetClipRect(slider->parent->base.sdl_renderer, NULL);
}

// -------- Update --------
static inline void update_slider(Slider* slider, SDL_Event event) {
    if (!slider || !slider->parent || !slider->parent->is_open) {
        printf("Invalid slider, parent, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent (logical)
    int abs_x = slider->x + slider->parent->x;
    int abs_y = slider->y + slider->parent->y + slider->parent->title_height;

    // Calculate thumb position and bounds (logical)
    float range = slider->max - slider->min;
    float pos_ratio = (slider->value - slider->min) / range;
    int thumb_x = abs_x + (int)(pos_ratio * slider->w);
    int thumb_width = 10;  // Fixed width logical for hit testing
    int thumb_height = slider->h;
    SDL_Rect thumb_rect = {thumb_x - (thumb_width / 2), abs_y, thumb_width, thumb_height};

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Check if mouse is over the thumb for hover
    int over_thumb = (mouse_x >= thumb_rect.x && mouse_x <= thumb_rect.x + thumb_rect.w &&
                      mouse_y >= thumb_rect.y && mouse_y <= thumb_rect.y + thumb_rect.h);

    if (event.type == SDL_MOUSEMOTION) {
        slider->is_hovered = over_thumb;
        if (slider->dragging) {
            // Update value based on mouse position, clamped to slider bounds
            int new_value = slider->min + (int)(((mouse_x - abs_x) / (float)slider->w) * range);
            if (new_value < slider->min) new_value = slider->min;
            if (new_value > slider->max) new_value = slider->max;
            slider->value = new_value;
        }
    } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (over_thumb) {
            slider->dragging = true;
        }
    } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        slider->dragging = false;
    }
}

// -------- Free --------
static inline void free_slider(Slider* slider) {
    if (slider) {
        free(slider->label);
        if (slider->custom_track_color) free(slider->custom_track_color);
        if (slider->custom_thumb_color) free(slider->custom_thumb_color);
        if (slider->custom_label_color) free(slider->custom_label_color);
    }
}


#define MAX_SLIDERS 100
static Slider* sliders[MAX_SLIDERS];
static int sliders_count = 0;

// -------- Register --------
static inline void register_slider(Slider* slider) {
    if (sliders_count < MAX_SLIDERS) {
        sliders[sliders_count++] = slider;
    }
}

// -------- Helpers for all Sliders --------
static inline void render_all_registered_sliders(void) {
    for (int i = 0; i < sliders_count; i++) {
        if (sliders[i]) {
            render_slider(sliders[i]);
        }
    }
}

static inline void update_all_registered_sliders(SDL_Event event) {
    for (int i = 0; i < sliders_count; i++) {
        if (sliders[i]) {
            update_slider(sliders[i], event);
        }
    }
}
static inline void free_all_registered_sliders(void) {
    for (int i = 0; i < sliders_count; i++) {
        if (sliders[i]) {
            free_slider(sliders[i]);
            sliders[i] = NULL;
        }
    }
    sliders_count = 0;
}
#endif // SLIDER_H
