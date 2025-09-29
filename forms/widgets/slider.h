#ifndef SLIDER_H
#define SLIDER_H

#include <SDL2/SDL.h>


typedef struct {
    Parent* parent;      // Parent container or window
    int x, y;            // Position
    int w, h;            // Width and height (assume horizontal slider, w > h)
    int min, max;        // Range of values
    int value;           // Current value
    char* label;         // Optional label text
    bool dragging;       // Flag to track if thumb is being dragged
} Slider;

#define MAX_SLIDERS 100
static Slider* sliders[MAX_SLIDERS];
static int sliders_count = 0;

// -------- Register --------
static inline void register_widget_slider(Slider* slider) {
    if (sliders_count < MAX_SLIDERS) {
        sliders[sliders_count++] = slider;
    }
}

// -------- Create --------
static inline Slider* new_slider(Parent* parent, int x, int y, int w, int h, int min, int max, int start_value, const char* label) {
    Slider* slider = (Slider*)malloc(sizeof(Slider));
    if (!slider) {
        printf("Failed to allocate Slider\n");
        return NULL;
    }
    slider->parent = parent;
    slider->x = x;
    slider->y = y;
    slider->w = w;
    slider->h = h;
    slider->min = min;
    slider->max = max;
    slider->value = start_value;
    slider->label = label ? strdup(label) : NULL;  // Copy label if provided
    slider->dragging = false;

    register_widget_slider(slider);
    return slider;
}

// -------- Render --------
static inline void render_slider(Slider* slider) {
    Base* base = &slider->parent->base;

    // Draw track (horizontal bar)
    int track_height = 4;  // Thin track
    draw_rect_(base, slider->x, slider->y + (slider->h / 2) - (track_height / 2), slider->w, track_height, (Color){128, 128, 128, 255});  // Gray track

    // Calculate thumb position
    float range = slider->max - slider->min;
    float pos_ratio = (slider->value - slider->min) / range;
    int thumb_x = slider->x + (int)(pos_ratio * slider->w);
    int thumb_width = 10;  // Fixed width for rectangular thumb
    int thumb_height = slider->h;  // Full height

    // Draw thumb as rectangle
    draw_rect_(base, thumb_x - (thumb_width / 2), slider->y, thumb_width, thumb_height, (Color){0, 0, 255, 255});  // Blue thumb

    // Draw label if exists (positioned to the right of the slider, centered vertically)
    if (slider->label) {
        draw_text_(base, slider->label, 16, slider->x + slider->w + 10, slider->y + (slider->h / 2) - 8, (Color){255, 255, 255, 255});
    }
}

// -------- Update --------
static inline void update_slider(Slider* slider, SDL_Event event) {
    Base* base = &slider->parent->base;  // Not directly used here, but available if needed

    // Calculate thumb position and bounds
    float range = slider->max - slider->min;
    float pos_ratio = (slider->value - slider->min) / range;
    int thumb_x = slider->x + (int)(pos_ratio * slider->w);
    int thumb_width = 10;  // Fixed width for rectangular thumb
    int thumb_height = slider->h;
    SDL_Rect thumb_rect = {thumb_x - (thumb_width / 2), slider->y, thumb_width, thumb_height};

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouse_x = event.button.x;
        int mouse_y = event.button.y;
        if (mouse_x >= thumb_rect.x && mouse_x <= thumb_rect.x + thumb_rect.w &&
            mouse_y >= thumb_rect.y && mouse_y <= thumb_rect.y + thumb_rect.h) {
            slider->dragging = true;
        }
    } else if (event.type == SDL_MOUSEMOTION && slider->dragging) {
        int mouse_x = event.motion.x;
        // Update value based on mouse position, clamped to slider bounds
        int new_value = slider->min + (int)(((mouse_x - slider->x) / (float)slider->w) * range);
        if (new_value < slider->min) new_value = slider->min;
        if (new_value > slider->max) new_value = slider->max;
        slider->value = new_value;
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        slider->dragging = false;
    }
}

// -------- Helpers for all Sliders --------
static inline void render_all_registered_sliders(void) {
    for (int i = 0; i < sliders_count; i++) {
        render_slider(sliders[i]);
    }
}

static inline void update_all_registered_sliders(SDL_Event event) {
    for (int i = 0; i < sliders_count; i++) {
        update_slider(sliders[i], event);
    }
}

#endif // SLIDER_H