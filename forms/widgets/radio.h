#ifndef RADIO_H
#define RADIO_H


typedef struct {
    Parent* parent;      // Parent container or window
    int x, y;            // Position
    int w, h;            // Size
    char* label;         // Label text
    bool selected;       // Is selected?
    int group_id;        // Group ID (1 group → only 1 selected)
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
    radio->selected = false;
    radio->group_id = group_id;

    register_widget_radio(radio);
    return radio;
}

// -------- Render --------
static inline void render_radio_(Radio* radio) {
    Base* base = &radio->parent->base;

    // Outer circle
    draw_circle_(base, radio->x, radio->y, radio->h / 2,
                 (Color){200, 200, 200, 255});

    // Inner circle if selected
    if (radio->selected) {
        draw_circle_(base, radio->x, radio->y, (radio->h / 2) - 4,
                     (Color){50, 150, 255, 255});
    }

    // Label text
    draw_text_(base, radio->label, 16,
               radio->x + radio->h, radio->y - (radio->h / 3),
               (Color){255, 255, 255, 255});
}

// -------- Update --------
static inline void update_radio_(Radio* radio, SDL_Event event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mx = event.button.x;
        int my = event.button.y;

        // Hitbox: circle bounding box
        if (mx >= radio->x - radio->h/2 && mx <= radio->x + radio->h/2 &&
            my >= radio->y - radio->h/2 && my <= radio->y + radio->h/2) {

            // Deselect others in same group
            for (int i = 0; i < radios_count; i++) {
                if (radio_widgets[i]->group_id == radio->group_id) {
                    radio_widgets[i]->selected = false;
                }
            }
            radio->selected = true;
        }
    }
}

// -------- Helpers for all radios --------
static inline void render_all_registered_radios(void) {
    for (int i = 0; i < radios_count; i++) {
        render_radio_(radio_widgets[i]);
    }
}

static inline void update_all_registered_radios(SDL_Event event) {
    for (int i = 0; i < radios_count; i++) {
        update_radio_(radio_widgets[i], event);
    }
}

#endif // RADIO_H