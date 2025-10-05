/**
 * @file drop.h
 * @brief Contains logic for dropdown widgets using SDL2
 */

#ifndef DROP_H
#define DROP_H

#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <SDL2/SDL.h> // for SDL_Event, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText if needed


typedef struct {
    Parent* parent;         // Parent window or container
    int x, y;               // Position relative to parent
    int w, h;               // Width and height of the dropdown button
    char** options;         // Array of option strings (caller-managed)
    int option_count;       // Number of options
    int selected_index;     // Index of currently selected option (-1 = none)
    bool is_expanded;       // Whether the dropdown is open
    bool is_hovered;        // Whether mouse is over the dropdown button
    int font_size;          // Font size (overridable, defaults to theme)
    char* place_holder;     // Placeholder text when no option is selected
    // Theme overrides (NULL = use theme)
    Color* custom_bg_color;       // Background for options
    Color* custom_button_color;   // Background for dropdown button
    Color* custom_text_color;     // Text color
    Color* custom_highlight_color; // Highlight for selected/hovered option
} Drop;

#define MAX_DROPS 100
static Drop* drop_widgets[MAX_DROPS];
static int drops_count = 0;

static inline void register_widget_drop(Drop* drop);

Drop* new_drop_down_(Parent* parent, int x, int y, int w, int h, char** options, int option_count) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    Drop* drop = (Drop*)malloc(sizeof(Drop));
    if (!drop) {
        printf("Failed to allocate memory for Drop\n");
        return NULL;
    }

    // Initialize fields
    drop->parent = parent;
    drop->x = x;
    drop->y = y;
    drop->w = w;
    drop->h = h;
    drop->options = options; // Caller manages options memory
    drop->option_count = option_count;
    drop->selected_index = -1; // No option selected by default
    drop->is_expanded = false;
    drop->is_hovered = false;
    drop->font_size = 0; // 0 = use theme default
    drop->place_holder = strdup("select option"); // Copy string to avoid issues
    if (!drop->place_holder) {
        free(drop);
        printf("Failed to allocate memory for placeholder\n");
        return NULL;
    }
    // Init overrides to NULL (use theme)
    drop->custom_bg_color = NULL;
    drop->custom_button_color = NULL;
    drop->custom_text_color = NULL;
    drop->custom_highlight_color = NULL;

    // Register the dropdown
    register_widget_drop(drop);
    return drop;
}

// Setters for overrides
static inline void set_drop_bg_color(Drop* drop, Color color) {
    if (drop) {
        if (!drop->custom_bg_color) {
            drop->custom_bg_color = (Color*)malloc(sizeof(Color));
        }
        *drop->custom_bg_color = color;
    }
}

static inline void set_drop_button_color(Drop* drop, Color color) {
    if (drop) {
        if (!drop->custom_button_color) {
            drop->custom_button_color = (Color*)malloc(sizeof(Color));
        }
        *drop->custom_button_color = color;
    }
}

static inline void set_drop_text_color(Drop* drop, Color color) {
    if (drop) {
        if (!drop->custom_text_color) {
            drop->custom_text_color = (Color*)malloc(sizeof(Color));
        }
        *drop->custom_text_color = color;
    }
}

static inline void set_drop_highlight_color(Drop* drop, Color color) {
    if (drop) {
        if (!drop->custom_highlight_color) {
            drop->custom_highlight_color = (Color*)malloc(sizeof(Color));
        }
        *drop->custom_highlight_color = color;
    }
}

static inline void set_drop_font_size(Drop* drop, int size) {
    if (drop) {
        drop->font_size = size;
    }
}

static inline void draw_upside_down_triangle_(Base* base, int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    // Draw a downward-pointing triangle by defining vertices directly
    // (x1, y1) and (x2, y2) form the top horizontal line, (x3, y3) is the bottom point
    draw_triangle_(base, x1, y1, x2, y2, x3, y3, color);
}

