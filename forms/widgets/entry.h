#include <stdlib.h> // for malloc
#include <string.h> // for strlen, strcat
#include <SDL2/SDL.h> // for SDL_Event, SDLK_*, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText usage
#include <math.h>   // For roundf in scaling

typedef struct {
    Parent* parent;            // Pointer to the parent window or container
    int x, y;                  // Position of the entry (logical)
    int w, h;                  // Width and height of the entry (logical)
    char* place_holder;        // Placeholder text
    int max_length;            // Maximum text length
    char* text;                // Input text
    int is_active;             // Is the entry active?
    int cursor_pos;            // Cursor position (character index)
    int selection_start;       // Selection anchor (-1 if no selection)
    int visible_text_start;    // Index of first visible character
} Entry;

void register_widget_entry(Entry* entry);

Entry* new_entry_(Parent* parent, int x, int y, int w, int max_length) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    int logical_font_size = current_theme->default_font_size;
    int logical_padding = current_theme->padding;

    Entry* new_entry = (Entry*)malloc(sizeof(Entry));
    if (!new_entry) {
        printf("Failed to allocate memory for entry\n");
        return NULL;
    }

    new_entry->parent = parent;
    new_entry->place_holder = strdup(" "); // Default placeholder
    if (!new_entry->place_holder) {
        printf("Failed to allocate memory for placeholder\n");
        free(new_entry);
        return NULL;
    }
    new_entry->x = x;
    new_entry->y = y;
    new_entry->w = w;
    new_entry->h = logical_font_size + 2 * logical_padding;  // Proportional height, smaller default
    new_entry->max_length = max_length;
    new_entry->text = (char*)malloc(max_length + 1);
    if (!new_entry->text) {
        printf("Failed to allocate memory for entry text\n");
        free(new_entry->place_holder);
        free(new_entry);
        return NULL;
    }
    new_entry->text[0] = '\0';
    new_entry->is_active = 0;
    new_entry->cursor_pos = 0;
    new_entry->selection_start = -1;
    new_entry->visible_text_start = 0;

    // Register widget
    register_widget_entry(new_entry);
    return new_entry;
}

