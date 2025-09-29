#include <stdlib.h> // for malloc
#include <string.h> // for strlen, strcat
#include <ctype.h> // for isspace
#include <SDL2/SDL.h> // for SDL_Event, SDLK_*, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText usage

typedef struct {
    int start;
    int len;
} Line;

Line* compute_visual_lines(const char* text, int max_width, TTF_Font* font, int* num_lines) {
    *num_lines = 0;
    if (!text) return NULL;

    int text_len = strlen(text);
    int max_lines = text_len * 2 + 2; // Safe upper bound for wraps and newlines
    Line* lines = (Line*)malloc(sizeof(Line) * max_lines);
    if (!lines) return NULL;

    int line_start = 0;
    for (int pos = 0; pos <= text_len; pos++) {
        if (pos == text_len || text[pos] == '\n') {
            int seg_start = line_start;
            int seg_len = pos - line_start;
            int seg_pos = 0;
            while (seg_pos < seg_len) {
                int line_start_local = seg_pos;
                int last_space = -1;
                int current_width = 0;
                while (seg_pos < seg_len) {
                    char ch[2] = {text[seg_start + seg_pos], '\0'};
                    int char_w;
                    TTF_SizeText(font, ch, &char_w, NULL);
                    if (current_width + char_w > max_width) {
                        if (current_width == 0) {
                            current_width += char_w;
                            if (isspace(ch[0])) last_space = seg_pos;
                            seg_pos++;
                            continue;
                        }
                        int add_len;
                        if (last_space != -1) {
                            add_len = last_space - line_start_local;
                            seg_pos = last_space + 1;
                        } else {
                            add_len = seg_pos - line_start_local;
                        }
                        if (add_len > 0) {
                            lines[*num_lines].start = seg_start + line_start_local;
                            lines[*num_lines].len = add_len;
                            (*num_lines)++;
                        }
                        line_start_local = seg_pos;
                        current_width = 0;
                        last_space = -1;
                        continue;
                    }
                    current_width += char_w;
                    if (isspace(text[seg_start + seg_pos])) last_space = seg_pos;
                    seg_pos++;
                }
                // Add last part of wrap
                int add_len = seg_pos - line_start_local;
                if (add_len > 0 || (add_len == 0 && seg_len == 0)) {
                    lines[*num_lines].start = seg_start + line_start_local;
                    lines[*num_lines].len = add_len;
                    (*num_lines)++;
                }
            }
            // If no wraps but empty segment
            if (seg_len == 0) {
                lines[*num_lines].start = seg_start;
                lines[*num_lines].len = 0;
                (*num_lines)++;
            }
            line_start = pos + 1;
        }
    }
    return lines;
}

typedef struct {
    Parent* parent;            // Pointer to the parent window or container
    int x, y;                  // Position of the textbox
    int w, h;                  // Width and height of the textbox (taller by default)
    char* place_holder;        // Placeholder text
    int max_length;            // Maximum text length
    char* text;                // Input text (supports \n for lines)
    int is_active;             // Is the textbox active?
    int cursor_pos;            // Cursor position (character index)
    int selection_start;       // Selection anchor (-1 if no selection)
    int visible_line_start;    // Index of first visible line
    int line_height;           // Height of each line (computed from font)
} TextBox;

void register_widget_textbox(TextBox* textbox);

TextBox* new_textbox_(Parent* parent, int x, int y, int w, int max_length) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    TextBox* new_textbox = (TextBox*)malloc(sizeof(TextBox));
    if (!new_textbox) {
        printf("Failed to allocate memory for textbox\n");
        return NULL;
    }

    new_textbox->parent = parent;
    new_textbox->place_holder = strdup(" "); // Default placeholder
    if (!new_textbox->place_holder) {
        printf("Failed to allocate memory for placeholder\n");
        free(new_textbox);
        return NULL;
    }
    new_textbox->x = x;
    new_textbox->y = y;
    new_textbox->w = w;
    new_textbox->h = 200;  // Taller by default for multi-line
    new_textbox->max_length = max_length;
    new_textbox->text = (char*)malloc(max_length + 1);
    if (!new_textbox->text) {
        printf("Failed to allocate memory for textbox text\n");
        free(new_textbox->place_holder);
        free(new_textbox);
        return NULL;
    }
    new_textbox->text[0] = '\0';
    new_textbox->is_active = 0;
    new_textbox->cursor_pos = 0;
    new_textbox->selection_start = -1;
    new_textbox->visible_line_start = 0;

    // Compute line_height from font
    TTF_Font* font = TTF_OpenFont(FONT_FILE, 30);
    if (font) {
        TTF_SizeText(font, "A", NULL, &new_textbox->line_height);
        TTF_CloseFont(font);
    } else {
        new_textbox->line_height = 30;  // Fallback
    }

    // Register widget
    register_widget_textbox(new_textbox);
    return new_textbox;
}

