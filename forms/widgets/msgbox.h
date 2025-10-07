/**
 * @file message_box.h
 * @brief Contains logic for message box widgets using SDL2
 *        Composes existing widgets: Container (Parent), Text, and Button.
 */

#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <SDL2/SDL.h> // for SDL_Event, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText
#include <math.h>   // For roundf in scaling


typedef enum {
    MSG_INFO,
    MSG_WARNING,
    MSG_ERROR,
    MSG_CONFIRM  // For Yes/No
} MessageType;

typedef struct {
    Parent* parent;              // Pointer to the root window
    Parent* dialog_container;    // Container for the message box
    Text* title_text;            // Title text widget (optional if container has title)
    Text* message_text;          // Message body text widget
    Button* ok_button;           // OK or Yes button
    Button* cancel_button;       // Cancel or No button (NULL if not needed)
    MessageType type;            // Type of message (affects styling/icon if extended)
    void (*ok_callback)(void);   // Callback for OK/Yes
    void (*cancel_callback)(void); // Callback for Cancel/No (optional)
    bool is_open;                // Whether the message box is visible
    bool is_modal;               // Blocks other input (simulated by priority)
} MessageBox;

#define MAX_MESSAGE_BOXES 100
static MessageBox* message_box_widgets[MAX_MESSAGE_BOXES];
static int message_boxes_count = 0;

static inline void register_widget_message_box(MessageBox* message_box);

