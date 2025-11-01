/**
 * @file button.h
 * @brief Contains logic for button widgets using SDL2
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <SDL2/SDL.h> // for SDL_Event, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText usage
#include <math.h>   // For roundf in scaling

void OVERRIDE(void) {
    printf("Button was clicked!\n");
    // Add custom logic, e.g., open a dialog, submit a form, etc.
}

typedef struct {
    Parent* parent;            // Pointer to the parent window or container
    int x, y;                  // Position of the button (logical)
    int w, h;                  // Width and height of the button (logical)
    char* label;               // Button label text
    void (*callback)(void);    // Callback function on click
    int is_hovered;            // Is the mouse hovering over the button?
    int is_pressed;            // Is the button pressed?
    Color* custom_bg_color;    // Optional override for bg color (NULL = use theme)
    Color* custom_text_color;  // Optional override for text color (NULL = use theme)
} Button;


Button new_button(Parent* parent, int x, int y, int w, int h, const char* label, void (*callback)(void)) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
    }

    Button new_button;

    new_button.parent = parent;
    new_button.x = x;
    new_button.y = y;
    new_button.w = w;
    new_button.h = h;
    new_button.label = strdup(label);
    if (!new_button.label) {
        printf("Failed to allocate memory for button label\n");
    }
    new_button.callback = callback;
    new_button.is_hovered = 0;
    new_button.is_pressed = 0;
    new_button.custom_bg_color = NULL;
    new_button.custom_text_color = NULL;

    return new_button;
}

// Setter for bg color override
void set_button_bg_color(Button* button, Color color) {
    if (button) {
        if (!button->custom_bg_color) {
            button->custom_bg_color = (Color*)malloc(sizeof(Color));
        }
        *button->custom_bg_color = color;
    }
}

// Setter for text color override
void set_button_text_color(Button* button, Color color) {
    if (button) {
        if (!button->custom_text_color) {
            button->custom_text_color = (Color*)malloc(sizeof(Color));
        }
        *button->custom_text_color = color;
    }
}

void render_button(Button* button) {
    if (!button || !button->parent || !button->parent->base.sdl_renderer || !button->parent->is_open) {
        printf("Invalid button, renderer, or parent is not open\n");
        return;
    }
	//set container clipping
	if(button->parent->is_window == false){
	SDL_Rect parent_bounds = get_parent_rect(button->parent);
	SDL_RenderSetClipRect(button->parent->base.sdl_renderer, &parent_bounds);
	}

    // Fallback if no theme set
    if (!current_theme) {
        // Use hardcoded defaults (your original colors)
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }

    float dpi = button->parent->base.dpi_scale;
    // Calculate absolute position relative to parent (logical), then scale
    int abs_x = button->x + button->parent->x;
    int abs_y = button->y + button->parent->y + button->parent->title_height;  // <-- Key offset here
    int sx = (int)roundf(abs_x * dpi);
    int sy = (int)roundf(abs_y * dpi);
    int sw = (int)roundf(button->w * dpi);
    int sh = (int)roundf(button->h * dpi);
    float roundness = current_theme->roundness;  // Roundness is a ratio (0-1), no scaling needed
    int font_size = (int)roundf(current_theme->default_font_size * dpi);

    // Determine bg color: custom > theme state variants
    Color button_color = button->custom_bg_color ? *button->custom_bg_color : current_theme->button_normal;
    if (button->is_pressed) {
        button_color = button->custom_bg_color ? darken_color(*button->custom_bg_color, 0.2f) : current_theme->button_pressed;
    } else if (button->is_hovered) {
        button_color = button->custom_bg_color ? lighten_color(*button->custom_bg_color, 0.1f) : current_theme->button_hovered;
    }

    // Draw rounded rectangle (use theme roundness)
    draw_rounded_rect_(&(button->parent->base), sx, sy, sw, sh, roundness, button_color);

    // Draw text centered
    if (button->label) {
        char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
        TTF_Font* font = TTF_OpenFont(font_file, font_size);
        if (font) {
            int text_w, text_h;
            TTF_SizeText(font, button->label, &text_w, &text_h);
            int text_x = sx + (sw - text_w) / 2;
            int text_y = sy + (sh - text_h) / 2;
            Color text_color = button->custom_text_color ? *button->custom_text_color : current_theme->button_text;
            draw_text_from_font_(&(button->parent->base), font, button->label, text_x, text_y, text_color, ALIGN_LEFT);
            TTF_CloseFont(font);
        }
    }
    // Reset clipping
    SDL_RenderSetClipRect(button->parent->base.sdl_renderer, NULL);
}

void update_button(Button* button, SDL_Event event) {
    if (!button || !button->parent || !button->parent->is_open) {
        printf("Invalid button, parent, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent (logical)
    int abs_x = button->x + button->parent->x;
    int abs_y = button->y + button->parent->y+ button->parent->title_height;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Check if mouse is over the button (logical)
    int over = (mouseX >= abs_x && mouseX <= abs_x + button->w &&
                mouseY >= abs_y && mouseY <= abs_y + button->h);

    if (event.type == SDL_MOUSEMOTION) {
        button->is_hovered = over;
    } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (over) {
            button->is_pressed = 1;
        }
    } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (button->is_pressed && over) {
            if (button->callback) {
                button->callback();
            }
        }
        button->is_pressed = 0;
    }
}

void free_button(Button* button) {
    if (button) {
        free(button->label);
        if (button->custom_bg_color) free(button->custom_bg_color);
        if (button->custom_text_color) free(button->custom_text_color);
    }
}

// Registration
#define MAX_BUTTONS 100

Button* button_widgets[MAX_BUTTONS];
int buttons_count = 0;

void register_button(Button* button) {
    if (buttons_count < MAX_BUTTONS) {
        button_widgets[buttons_count] = button;
        buttons_count++;
    }
}

void render_all_registered_buttons(void) {
    for (int i = 0; i < buttons_count; i++) {
        if (button_widgets[i]) {
            render_button(button_widgets[i]);
        }
    }
}

void update_all_registered_buttons(SDL_Event event) {
    for (int i = 0; i < buttons_count; i++) {
        if (button_widgets[i]) {
            update_button(button_widgets[i], event);
        }
    }
}
void free_all_registered_buttons(void) {
    for (int i = 0; i < buttons_count; i++) {
        free_button(button_widgets[i]);
        button_widgets[i] = NULL;
    }
    buttons_count = 0;
}
#endif // BUTTON_H