void render_textbox(TextBox* textbox) {
    if (!textbox || !textbox->parent || !textbox->parent->base.sdl_renderer || !textbox->parent->is_open) {
        printf("Invalid textbox, renderer, or parent is not open\n");
        return;
    }

    // Calculate absolute position relative to parent
    int abs_x = textbox->x + textbox->parent->x;
    int abs_y = textbox->y + textbox->parent->y;

    // Draw outline rect
    draw_rect_(&textbox->parent->base, abs_x, abs_y, textbox->w, textbox->h, COLOR_BLACK);
    // Draw textbox rect
    draw_rect_(&textbox->parent->base, abs_x + 2, abs_y + 2, textbox->w - 4, textbox->h - 4, COLOR_WHITE);

    TTF_Font* font = TTF_OpenFont(FONT_FILE, 30);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    // Determine text to display
    char* display_text = (textbox->is_active || textbox->text[0] != '\0') ? textbox->text : textbox->place_holder;

    int padding = 10;
    int text_x = abs_x + padding;
    int text_y = abs_y + padding;
    int max_text_width = textbox->w - 2 * padding;

    // Compute visual lines
    int num_lines = 0;
    Line* lines = compute_visual_lines(display_text, max_text_width, font, &num_lines);

    int visible_lines_count = (textbox->h - 2 * padding) / textbox->line_height;

    // Clip rendering to textbox rectangle
    SDL_Rect clip_rect = {abs_x + 2, abs_y + 2, textbox->w - 4, textbox->h - 4};
    SDL_RenderSetClipRect(textbox->parent->base.sdl_renderer, &clip_rect);

    int sel_min = -1;
    int sel_max = -1;
    if (textbox->selection_start != -1 && textbox->is_active) {
        sel_min = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
        sel_max = textbox->selection_start < textbox->cursor_pos ? textbox->cursor_pos : textbox->selection_start;
    }

    // Render visible lines
    for (int i = textbox->visible_line_start; i < num_lines && i < textbox->visible_line_start + visible_lines_count; i++) {
        Line l = lines[i];
        char* line_text = (char*)malloc(l.len + 1);
        if (line_text) {
            strncpy(line_text, display_text + l.start, l.len);
            line_text[l.len] = '\0';
        } else {
            continue;
        }

        int draw_y = text_y + (i - textbox->visible_line_start) * textbox->line_height;

        // If there's a selection, draw highlight for overlapping part
        if (sel_min != -1) {
            int line_start_char = l.start;
            int line_end_char = l.start + l.len;
            if (sel_max > line_start_char && sel_min < line_end_char) {
                int overlap_start = sel_min > line_start_char ? sel_min : line_start_char;
                int overlap_end = sel_max < line_end_char ? sel_max : line_end_char;

                // Prefix width to overlap_start
                char* temp = (char*)malloc((overlap_start - line_start_char) + 1);
                if (temp) {
                    strncpy(temp, display_text + line_start_char, overlap_start - line_start_char);
                    temp[overlap_start - line_start_char] = '\0';
                    int highlight_x_offset = 0;
                    TTF_SizeText(font, temp, &highlight_x_offset, NULL);
                    free(temp);

                    int highlight_x = text_x + highlight_x_offset;

                    // Overlap width
                    temp = (char*)malloc((overlap_end - overlap_start) + 1);
                    if (temp) {
                        strncpy(temp, display_text + overlap_start, overlap_end - overlap_start);
                        temp[overlap_end - overlap_start] = '\0';
                        int highlight_w = 0;
                        TTF_SizeText(font, temp, &highlight_w, NULL);
                        free(temp);

                        // Draw blue highlight
                        Color highlight_color = {135, 206, 235, 255};  // Light blue
                        draw_rect_(&textbox->parent->base, highlight_x, draw_y, highlight_w, textbox->line_height, highlight_color);
                    }
                }
            }
        }

        // Render line text
        draw_text_from_font_(&textbox->parent->base, font, line_text, text_x, draw_y, COLOR_BLACK, ALIGN_LEFT);

        free(line_text);
    }

    // Render cursor if active and no selection
    if (textbox->is_active && textbox->selection_start == -1) {
        for (int i = 0; i < num_lines; i++) {
            Line l = lines[i];
            if (textbox->cursor_pos >= l.start && textbox->cursor_pos <= l.start + l.len) {
                if (i < textbox->visible_line_start || i >= textbox->visible_line_start + visible_lines_count) {
                    break; // Not visible
                }
                int rel_line = i - textbox->visible_line_start;
                int draw_y = text_y + rel_line * textbox->line_height;
                int offset_chars = textbox->cursor_pos - l.start;

                char* temp = (char*)malloc(offset_chars + 1);
                if (temp) {
                    strncpy(temp, display_text + l.start, offset_chars);
                    temp[offset_chars] = '\0';
                    int cursor_offset = 0;
                    TTF_SizeText(font, temp, &cursor_offset, NULL);
                    free(temp);

                    int cursor_x = text_x + cursor_offset;
                    draw_rect_(&textbox->parent->base, cursor_x, draw_y, 2, textbox->line_height, COLOR_BLACK);
                }
                break;
            }
        }
    }

    free(lines);
    SDL_RenderSetClipRect(textbox->parent->base.sdl_renderer, NULL);
    TTF_CloseFont(font);
}

