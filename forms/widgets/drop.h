typedef struct {
    Parent* parent;         // Parent window or container
    int x, y;               // Position relative to parent
    int w, h;               // Width and height of the dropdown button
    char** options;         // Array of option strings
    int option_count;       // Number of options
    int selected_index;     // Index of currently selected option
    bool is_expanded;       // Whether the dropdown is open
    Color bg_color;         // Background color for dropdown options
    Color bg_color0;        // Background color for dropdown button
    Color text_color;       // Text color
    Color highlight_color;  // Highlight color for selected option
    int font_size;          // Font size for draw_text_
    char* place_holder;     // Placeholder text when no option is selected
} Drop;

#define MAX_DROPS 100
static inline void register_widget_drop(Drop* drop);

Drop* new_drop_down_(Parent* parent, int x, int y, int w, int h, char** options, int option_count) {
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
    drop->bg_color = COLOR_GRAY;        // Background for options
    drop->bg_color0 = COLOR_BLACK;      // Background for button
    drop->text_color = COLOR_WHITE;
    drop->highlight_color = (Color){150, 150, 255, 255}; // Light blue for selected option
    drop->font_size = 16; // Default font size
    drop->place_holder = strdup("select option"); // Copy string to avoid issues
    if (!drop->place_holder) {
        free(drop);
        printf("Failed to allocate memory for placeholder\n");
        return NULL;
    }

    // Register the dropdown
    register_widget_drop(drop);
    return drop;
}

static inline void draw_upside_down_triangle_(Base* base, int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    // Draw a downward-pointing triangle by defining vertices directly
    // (x1, y1) and (x2, y2) form the top horizontal line, (x3, y3) is the bottom point
    draw_triangle_(base, x1, y1, x2, y2, x3, y3, color);
}

void render_drop_down_(Drop* drop) {
    if (!drop || !drop->parent) return;

    Base* base = &drop->parent->base;

    // Draw the main dropdown button
    draw_rect_(base, drop->x, drop->y, drop->w, drop->h, drop->bg_color0);

    // Draw placeholder text if no option is selected, otherwise draw selected option
    const char* display_text = (drop->selected_index >= 0 && drop->selected_index < drop->option_count)
                              ? drop->options[drop->selected_index]
                              : drop->place_holder;
    draw_text_(base, display_text, drop->font_size, drop->x + 5, drop->y + drop->h / 4, drop->text_color);

    // Draw the dropdown arrow (down when collapsed, up when expanded)
    int arrow_size = drop->h / 4;
    int arrow_x = drop->x + drop->w - arrow_size - 5;
    int arrow_y = drop->y + drop->h / 2 - arrow_size / 2;
    if (drop->is_expanded) {
        // Upward-pointing triangle
        draw_triangle_(base,
                       arrow_x, arrow_y + arrow_size,
                       arrow_x + arrow_size, arrow_y + arrow_size,
                       arrow_x + arrow_size / 2, arrow_y,
                       drop->text_color);
    } else {
        // Downward-pointing triangle
        draw_upside_down_triangle_(base,
                                  arrow_x, arrow_y,
                                  arrow_x + arrow_size, arrow_y,
                                  arrow_x + arrow_size / 2, arrow_y + arrow_size,
                                  drop->text_color);
    }

    // Draw options if expanded (NO bounds check anymore)
    if (drop->is_expanded) {
        for (int i = 0; i < drop->option_count; i++) {
            int option_y = drop->y + drop->h * (i + 1);

            // Draw option background
            draw_rect_(base, drop->x, option_y, drop->w, drop->h,
                       i == drop->selected_index ? drop->highlight_color : drop->bg_color);

            // Draw option text
            draw_text_(base, drop->options[i],
                       drop->font_size, drop->x + 5, option_y + drop->h / 4, drop->text_color);
        }
    }
}
void update_drop_down_(Drop* drop, SDL_Event event) {
    if (!drop || !drop->parent) return;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouse_x = event.button.x;
        int mouse_y = event.button.y;

        // Check if click is on the dropdown button
        if (mouse_x >= drop->x && mouse_x <= drop->x + drop->w &&
            mouse_y >= drop->y && mouse_y <= drop->y + drop->h) {
            drop->is_expanded = !drop->is_expanded; // Toggle dropdown
        }
        // Check if click is on an option
        else if (drop->is_expanded) {
            bool clicked_option = false;
            for (int i = 0; i < drop->option_count; i++) {
                int option_y = drop->y + drop->h * (i + 1);
                if (mouse_x >= drop->x && mouse_x <= drop->x + drop->w &&
                    mouse_y >= option_y && mouse_y <= option_y + drop->h) {
                    drop->selected_index = i;
                    drop->is_expanded = false; // Close dropdown after selection
                    clicked_option = true;
                    break;
                }
            }
            // Close dropdown if click is outside both button and options
            if (!clicked_option) {
                int options_height = drop->h * drop->option_count;
                if (mouse_x < drop->x || mouse_x > drop->x + drop->w ||
                    mouse_y < drop->y || mouse_y > drop->y + drop->h + options_height) {
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
    free(drop);
}

static Drop* drop_widgets[MAX_DROPS];
static int drops_count = 0;

static inline void register_widget_drop(Drop* drop) {
    if (drops_count < MAX_DROPS) {
        drop_widgets[drops_count] = drop;
        drops_count++;
    }
}

static inline void render_all_registered_drops(void) {
    for (int i = 0; i < drops_count; i++) {
        render_drop_down_(drop_widgets[i]);
    }
}

static inline void update_all_registered_drops(SDL_Event event) {
    for (int i = 0; i < drops_count; i++) {
        update_drop_down_(drop_widgets[i], event);
    }
}