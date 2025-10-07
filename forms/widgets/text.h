/**
 * @file text.h
 * @brief Contains logic for text widgets (labels) using SDL2
 */

#ifndef TEXT_H
#define TEXT_H

#include <stdlib.h> // for malloc
#include <string.h> // for strdup
#include <SDL2/SDL.h> // for SDL_Event, etc.
#include <SDL2/SDL_ttf.h> // for TTF_Font


typedef struct {
    Parent* parent;            // Pointer to the parent window or container
    int x, y;                  // Position of the text
    char* content;             // Text content
    int font_size;             // Font size in points
    Color color;               // Text color
    TextAlign align;           // Alignment (LEFT, CENTER, RIGHT)
} Text;

void register_widget_text(Text* text);

Text* new_text_(Parent* parent, int x, int y, const char* content, int font_size, Color color, TextAlign align) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    Text* new_text = (Text*)malloc(sizeof(Text));
    if (!new_text) {
        printf("Failed to allocate memory for text widget\n");
        return NULL;
    }

    new_text->parent = parent;
    new_text->x = x;
    new_text->y = y;
    new_text->content = strdup(content);
    if (!new_text->content) {
        printf("Failed to allocate memory for text content\n");
        free(new_text);
        return NULL;
    }
    new_text->font_size = font_size;
    new_text->color = color;
    new_text->align = align;

    // Register widget
    register_widget_text(new_text);
    return new_text;
}

void render_text(Text* text) {
    if (!text || !text->parent || !text->parent->base.sdl_renderer || !text->parent->is_open) {
        printf("Invalid text widget, renderer, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent
    int abs_x = text->x + text->parent->x;
    int abs_y = text->y + text->parent->y + text->parent->title_height;

    // Draw the text
    if (text->content) {
        TTF_Font* font = TTF_OpenFont(FONT_FILE, text->font_size);
        if (font) {
            draw_text_from_font_(&(text->parent->base), font, text->content, abs_x, abs_y, text->color, text->align);
            TTF_CloseFont(font);
        } else {
            printf("Failed to load font for text rendering\n");
        }
    }
}

void update_text(Text* text, SDL_Event event) {
    // Text widgets are static, no updates needed for events
    (void)text;
    (void)event;
}

void free_text(Text* text) {
    if (text) {
        free(text->content);
        free(text);
    }
}

// Registration
#define MAX_TEXTS 100

Text* text_widgets[MAX_TEXTS];
int texts_count = 0;

void register_widget_text(Text* text) {
    if (texts_count < MAX_TEXTS) {
        text_widgets[texts_count] = text;
        texts_count++;
    }
}

void render_all_registered_texts(void) {
    for (int i = 0; i < texts_count; i++) {
        if (text_widgets[i]) {
            render_text(text_widgets[i]);
        }
    }
}

void update_all_registered_texts(SDL_Event event) {
    for (int i = 0; i < texts_count; i++) {
        if (text_widgets[i]) {
            update_text(text_widgets[i], event);
        }
    }
}

#endif // TEXT_H
