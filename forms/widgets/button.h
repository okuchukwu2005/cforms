#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <SDL2/SDL.h> // for SDL_Event, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText usage
#include "graphics.h" // for draw functions
#include "color.h" // for Color

typedef struct {
    Parent* parent;            // Pointer to the parent window or container
    int x, y;                  // Position of the button
    int w, h;                  // Width and height of the button
    char* label;               // Button label text
    void (*callback)(void);    // Callback function on click
    int is_hovered;            // Is the mouse hovering over the button?
    int is_pressed;            // Is the button pressed?
} Button;

void register_widget_button(Button* button);

Button* new_button_(Parent* parent, int x, int y, int w, int h, const char* label, void (*callback)(void)) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    Button* new_button = (Button*)malloc(sizeof(Button));
    if (!new_button) {
        printf("Failed to allocate memory for button\n");
        return NULL;
    }

    new_button->parent = parent;
    new_button->x = x;
    new_button->y = y;
    new_button->w = w;
    new_button->h = h;
    new_button->label = strdup(label);
    if (!new_button->label) {
        printf("Failed to allocate memory for button label\n");
        free(new_button);
        return NULL;
    }
    new_button->callback = callback;
    new_button->is_hovered = 0;
    new_button->is_pressed = 0;

    // Register widget
    register_widget_button(new_button);
    return new_button;
}

void render_button(Button* button) {
    if (!button || !button->parent || !button->parent->base.sdl_renderer || !button->parent->is_open) {
        printf("Invalid button, renderer, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent
    int abs_x = button->x + button->parent->x;
    int abs_y = button->y + button->parent->y;
    const int corner_radius = 10; // Radius for rounded corners

    // Determine background color based on state
    Color bg_color = COLOR_BLUE; // Default blue background
    if (button->is_pressed) {
        bg_color = COLOR_NAVY; // Darker blue when pressed
    } else if (button->is_hovered) {
        bg_color = COLOR_CYAN; // Lighter blue when hovered
    }

    // Draw rounded rectangle (approximated with circles and rectangles)
    // Draw four corner circles
    draw_circle_(&button->parent->base, abs_x + corner_radius, abs_y + corner_radius, corner_radius, bg_color);
    draw_circle_(&button->parent->base, abs_x + button->w - corner_radius, abs_y + corner_radius, corner_radius, bg_color);
    draw_circle_(&button->parent->base, abs_x + corner_radius, abs_y + button->h - corner_radius, corner_radius, bg_color);
    draw_circle_(&button->parent->base, abs_x + button->w - corner_radius, abs_y + button->h - corner_radius, corner_radius, bg_color);

    // Draw rectangles to fill the body
    draw_rect_(&button->parent->base, abs_x + corner_radius, abs_y, button->w - 2 * corner_radius, button->h, bg_color); // Middle
    draw_rect_(&button->parent->base, abs_x, abs_y + corner_radius, corner_radius, button->h - 2 * corner_radius, bg_color); // Left
    draw_rect_(&button->parent->base, abs_x + button->w - corner_radius, abs_y + corner_radius, corner_radius, button->h - 2 * corner_radius, bg_color); // Right

    // Draw border (approximated as a slightly larger rounded rectangle outline)
    Color border_color = COLOR_BLACK;
    draw_circle_(&button->parent->base, abs_x + corner_radius, abs_y + corner_radius, corner_radius, border_color);
    draw_circle_(&button->parent->base, abs_x + button->w - corner_radius, abs_y + corner_radius, corner_radius, border_color);
    draw_circle_(&button->parent->base, abs_x + corner_radius, abs_y + button->h - corner_radius, corner_radius, border_color);
    draw_circle_(&button->parent->base, abs_x + button->w - corner_radius, abs_y + button->h - corner_radius, corner_radius, border_color);
    draw_rect_(&button->parent->base, abs_x + corner_radius, abs_y, button->w - 2 * corner_radius, 1, border_color); // Top
    draw_rect_(&button->parent->base, abs_x + corner_radius, abs_y + button->h - 1, button->w - 2 * corner_radius, 1, border_color); // Bottom
    draw_rect_(&button->parent->base, abs_x, abs_y + corner_radius, 1, button->h - 2 * corner_radius, border_color); // Left
    draw_rect_(&button->parent->base, abs_x + button->w - 1, abs_y + corner_radius, 1, button->h - 2 * corner_radius, border_color); // Right

    // Draw inner fill to smooth out border
    draw_rect_(&button->parent->base, abs_x + corner_radius, abs_y + 1, button->w - 2 * corner_radius, button->h - 2, bg_color);
    draw_rect_(&button->parent->base, abs_x + 1, abs_y + corner_radius, corner_radius - 1, button->h - 2 * corner_radius, bg_color);
    draw_rect_(&button->parent->base, abs_x + button->w - corner_radius, abs_y + corner_radius, corner_radius - 1, button->h - 2 * corner_radius, bg_color);

    TTF_Font* font = TTF_OpenFont(FONT_FILE, 30);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    // Render label centered with white text
    int text_w, text_h;
    TTF_SizeText(font, button->label, &text_w, &text_h);
    int text_x = abs_x + (button->w / 2);
    int text_y = abs_y + (button->h - text_h) / 2;
    draw_text_from_font_(&button->parent->base, font, button->label, text_x, text_y, COLOR_WHITE, ALIGN_CENTER);

    TTF_CloseFont(font);
}

void update_button(Button* button, SDL_Event event) {
    if (!button || !button->parent || !button->parent->is_open) {
        printf("Invalid button, parent, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent
    int abs_x = button->x + button->parent->x;
    int abs_y = button->y + button->parent->y;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Check if mouse is over the button
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
        free(button);
    }
}

// Registration
#define MAX_BUTTONS 100

Button* button_widgets[MAX_BUTTONS];
int buttons_count = 0;

void register_widget_button(Button* button) {
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