void update_visible_lines(TextBox* textbox) {
    if (!textbox || !textbox->parent) {
        printf("Invalid textbox or parent\n");
        return;
    }

    TTF_Font* font = TTF_OpenFont(FONT_FILE, 30);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    int padding = 10;
    int max_text_width = textbox->w - 2 * padding;
    int num_lines = 0;
    Line* lines = compute_visual_lines(textbox->text, max_text_width, font, &num_lines);

    int visible_lines = (textbox->h - 2 * padding) / textbox->line_height;

    // Find cursor's visual line
    int cursor_line = -1;
    for (int i = 0; i < num_lines; i++) {
        Line l = lines[i];
        if (textbox->cursor_pos >= l.start && textbox->cursor_pos <= l.start + l.len) {
            cursor_line = i;
            break;
        }
    }

    if (cursor_line != -1) {
        if (cursor_line < textbox->visible_line_start) {
            textbox->visible_line_start = cursor_line;
        } else if (cursor_line >= textbox->visible_line_start + visible_lines) {
            textbox->visible_line_start = cursor_line - visible_lines + 1;
        }
    }

    if (textbox->visible_line_start > num_lines - visible_lines) {
        textbox->visible_line_start = num_lines > visible_lines ? num_lines - visible_lines : 0;
    }
    if (textbox->visible_line_start < 0) textbox->visible_line_start = 0;

    free(lines);
    TTF_CloseFont(font);
}