// -------- Create --------
static inline MessageBox* new_message_box_(Parent* root, const char* title, const char* message, MessageType type, void (*ok_callback)(void), void (*cancel_callback)(void)) {
    if (!root || !root->base.sdl_renderer || !root->is_window) {
        printf("Invalid root window or renderer\n");
        return NULL;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    MessageBox* message_box = (MessageBox*)malloc(sizeof(MessageBox));
    if (!message_box) {
        printf("Failed to allocate MessageBox\n");
        return NULL;
    }

    // Compute sizes based on theme (logical)
    int padding = current_theme->padding;
    int font_size = current_theme->default_font_size;
    int button_height = font_size + 2 * padding;
    int button_width = 80;  // Logical, adjustable
    int dialog_width = 300; // Logical default
    int dialog_height = 150; // Logical default, adjust based on content

    // Estimate dialog size based on message text
    TTF_Font* font = TTF_OpenFont(current_theme->font_file ? current_theme->font_file : "FreeMono.ttf", font_size);
    if (font) {
        int text_w, text_h;
        TTF_SizeText(font, message, &text_w, &text_h);
        dialog_width = text_w + 2 * padding > 300 ? text_w + 2 * padding : 300;
        dialog_height = text_h + button_height + 3 * padding + (current_theme->title_height ? current_theme->title_height : font_size);
        TTF_CloseFont(font);
    }

    // Create container (centered in root)
    float dpi = root->base.dpi_scale;
    int dialog_x = (int)roundf((root->w - dialog_width) / 2);
    int dialog_y = (int)roundf((root->h - dialog_height) / 2);
    message_box->dialog_container = new_container_(root, dialog_x, dialog_y, dialog_width, dialog_height);
    if (!message_box->dialog_container) {
        free(message_box);
        return NULL;
    }
    set_container_properties_(message_box->dialog_container, false, title, true, false);  // Title bar, no move/close/resize

    // Add message text
    message_box->message_text = new_text_(message_box->dialog_container, padding, message_box->dialog_container->title_height + padding, message, font_size, current_theme->text_primary, ALIGN_CENTER);
    if (!message_box->message_text) {
        free_con_(message_box->dialog_container);
        free(message_box);
        return NULL;
    }

    // Add buttons (centered at bottom)
    int buttons_y = dialog_height - button_height - padding;
    if (type == MSG_CONFIRM) {
        // Yes/No buttons
        message_box->ok_button = new_button_(message_box->dialog_container, (dialog_width / 2) - button_width - padding / 2, buttons_y, button_width, button_height, "Yes", ok_callback);
        message_box->cancel_button = new_button_(message_box->dialog_container, (dialog_width / 2) + padding / 2, buttons_y, button_width, button_height, "No", cancel_callback);
    } else {
        // OK button only
        message_box->ok_button = new_button_(message_box->dialog_container, (dialog_width - button_width) / 2, buttons_y, button_width, button_height, "OK", ok_callback);
        message_box->cancel_button = NULL;
    }
    if (!message_box->ok_button) {
        free_text_(message_box->message_text);
        free_con_(message_box->dialog_container);
        free(message_box);
        return NULL;
    }
    if (type == MSG_CONFIRM && !message_box->cancel_button) {
        free_button(message_box->ok_button);
        free_text_(message_box->message_text);
        free_con_(message_box->dialog_container);
        free(message_box);
        return NULL;
    }

    message_box->parent = root;
    message_box->title_text = NULL;  // Title handled by container
    message_box->type = type;
    message_box->ok_callback = ok_callback;
    message_box->cancel_callback = cancel_callback;
    message_box->is_open = true;
    message_box->is_modal = true;

    register_widget_message_box(message_box);
    return message_box;
}

// -------- Render --------
static inline void render_message_box(MessageBox* message_box) {
    if (!message_box || !message_box->is_open || !message_box->parent || !message_box->parent->base.sdl_renderer) {
        return;
    }

    // Fallback if no theme
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Render container (includes title bar and background)
    render_container(message_box->dialog_container);

    // Render message text
    if (message_box->message_text) {
        float dpi = message_box->parent->base.dpi_scale;
        int abs_x = message_box->message_text->x + message_box->dialog_container->x + message_box->parent->x;
        int abs_y = message_box->message_text->y + message_box->dialog_container->y + message_box->parent->y;
        int font_size = (int)roundf(message_box->message_text->font_size * dpi);
        TTF_Font* font = TTF_OpenFont(current_theme->font_file ? current_theme->font_file : "FreeMono.ttf", font_size);
        if (font) {
            draw_text_from_font_(&message_box->parent->base, font, message_box->message_text->text, 
                                (int)roundf(abs_x * dpi), (int)roundf(abs_y * dpi), 
                                message_box->message_text->color, message_box->message_text->align);
            TTF_CloseFont(font);
        }
    }

    // Render buttons
    if (message_box->ok_button) render_button(message_box->ok_button);
    if (message_box->cancel_button) render_button(message_box->cancel_button);
}

// -------- Update --------
static inline void update_message_box(MessageBox* message_box, SDL_Event event) {
    if (!message_box || !message_box->is_open || !message_box->parent) {
        return;
    }

    // If modal, block other widgets (handled by updating message box first)
    update_container(message_box->dialog_container, event);

    // Update buttons
    if (message_box->ok_button) update_button(message_box->ok_button, event);
    if (message_box->cancel_button) update_button(message_box->cancel_button, event);

    // Note: Button callbacks handle app logic; they can call close_message_box if needed
}

// -------- Close --------
static inline void close_message_box(MessageBox* message_box) {
    if (message_box) {
        message_box->is_open = false;
        if (message_box->message_text) free_text_(message_box->message_text);
        if (message_box->ok_button) free_button(message_box->ok_button);
        if (message_box->cancel_button) free_button(message_box->cancel_button);
        if (message_box->dialog_container) free_con_(message_box->dialog_container);
    }
}

// -------- Free --------
static inline void free_message_box(MessageBox* message_box) {
    if (message_box) {
        close_message_box(message_box);
        free(message_box);
    }
}

// -------- Helpers for all Message Boxes --------
static inline void register_widget_message_box(MessageBox* message_box) {
    if (message_boxes_count < MAX_MESSAGE_BOXES) {
        message_box_widgets[message_boxes_count++] = message_box;
    }
}

static inline void render_all_registered_message_boxes(void) {
    for (int i = 0; i < message_boxes_count; i++) {
        if (message_box_widgets[i]) {
            render_message_box(message_box_widgets[i]);
        }
    }
}

static inline void update_all_registered_message_boxes(SDL_Event event) {
    for (int i = 0; i < message_boxes_count; i++) {
        if (message_box_widgets[i]) {
            update_message_box(message_box_widgets[i], event);
        }
    }
}

#endif // MESSAGE_BOX_H