void render_entry(Entry* entry) {
    if (!entry || !entry->parent || !entry->parent->base.sdl_renderer || !entry->parent->is_open) {
        printf("Invalid entry, renderer, or parent is not open\n");
        return;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    float dpi = entry->parent->base.dpi_scale;
    int sx = (int)roundf((entry->x + entry->parent->x) * dpi);
    int sy = (int)roundf((entry->y + entry->parent->y) * dpi);
    int sw = (int)roundf(entry->w * dpi);
    int sh = (int)roundf(entry->h * dpi);
    int border_width = (int)roundf(2 * dpi);
    int padding = (int)roundf(current_theme->padding * dpi);
    int cursor_width = (int)roundf(2 * dpi);
    int font_size = (int)roundf(current_theme->default_font_size * dpi);
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";

    TTF_Font* font = TTF_OpenFont(font_file, font_size);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    int font_height = TTF_FontHeight(font);

    Color outline_color = current_theme->accent;  // Outline
    Color bg_color = current_theme->bg_secondary; // Background
    Color cursor_color = current_theme->accent;   // Cursor
    Color highlight_color = current_theme->accent_hovered; // Selection highlight

    // Draw outline rect
    draw_rect_(&entry->parent->base, sx, sy, sw, sh, outline_color);
    // Draw entry rect (background)
    draw_rect_(&entry->parent->base, sx + border_width, sy + border_width, sw - 2 * border_width, sh - 2 * border_width, bg_color);

    // Determine text to display
    char* display_text = (entry->is_active || entry->text[0] != '\0') ? entry->text + entry->visible_text_start : entry->place_holder;
    Color text_color = (display_text == entry->place_holder) ? current_theme->text_secondary : current_theme->text_primary;
    int text_x = sx + padding;
    int text_y = sy + (sh - font_height) / 2;

    // Clip rendering to entry rectangle
    SDL_Rect clip_rect = {sx + border_width, sy + border_width, sw - 2 * border_width, sh - 2 * border_width};
    SDL_RenderSetClipRect(entry->parent->base.sdl_renderer, &clip_rect);

    // If there's a selection, draw highlight
    if (entry->selection_start != -1 && entry->is_active) {
        int sel_start = (entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos) - entry->visible_text_start;
        int sel_end = (entry->selection_start < entry->cursor_pos ? entry->cursor_pos : entry->selection_start) - entry->visible_text_start;

        if (sel_start < 0) sel_start = 0;
        if (sel_end > strlen(display_text)) sel_end = strlen(display_text);

        if (sel_start < sel_end) {
            // Calculate highlight start width
            char temp_start[entry->max_length + 1];
            strncpy(temp_start, display_text, sel_start);
            temp_start[sel_start] = '\0';
            int highlight_offset = 0;
            TTF_SizeText(font, temp_start, &highlight_offset, NULL);
            int highlight_x = text_x + highlight_offset;

            // Highlight width
            strncpy(temp_start, display_text + sel_start, sel_end - sel_start);
            temp_start[sel_end - sel_start] = '\0';
            int highlight_w = 0;
            TTF_SizeText(font, temp_start, &highlight_w, NULL);

            // Draw highlight
            draw_rect_(&entry->parent->base, highlight_x, text_y, highlight_w, font_height, highlight_color);
        }
    }

    // Render visible text
    draw_text_from_font_(&entry->parent->base, font, display_text, text_x, text_y, text_color, ALIGN_LEFT);

    // Render cursor if active and no selection (or at cursor_pos end)
    if (entry->is_active && entry->selection_start == -1) {
        int cursor_offset = 0;
        if (entry->text[0] != '\0') {
            char temp[entry->max_length + 1];
            strncpy(temp, display_text, entry->cursor_pos - entry->visible_text_start);
            temp[entry->cursor_pos - entry->visible_text_start] = '\0';
            TTF_SizeText(font, temp, &cursor_offset, NULL);
        }
        int cursor_x = text_x + cursor_offset;
        draw_rect_(&entry->parent->base, cursor_x, text_y, cursor_width, font_height, cursor_color);
    }

    SDL_RenderSetClipRect(entry->parent->base.sdl_renderer, NULL);
    TTF_CloseFont(font);
}


void update_visible_text(Entry* entry) {
    if (!entry || !entry->parent) {
        printf("Invalid entry or parent\n");
        return;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    int logical_font_size = current_theme->default_font_size;
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
    int logical_padding = current_theme->padding;

    TTF_Font* font = TTF_OpenFont(font_file, logical_font_size);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    // Calculate max characters that fit in entry width (logical)
    int max_visible_width = entry->w - 2 * logical_padding; // Logical padding
    int text_width = 0;
    int max_visible_chars = 0;
    for (int i = 0; i < strlen(entry->text); i++) {
        char ch[2] = {entry->text[i], '\0'};
        int char_width;
        TTF_SizeText(font, ch, &char_width, NULL);
        if (text_width + char_width <= max_visible_width) {
            text_width += char_width;
            max_visible_chars++;
        } else {
            break;
        }
    }

    // Adjust for cursor
    int cursor_pixel_x = 0;
    if (entry->cursor_pos > 0) {
        char temp[entry->max_length + 1];
        strncpy(temp, entry->text, entry->cursor_pos);
        temp[entry->cursor_pos] = '\0';
        TTF_SizeText(font, temp, &cursor_pixel_x, NULL);
    }

    if (cursor_pixel_x > max_visible_width) {
        while (cursor_pixel_x > max_visible_width && entry->visible_text_start < strlen(entry->text)) {
            char ch[2] = {entry->text[entry->visible_text_start], '\0'};
            int char_width;
            TTF_SizeText(font, ch, &char_width, NULL);
            cursor_pixel_x -= char_width;
            entry->visible_text_start++;
        }
    } else if (entry->cursor_pos < entry->visible_text_start) {
        entry->visible_text_start = entry->cursor_pos;
    }

    // Additional: If selection, try to show as much as possible (but prioritize cursor)
    if (entry->selection_start != -1) {
        int sel_end = entry->selection_start > entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
        if (sel_end < entry->visible_text_start) {
            entry->visible_text_start = sel_end > max_visible_chars ? sel_end - max_visible_chars : 0;
        }
    }

    // Ensure bounds
    if (entry->visible_text_start > strlen(entry->text) - max_visible_chars) {
        entry->visible_text_start = strlen(entry->text) > max_visible_chars ? strlen(entry->text) - max_visible_chars : 0;
    }

    TTF_CloseFont(font);
}
void update_entry(Entry* entry, SDL_Event event) {
    if (!entry || !entry->parent || !entry->parent->is_open) {
        printf("Invalid entry, parent, or parent is not open\n");
        return;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    Uint16 mod = SDL_GetModState();  // Use current mod state for all

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Calculate absolute position relative to parent
        int abs_x = entry->x + entry->parent->x;
        int abs_y = entry->y + entry->parent->y;

        int mouseX = event.button.x;
        int mouseY = event.button.y;
        // Check if click is inside the rectangle
        if (mouseX >= abs_x && mouseX <= abs_x + entry->w &&
            mouseY >= abs_y && mouseY <= abs_y + entry->h) {
            entry->is_active = 1;
            printf("Entry clicked! Active\n");
            entry->selection_start = -1;

            // Calculate cursor_pos from click position (logical)
            int logical_font_size = current_theme->default_font_size;
            char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
            int logical_padding = current_theme->padding;

            TTF_Font* font = TTF_OpenFont(font_file, logical_font_size);
            if (font) {
                int click_offset = mouseX - (abs_x + logical_padding);
                int cum_width = 0;
                entry->cursor_pos = 0;
                for (int i = 0; i < strlen(entry->text); i++) {
                    char ch[2] = {entry->text[i], '\0'};
                    int char_w;
                    TTF_SizeText(font, ch, &char_w, NULL);
                    if (cum_width + char_w / 2 > click_offset) { // Closest by midpoint
                        break;
                    }
                    cum_width += char_w;
                    entry->cursor_pos = i + 1;
                }
                TTF_CloseFont(font);
            }
            update_visible_text(entry);
        } else {
            entry->is_active = 0;
            printf("Clicked outside entry! Inactive\n");
            entry->selection_start = -1;
        }
    } else if (event.type == SDL_TEXTINPUT && entry->is_active) {
        // If selection, delete it first
        if (entry->selection_start != -1) {
            int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
            int sel_end = entry->selection_start < entry->cursor_pos ? entry->cursor_pos : entry->selection_start;
            memmove(entry->text + sel_start, entry->text + sel_end, strlen(entry->text) - sel_end + 1);
            entry->cursor_pos = sel_start;
            entry->selection_start = -1;
        }
        // Then insert text
        int len = strlen(entry->text);
        int input_len = strlen(event.text.text);
        if (len + input_len < entry->max_length) {
            memmove(entry->text + entry->cursor_pos + input_len, entry->text + entry->cursor_pos, len - entry->cursor_pos + 1);
            strncpy(entry->text + entry->cursor_pos, event.text.text, input_len);
            entry->cursor_pos += input_len;
            update_visible_text(entry);
        }
    } else if (event.type == SDL_KEYDOWN && entry->is_active) {
        if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (entry->selection_start != -1) {
                // Delete selection
                int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                int sel_end = entry->selection_start < entry->cursor_pos ? entry->cursor_pos : entry->selection_start;
                memmove(entry->text + sel_start, entry->text + sel_end, strlen(entry->text) - sel_end + 1);
                entry->cursor_pos = sel_start;
                entry->selection_start = -1;
            } else if (entry->cursor_pos > 0) {
                memmove(entry->text + entry->cursor_pos - 1, entry->text + entry->cursor_pos, strlen(entry->text) - entry->cursor_pos + 1);
                entry->cursor_pos--;
            }
            update_visible_text(entry);
        } else if (event.key.keysym.sym == SDLK_LEFT) {
            if (entry->cursor_pos > 0) {
                if (mod & KMOD_SHIFT) {
                    if (entry->selection_start == -1) entry->selection_start = entry->cursor_pos;
                    entry->cursor_pos--;
                } else {
                    entry->cursor_pos--;
                    entry->selection_start = -1;  // Clear selection
                }
                update_visible_text(entry);
            }
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            if (entry->cursor_pos < strlen(entry->text)) {
                if (mod & KMOD_SHIFT) {
                    if (entry->selection_start == -1) entry->selection_start = entry->cursor_pos;
                    entry->cursor_pos++;
                } else {
                    entry->cursor_pos++;
                    entry->selection_start = -1;  // Clear selection
                }
                update_visible_text(entry);
            }
        } else if (event.key.keysym.sym == SDLK_RETURN) {
            entry->is_active = 0;
            entry->selection_start = -1;
        } else if (event.key.keysym.sym == SDLK_a && (mod & KMOD_CTRL)) {
            // Ctrl+A: Select all
            if (strlen(entry->text) > 0) {
                entry->selection_start = 0;
                entry->cursor_pos = strlen(entry->text);
            }
        } else if (event.key.keysym.sym == SDLK_c && (mod & KMOD_CTRL)) {
            // Ctrl+C: Copy selection to clipboard
            if (entry->selection_start != -1) {
                int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                int sel_len = abs(entry->cursor_pos - entry->selection_start);
                char* sel_text = malloc(sel_len + 1);
                strncpy(sel_text, entry->text + sel_start, sel_len);
                sel_text[sel_len] = '\0';
                SDL_SetClipboardText(sel_text);
                free(sel_text);
            }
        } else if (event.key.keysym.sym == SDLK_x && (mod & KMOD_CTRL)) {
            // Ctrl+X: Cut (copy then delete selection)
            if (entry->selection_start != -1) {
                int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                int sel_len = abs(entry->cursor_pos - entry->selection_start);
                char* sel_text = malloc(sel_len + 1);
                strncpy(sel_text, entry->text + sel_start, sel_len);
                sel_text[sel_len] = '\0';
                SDL_SetClipboardText(sel_text);
                free(sel_text);
                // Delete selection
                memmove(entry->text + sel_start, entry->text + sel_start + sel_len, strlen(entry->text) - (sel_start + sel_len) + 1);
                entry->cursor_pos = sel_start;
                entry->selection_start = -1;
                update_visible_text(entry);
            }
        } else if (event.key.keysym.sym == SDLK_v && (mod & KMOD_CTRL)) {
            // Ctrl+V: Paste from clipboard
            if (SDL_HasClipboardText()) {
                char* paste_text = SDL_GetClipboardText();
                if (paste_text) {
                    int paste_len = strlen(paste_text);
                    int len = strlen(entry->text);
                    // Delete selection if any
                    if (entry->selection_start != -1) {
                        int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                        int sel_len = abs(entry->cursor_pos - entry->selection_start);
                        memmove(entry->text + sel_start, entry->text + sel_start + sel_len, len - (sel_start + sel_len) + 1);
                        entry->cursor_pos = sel_start;
                        entry->selection_start = -1;
                        len -= sel_len;  // Update len after delete
                    }
                    // Insert paste
                    if (len + paste_len < entry->max_length) {
                        memmove(entry->text + entry->cursor_pos + paste_len, entry->text + entry->cursor_pos, len - entry->cursor_pos + 1);
                        strncpy(entry->text + entry->cursor_pos, paste_text, paste_len);
                        entry->cursor_pos += paste_len;
                        update_visible_text(entry);
                    }
                    SDL_free(paste_text);
                }
            }
        }
    }
}

void free_entry(Entry* entry) {
    if (entry) {
        free(entry->text);
        free(entry->place_holder);
        free(entry);
    }
}

// ___________________
#define MAX_ENTRYS 100

Entry* entry_widgets[MAX_ENTRYS];
int entrys_count = 0;

void register_widget_entry(Entry* entry) {
    if (entrys_count < MAX_ENTRYS) {
        entry_widgets[entrys_count] = entry; 
        entrys_count++;
    }
}

void render_all_registered_entrys(void) {
    for (int i = 0; i < entrys_count; i++) {
        if (entry_widgets[i]) {
            render_entry(entry_widgets[i]);
        }
    }
}

void update_all_registered_entrys(SDL_Event event) {
    for (int i = 0; i < entrys_count; i++) {
        if (entry_widgets[i]) {
            update_entry(entry_widgets[i], event);
        }
    }
}