void render_drop_down_(Drop* drop) {
    if (!drop || !drop->parent || !drop->parent->base.sdl_renderer || !drop->parent->is_open) {
        printf("Invalid drop, renderer, or parent is not open\n");
        return;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }

    // Calculate absolute position relative to parent
    int abs_x = drop->x + drop->parent->x;
    int abs_y = drop->y + drop->parent->y;

    Base* base = &drop->parent->base;
    int effective_font_size = drop->font_size > 0 ? drop->font_size : current_theme->default_font_size;

    // Get colors from theme/overrides
    Color button_color = drop->custom_button_color ? *drop->custom_button_color : current_theme->button_normal;
    if (drop->is_hovered) {
        button_color = drop->custom_button_color ? lighten_color(*drop->custom_button_color, 0.1f) : current_theme->button_hovered;
    }
    Color bg_color = drop->custom_bg_color ? *drop->custom_bg_color : current_theme->bg_secondary;
    Color text_color = drop->custom_text_color ? *drop->custom_text_color : current_theme->text_primary;
    Color highlight_color = drop->custom_highlight_color ? *drop->custom_highlight_color : current_theme->accent;

    // Draw the main dropdown button
    draw_rect_(base, abs_x, abs_y, drop->w, drop->h, button_color);

    // Draw placeholder text if no option is selected, otherwise draw selected option
    const char* display_text = (drop->selected_index >= 0 && drop->selected_index < drop->option_count)
                              ? drop->options[drop->selected_index]
                              : drop->place_holder;
    draw_text_(base, display_text, effective_font_size, abs_x + 5, abs_y + drop->h / 4, text_color);

    // Draw the dropdown arrow (down when collapsed, up when expanded)
    int arrow_size = drop->h / 4;
    int arrow_x = abs_x + drop->w - arrow_size - 5;
    int arrow_y = abs_y + drop->h / 2 - arrow_size / 2;
    if (drop->is_expanded) {
        // Upward-pointing triangle
        draw_triangle_(base,
                       arrow_x, arrow_y + arrow_size,
                       arrow_x + arrow_size, arrow_y + arrow_size,
                       arrow_x + arrow_size / 2, arrow_y,
                       text_color);
    } else {
        // Downward-pointing triangle
        draw_upside_down_triangle_(base,
                                  arrow_x, arrow_y,
                                  arrow_x + arrow_size, arrow_y,
                                  arrow_x + arrow_size / 2, arrow_y + arrow_size,
                                  text_color);
    }

    // Draw options if expanded (NO bounds check anymore)
    if (drop->is_expanded) {
        for (int i = 0; i < drop->option_count; i++) {
            int option_y = abs_y + drop->h * (i + 1);

            // Draw option background
            Color option_bg = (i == drop->selected_index) ? highlight_color : bg_color;
            draw_rect_(base, abs_x, option_y, drop->w, drop->h, option_bg);

            // Draw option text
            draw_text_(base, drop->options[i],
                       effective_font_size, abs_x + 5, option_y + drop->h / 4, text_color);
        }
    }
}

void update_drop_down_(Drop* drop, SDL_Event event) {
    if (!drop || !drop->parent || !drop->parent->is_open) {
        printf("Invalid drop, parent, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent
    int abs_x = drop->x + drop->parent->x;
    int abs_y = drop->y + drop->parent->y;

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Check if mouse is over the dropdown button for hover
    bool over_button = (mouse_x >= abs_x && mouse_x <= abs_x + drop->w &&
                        mouse_y >= abs_y && mouse_y <= abs_y + drop->h);
    drop->is_hovered = over_button;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int event_mouse_x = event.button.x;
        int event_mouse_y = event.button.y;

        // Check if click is on the dropdown button
        if (event_mouse_x >= abs_x && event_mouse_x <= abs_x + drop->w &&
            event_mouse_y >= abs_y && event_mouse_y <= abs_y + drop->h) {
            drop->is_expanded = !drop->is_expanded; // Toggle dropdown
        }
        // Check if click is on an option
        else if (drop->is_expanded) {
            bool clicked_option = false;
            for (int i = 0; i < drop->option_count; i++) {
                int option_y = abs_y + drop->h * (i + 1);
                if (event_mouse_x >= abs_x && event_mouse_x <= abs_x + drop->w &&
                    event_mouse_y >= option_y && event_mouse_y <= option_y + drop->h) {
                    drop->selected_index = i;
                    drop->is_expanded = false; // Close dropdown after selection
                    clicked_option = true;
                    break;
                }
            }
            // Close dropdown if click is outside both button and options
            if (!clicked_option) {
                int options_height = drop->h * drop->option_count;
                if (event_mouse_x < abs_x || event_mouse_x > abs_x + drop->w ||
                    event_mouse_y < abs_y || event_mouse_y > abs_y + drop->h + options_height) {
                    drop->is_expanded = false;
                }
            }
        }
        // Click outside closes the dropdown
        else {
            drop->is_expanded = false;
        }
    }
}

void free_drop_(Drop* drop) {
    if (!drop) return;
    free(drop->place_holder); // Free the duplicated placeholder string
    if (drop->custom_bg_color) free(drop->custom_bg_color);
    if (drop->custom_button_color) free(drop->custom_button_color);
    if (drop->custom_text_color) free(drop->custom_text_color);
    if (drop->custom_highlight_color) free(drop->custom_highlight_color);
    free(drop);
}

static inline void register_widget_drop(Drop* drop) {
    if (drops_count < MAX_DROPS) {
        drop_widgets[drops_count] = drop;
        drops_count++;
    }
}

static inline void render_all_registered_drops(void) {
    for (int i = 0; i < drops_count; i++) {
        if (drop_widgets[i]) {
            render_drop_down_(drop_widgets[i]);
        }
    }
}

static inline void update_all_registered_drops(SDL_Event event) {
    for (int i = 0; i < drops_count; i++) {
        if (drop_widgets[i]) {
            update_drop_down_(drop_widgets[i], event);
        }
    }
}

#endif // DROP_H
