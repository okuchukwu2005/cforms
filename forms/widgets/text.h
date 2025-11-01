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
    Color* color;               // Text color
    TextAlign align;           // Alignment (LEFT, CENTER, RIGHT)
} Text;

Text new_text(Parent* parent, int x, int y, const char* content, int font_size, TextAlign align) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
    }

    Text new_text;
    
    new_text.parent = parent;
    new_text.x = x;
    new_text.y = y;
    new_text.content = strdup(content);
    if (!new_text.content) {
        printf("Failed to allocate memory for text content\n");
    }
    new_text.font_size = font_size;
    new_text.color = NULL;
    new_text.align = align;

    return new_text;
}

void render_text(Text* text) {
    if (!text || !text->parent || !text->parent->base.sdl_renderer || !text->parent->is_open) {
        printf("Invalid text widget, renderer, or parent is not open\n");
        return;
    }
    //set container clipping
    if(text->parent->is_window == false){
    SDL_Rect parent_bounds = get_parent_rect(text->parent);
    SDL_RenderSetClipRect(text->parent->base.sdl_renderer, &parent_bounds);
    }

      // Fallback if no theme set
    if (!current_theme) {
        // Use hardcoded defaults (your original colors)
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }
    
	const Color* color_to_use = text->color ? text->color : &current_theme->text_primary;

    // Calculate absolute position relative to parent
    int abs_x = text->x + text->parent->x;
    int abs_y = text->y + text->parent->y + text->parent->title_height;

    // Draw the text
    if (text->content) {
        TTF_Font* font = TTF_OpenFont(FONT_FILE, text->font_size);
        if (font) {
            draw_text_from_font_(&(text->parent->base), font, text->content, abs_x, abs_y, *color_to_use, text->align);
            TTF_CloseFont(font);
        } else {
            printf("Failed to load font for text rendering\n");
        }
    }
    // Reset clipping
    SDL_RenderSetClipRect(text->parent->base.sdl_renderer, NULL);
}
// Setters for overrides
static inline void set_text_color(Text* text, Color color) {
    if (!text) return;

    if (!text->color) {
        text->color = (Color*)malloc(sizeof(Color));
        if (!text->color) {
            printf("Failed to allocate memory for text color\n");
            return;
        }
    }

    *(text->color) = color;  // copy the struct
}

void update_text(Text* text, SDL_Event event) {
    // Text widgets are static, no updates needed for events
    (void)text;
    (void)event;
}

void free_text(Text *text) {
    if (!text) return;
    free(text->content);
    free(text->color);
}


// Registration
#define MAX_TEXTS 100

Text* text_widgets[MAX_TEXTS];
int texts_count = 0;

void register_text(Text* text) {
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

void free_all_registered_texts(void){
	for(int i = 0; i<texts_count; i++){
		if(text_widgets[i]){
			free_text(text_widgets[i]);
			text_widgets[i]=NULL;
		}
	}
	texts_count=0;
}
#endif // TEXT_H