void update_textbox(TextBox* textbox, SDL_Event event) {
    if (!textbox || !textbox->parent || !textbox->parent->is_open) {
        printf("Invalid textbox, parent, or parent is not open\n");
        return;
    }

    Uint16 mod = event.key.keysym.mod;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int abs_x = textbox->x + textbox->parent->x;
        int abs_y = textbox->y + textbox->parent->y;
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        if (mouseX >= abs_x && mouseX <= abs_x + textbox->w &&
            mouseY >= abs_y && mouseY <= abs_y + textbox->h) {
            textbox->is_active = 1;
            textbox->selection_start = -1;
            update_visible_lines(textbox);
            printf("Textbox clicked! Active\n");
        } else {
            textbox->is_active = 0;
            printf("Clicked outside textbox! Inactive\n");
        }
    } else if (event.type == SDL_TEXTINPUT && textbox->is_active) {
        if (textbox->selection_start != -1) {
            int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
            int sel_end = textbox->selection_start < textbox->cursor_pos ? textbox->cursor_pos : textbox->selection_start;
            memmove(textbox->text + sel_start, textbox->text + sel_end, strlen(textbox->text) - sel_end + 1);
            textbox->cursor_pos = sel_start;
            textbox->selection_start = -1;
        }
        int len = strlen(textbox->text);
        int input_len = strlen(event.text.text);
        if (len + input_len < textbox->max_length) {
            memmove(textbox->text + textbox->cursor_pos + input_len, textbox->text + textbox->cursor_pos, len - textbox->cursor_pos + 1);
            strncpy(textbox->text + textbox->cursor_pos, event.text.text, input_len);
            textbox->cursor_pos += input_len;
            update_visible_lines(textbox);
        }
    } else if (event.type == SDL_KEYDOWN && textbox->is_active) {
        if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (textbox->selection_start != -1) {
                int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
                int sel_end = textbox->selection_start < textbox->cursor_pos ? textbox->cursor_pos : textbox->selection_start;
                memmove(textbox->text + sel_start, textbox->text + sel_end, strlen(textbox->text) - sel_end + 1);
                textbox->cursor_pos = sel_start;
                textbox->selection_start = -1;
            } else if (textbox->cursor_pos > 0) {
                memmove(textbox->text + textbox->cursor_pos - 1, textbox->text + textbox->cursor_pos, strlen(textbox->text) - textbox->cursor_pos + 1);
                textbox->cursor_pos--;
            }
            update_visible_lines(textbox);
        } else if (event.key.keysym.sym == SDLK_LEFT) {
            if (textbox->cursor_pos > 0) {
                if (mod & KMOD_SHIFT) {
                    if (textbox->selection_start == -1) textbox->selection_start = textbox->cursor_pos;
                    textbox->cursor_pos--;
                } else {
                    textbox->cursor_pos--;
                    textbox->selection_start = -1;
                }
                update_visible_lines(textbox);
            }
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            if (textbox->cursor_pos < strlen(textbox->text)) {
                if (mod & KMOD_SHIFT) {
                    if (textbox->selection_start == -1) textbox->selection_start = textbox->cursor_pos;
                    textbox->cursor_pos++;
                } else {
                    textbox->cursor_pos++;
                    textbox->selection_start = -1;
                }
                update_visible_lines(textbox);
            }
        } else if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
            TTF_Font* font = TTF_OpenFont(FONT_FILE, 30);
            if (!font) {
                printf("Failed to load font: %s\n", TTF_GetError());
                return;
            }

            int padding = 10;
            int max_text_width = textbox->w - 2 * padding;
            int num_lines = 0;
            Line* lines = compute_visual_lines(textbox->text, max_text_width, font, &num_lines);

            // Find current line and offset
            int curr_line_idx = -1;
            int curr_offset_chars = 0;
            int old_cursor_pos = textbox->cursor_pos;
            for (int i = 0; i < num_lines; i++) {
                Line l = lines[i];
                if (textbox->cursor_pos >= l.start && textbox->cursor_pos <= l.start + l.len) {
                    curr_line_idx = i;
                    curr_offset_chars = textbox->cursor_pos - l.start;
                    break;
                }
            }

            if (curr_line_idx != -1) {
                // Compute preferred width
                char* temp = (char*)malloc(curr_offset_chars + 1);
                if (temp) {
                    strncpy(temp, textbox->text + lines[curr_line_idx].start, curr_offset_chars);
                    temp[curr_offset_chars] = '\0';
                    int preferred_width = 0;
                    TTF_SizeText(font, temp, &preferred_width, NULL);
                    free(temp);

                    int delta = event.key.keysym.sym == SDLK_DOWN ? 1 : -1;
                    int target_line_idx = curr_line_idx + delta;
                    if (target_line_idx >= 0 && target_line_idx < num_lines) {
                        Line target_l = lines[target_line_idx];

                        // Find offset in target line closest to preferred_width
                        int accum_w = 0;
                        int target_offset = 0;
                        for (int j = 0; j < target_l.len; j++) {
                            char ch[2] = {textbox->text[target_l.start + j], '\0'};
                            int char_w = 0;
                            TTF_SizeText(font, ch, &char_w, NULL);

                            if (accum_w + char_w / 2 > preferred_width) break;
                            accum_w += char_w;
                            target_offset++;
                        }

                        // If preferred beyond line length, go to end
                        if (accum_w < preferred_width) {
                            target_offset = target_l.len;
                        }

                        textbox->cursor_pos = target_l.start + target_offset;
                    } else if (target_line_idx < 0) {
                        textbox->cursor_pos = 0;
                    } else if (target_line_idx >= num_lines) {
                        textbox->cursor_pos = strlen(textbox->text);
                    }
                }
            }

            free(lines);
            TTF_CloseFont(font);

            if (mod & KMOD_SHIFT) {
                if (textbox->selection_start == -1) textbox->selection_start = old_cursor_pos;
            } else {
                textbox->selection_start = -1;
            }
            update_visible_lines(textbox);
        } else if (event.key.keysym.sym == SDLK_RETURN) {
            // Insert \n
            int len = strlen(textbox->text);
            if (len + 1 < textbox->max_length) {
                memmove(textbox->text + textbox->cursor_pos + 1, textbox->text + textbox->cursor_pos, len - textbox->cursor_pos + 1);
                textbox->text[textbox->cursor_pos] = '\n';
                textbox->cursor_pos++;
                update_visible_lines(textbox);
            }
        } else if (event.key.keysym.sym == SDLK_a && (mod & KMOD_CTRL)) {
            if (strlen(textbox->text) > 0) {
                textbox->selection_start = 0;
                textbox->cursor_pos = strlen(textbox->text);
            }
        } else if (event.key.keysym.sym == SDLK_c && (mod & KMOD_CTRL)) {
            if (textbox->selection_start != -1) {
                int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
                int sel_len = abs(textbox->cursor_pos - textbox->selection_start);
                char* sel_text = malloc(sel_len + 1);
                strncpy(sel_text, textbox->text + sel_start, sel_len);
                sel_text[sel_len] = '\0';
                SDL_SetClipboardText(sel_text);
                free(sel_text);
            }
        } else if (event.key.keysym.sym == SDLK_x && (mod & KMOD_CTRL)) {
            if (textbox->selection_start != -1) {
                int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
                int sel_len = abs(textbox->cursor_pos - textbox->selection_start);
                char* sel_text = malloc(sel_len + 1);
                strncpy(sel_text, textbox->text + sel_start, sel_len);
                sel_text[sel_len] = '\0';
                SDL_SetClipboardText(sel_text);
                free(sel_text);
                memmove(textbox->text + sel_start, textbox->text + sel_start + sel_len, strlen(textbox->text) - (sel_start + sel_len) + 1);
                textbox->cursor_pos = sel_start;
                textbox->selection_start = -1;
                update_visible_lines(textbox);
            }
        } else if (event.key.keysym.sym == SDLK_v && (mod & KMOD_CTRL)) {
            if (SDL_HasClipboardText()) {
                char* paste_text = SDL_GetClipboardText();
                if (paste_text) {
                    int paste_len = strlen(paste_text);
                    int len = strlen(textbox->text);
                    if (textbox->selection_start != -1) {
                        int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
                        int sel_len = abs(textbox->cursor_pos - textbox->selection_start);
                        memmove(textbox->text + sel_start, textbox->text + sel_start + sel_len, len - (sel_start + sel_len) + 1);
                        textbox->cursor_pos = sel_start;
                        textbox->selection_start = -1;
                        len -= sel_len;
                    }
                    if (len + paste_len < textbox->max_length) {
                        memmove(textbox->text + textbox->cursor_pos + paste_len, textbox->text + textbox->cursor_pos, len - textbox->cursor_pos + 1);
                        strncpy(textbox->text + textbox->cursor_pos, paste_text, paste_len);
                        textbox->cursor_pos += paste_len;
                        update_visible_lines(textbox);
                    }
                    SDL_free(paste_text);
                }
            }
        }
    }
}

void free_textbox(TextBox* textbox) {
    if (textbox) {
        free(textbox->text);
        free(textbox->place_holder);
        free(textbox);
    }
}

// Registration
#define MAX_TEXTBOXS 100

TextBox* textbox_widgets[MAX_TEXTBOXS];
int textboxs_count = 0;

void register_widget_textbox(TextBox* textbox) {
    if (textboxs_count < MAX_TEXTBOXS) {
        textbox_widgets[textboxs_count] = textbox;
        textboxs_count++;
    }
}

void render_all_registered_textboxs(void) {
    for (int i = 0; i < textboxs_count; i++) {
        if (textbox_widgets[i]) {
            render_textbox(textbox_widgets[i]);
        }
    }
}

void update_all_registered_textboxs(SDL_Event event) {
    for (int i = 0; i < textboxs_count; i++) {
        if (textbox_widgets[i]) {
            update_textbox(textbox_widgets[i], event);
        }
    }
}
