#include <stdlib.h> // for malloc
#include <string.h> // for strlen, strcat
#include <ctype.h> // for isspace
#include <SDL2/SDL.h> // for SDL_Event, SDLK_*, etc.
#include <SDL2/SDL_ttf.h> // for TTF_SizeText usage
#include <math.h>   // For roundf in scaling

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
    int x, y;                  // Position of the textbox (logical)
    int w, h;                  // Width and height of the textbox (logical, taller by default)
    char* place_holder;        // Placeholder text
    int max_length;            // Maximum text length
    char* text;                // Input text (supports \n for lines)
    int is_active;             // Is the textbox active?
    int cursor_pos;            // Cursor position (character index)
    int selection_start;       // Selection anchor (-1 if no selection)
    int visible_line_start;    // Index of first visible line
    int line_height;           // Height of each line (logical, computed from font)
    int is_mouse_selecting;    // Flag to track if mouse is being used to select text
} TextBox;

void register_widget_textbox(TextBox* textbox);

TextBox* new_textbox_(Parent* parent, int x, int y, int w, int max_length) {
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL;
    }

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    int logical_font_size = current_theme->default_font_size;
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
    int logical_padding = current_theme->padding;

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
    new_textbox->h = 10 * (logical_font_size + logical_padding / 2);  // Example: ~10 lines tall logically
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
    new_textbox->is_mouse_selecting = 0; // Initialize mouse selection flag

    // Compute line_height from font (logical)
    TTF_Font* font = TTF_OpenFont(font_file, logical_font_size);
    if (font) {
        new_textbox->line_height = TTF_FontHeight(font);
        TTF_CloseFont(font);
    } else {
        new_textbox->line_height = logical_font_size + logical_padding / 2;  // Fallback
    }

    // Register widget
    register_widget_textbox(new_textbox);
    return new_textbox;
}

// Renders the textbox widget to the screen
// Parameters:
// - textbox: The TextBox widget to render
void render_textbox(TextBox* textbox) {
    // Validate inputs to ensure the textbox, its parent, and renderer are valid
    if (!textbox || !textbox->parent || !textbox->parent->base.sdl_renderer || !textbox->parent->is_open) {
        printf("Invalid textbox, renderer, or parent is not open\n");
        return;
    }

    // Default to light theme if none is set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Get DPI scale for converting logical coordinates to physical pixels
    float dpi = textbox->parent->base.dpi_scale;

    // Calculate absolute position in logical coordinates, accounting for parent position and title bar
    int abs_x = textbox->x + textbox->parent->x;
    int abs_y = textbox->y + textbox->parent->y + textbox->parent->title_height;
    int sx = (int)roundf(abs_x * dpi);
    int sy = (int)roundf(abs_y * dpi);
    int sw = (int)roundf(textbox->w * dpi);
    int sh = (int)roundf(textbox->h * dpi);
    int border_width = (int)roundf(2 * dpi);
    int padding = (int)roundf(current_theme->padding * dpi);
    int cursor_width = (int)roundf(2 * dpi);
    int font_size = (int)roundf(current_theme->default_font_size * dpi);
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";

    // Load the font for rendering text
    TTF_Font* font = TTF_OpenFont(font_file, font_size);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    // Get the font height for vertical alignment
    int font_height = TTF_FontHeight(font);

    // Define colors from the theme for consistent styling
    Color outline_color = current_theme->accent;  // Border color
    Color bg_color = current_theme->bg_secondary; // Background color
    Color cursor_color = current_theme->accent;   // Cursor color
    Color highlight_color = current_theme->accent_hovered; // Selection highlight color

    // Draw outline rect
    draw_rect_(&textbox->parent->base, sx, sy, sw, sh, outline_color);
    // Draw textbox rect (background)
    draw_rect_(&textbox->parent->base, sx + border_width, sy + border_width, 
               sw - 2 * border_width, sh - 2 * border_width, bg_color);

    // Determine text to display
    char* display_text = (textbox->is_active || textbox->text[0] != '\0') ? textbox->text : textbox->place_holder;
    Color text_color = (display_text == textbox->place_holder) ? current_theme->text_secondary : current_theme->text_primary;
    int text_x = sx + padding;
    int text_y = sy + padding;
    int max_text_width = sw - 2 * padding;

    // Compute visual lines (using scaled font and width, but since scale-invariant, lines same as logical)
    int num_lines = 0;
    Line* lines = compute_visual_lines(display_text, max_text_width, font, &num_lines);

    // Calculate number of visible lines
    int visible_lines_count = (sh - 2 * padding) / font_height;

    // Clip rendering to textbox rectangle
    SDL_Rect clip_rect = {sx + border_width, sy + border_width, sw - 2 * border_width, sh - 2 * border_width};
    SDL_RenderSetClipRect(textbox->parent->base.sdl_renderer, &clip_rect);

    // Determine selection range
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

        int draw_y = text_y + (i - textbox->visible_line_start) * font_height;

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

                        // Draw highlight
                        draw_rect_(&textbox->parent->base, highlight_x, draw_y, highlight_w, font_height, highlight_color);
                    }
                }
            }
        }

        // Render line text
        draw_text_from_font_(&textbox->parent->base, font, line_text, text_x, draw_y, text_color, ALIGN_LEFT);

        free(line_text);
    }

    // Render cursor if active
    if (textbox->is_active) {
        for (int i = 0; i < num_lines; i++) {
            Line l = lines[i];
            if (textbox->cursor_pos >= l.start && textbox->cursor_pos <= l.start + l.len) {
                if (i < textbox->visible_line_start || i >= textbox->visible_line_start + visible_lines_count) {
                    break; // Not visible
                }
                int rel_line = i - textbox->visible_line_start;
                int draw_y = text_y + rel_line * font_height;
                int offset_chars = textbox->cursor_pos - l.start;

                char* temp = (char*)malloc(offset_chars + 1);
                if (temp) {
                    strncpy(temp, display_text + l.start, offset_chars);
                    temp[offset_chars] = '\0';
                    int cursor_offset = 0;
                    TTF_SizeText(font, temp, &cursor_offset, NULL);
                    free(temp);

                    int cursor_x = text_x + cursor_offset;
                    draw_rect_(&textbox->parent->base, cursor_x, draw_y, cursor_width, font_height, cursor_color);
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

    int max_text_width = textbox->w - 2 * logical_padding;
    int num_lines = 0;
    Line* lines = compute_visual_lines(textbox->text, max_text_width, font, &num_lines);

    int visible_lines = (textbox->h - 2 * logical_padding) / textbox->line_height;

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

// Updates the textbox widget based on SDL events (mouse, keyboard, text input)
// Parameters:
// - textbox: The TextBox widget to update
// - event: The SDL event to process
void update_textbox(TextBox* textbox, SDL_Event event) {
    // Validate inputs to ensure the textbox, its parent, and parent state are valid
    if (!textbox || !textbox->parent || !textbox->parent->is_open) {
        printf("Invalid textbox, parent, or parent is not open\n");
        return;
    }

    // Default to light theme if none is set for consistent styling
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Get DPI scale for converting logical to physical coordinates
    float dpi = textbox->parent->base.dpi_scale;
    Uint16 mod = SDL_GetModState(); // Current keyboard modifier state (e.g., Shift, Ctrl)

    // Calculate absolute position in logical coordinates, accounting for parent and title bar
    int abs_x = textbox->x + textbox->parent->x;
    int abs_y = textbox->y + textbox->parent->y + textbox->parent->title_height;
    // Scale to physical for hit test
    int s_abs_x = (int)roundf(abs_x * dpi);
    int s_abs_y = (int)roundf(abs_y * dpi);
    int s_w = (int)roundf(textbox->w * dpi);
    int s_h = (int)roundf(textbox->h * dpi);

    // Load font for cursor position calculations
    int logical_font_size = current_theme->default_font_size;
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
    int logical_padding = current_theme->padding;
    TTF_Font* font = TTF_OpenFont(font_file, logical_font_size);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    // Handle mouse button down event (left click)
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        // Check if click is inside the textbox (physical coords)
        if (mouseX >= s_abs_x && mouseX <= s_abs_x + s_w &&
            mouseY >= s_abs_y && mouseY <= s_abs_y + s_h) {
            textbox->is_active = 1; // Activate the textbox
            textbox->is_mouse_selecting = 1; // Enable mouse-based selection
            textbox->selection_start = -1; // Clear existing selection
            printf("Textbox clicked! Active\n");

            // Calculate cursor_pos from click position (logical)
            int logical_mouse_x = (int)roundf(mouseX / dpi);
            int logical_mouse_y = (int)roundf(mouseY / dpi);
            int click_y = logical_mouse_y - (abs_y + logical_padding);
            int clicked_line = textbox->visible_line_start + click_y / textbox->line_height;
            int max_text_width = textbox->w - 2 * logical_padding;
            int num_lines = 0;
            Line* lines = compute_visual_lines(textbox->text, max_text_width, font, &num_lines);
            if (clicked_line < num_lines) {
                Line l = lines[clicked_line];
                int click_x = logical_mouse_x - (abs_x + logical_padding);
                int cum_width = 0;
                textbox->cursor_pos = l.start;
                for (int j = 0; j < l.len; j++) {
                    char ch[2] = {textbox->text[l.start + j], '\0'};
                    int char_w;
                    TTF_SizeText(font, ch, &char_w, NULL);
                    if (cum_width + char_w / 2 > click_x) { // Closest character by midpoint
                        break;
                    }
                    cum_width += char_w;
                    textbox->cursor_pos = l.start + j + 1;
                }
            } else {
                textbox->cursor_pos = strlen(textbox->text); // Click beyond text sets cursor to end
            }
            free(lines);
            update_visible_lines(textbox);
        } else {
            textbox->is_active = 0; // Deactivate if clicked outside
            textbox->is_mouse_selecting = 0; // Stop mouse selection
            textbox->selection_start = -1; // Clear selection
            printf("Clicked outside textbox! Inactive\n");
        }
    } 
    // Handle mouse button up event (left click)
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        textbox->is_mouse_selecting = 0; // End mouse-based selection
    } 
    // Handle mouse motion for drag selection
    else if (event.type == SDL_MOUSEMOTION && textbox->is_mouse_selecting && (event.motion.state & SDL_BUTTON_LMASK)) {
        int mouseX = event.motion.x;
        int mouseY = event.motion.y;
        // Update selection if mouse is within textbox bounds (optional: can remove bounds check)
        if (mouseX >= s_abs_x && mouseX <= s_abs_x + s_w &&
            mouseY >= s_abs_y && mouseY <= s_abs_y + s_h) {
            // Set selection anchor on first drag motion if not already set
            if (textbox->selection_start == -1) {
                textbox->selection_start = textbox->cursor_pos;
            }
            // Calculate new cursor position from mouse position
            int logical_mouse_x = (int)roundf(mouseX / dpi);
            int logical_mouse_y = (int)roundf(mouseY / dpi);
            int click_y = logical_mouse_y - (abs_y + logical_padding);
            int clicked_line = textbox->visible_line_start + click_y / textbox->line_height;
            int max_text_width = textbox->w - 2 * logical_padding;
            int num_lines = 0;
            Line* lines = compute_visual_lines(textbox->text, max_text_width, font, &num_lines);
            if (clicked_line < num_lines) {
                Line l = lines[clicked_line];
                int click_x = logical_mouse_x - (abs_x + logical_padding);
                int cum_width = 0;
                textbox->cursor_pos = l.start;
                for (int j = 0; j < l.len; j++) {
                    char ch[2] = {textbox->text[l.start + j], '\0'};
                    int char_w;
                    TTF_SizeText(font, ch, &char_w, NULL);
                    if (cum_width + char_w / 2 > click_x) {
                        break;
                    }
                    cum_width += char_w;
                    textbox->cursor_pos = l.start + j + 1;
                }
            } else {
                textbox->cursor_pos = strlen(textbox->text);
            }
            free(lines);
            update_visible_lines(textbox);
        }
    } 
    // Handle text input when textbox is active
    else if (event.type == SDL_TEXTINPUT && textbox->is_active) {
        // If there's a selection, delete it before inserting new text
        if (textbox->selection_start != -1) {
            int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
            int sel_end = textbox->selection_start < textbox->cursor_pos ? textbox->cursor_pos : textbox->selection_start;
            memmove(textbox->text + sel_start, textbox->text + sel_end, strlen(textbox->text) - sel_end + 1);
            textbox->cursor_pos = sel_start;
            textbox->selection_start = -1;
        }
        // Insert new text at cursor position
        int len = strlen(textbox->text);
        int input_len = strlen(event.text.text);
        if (len + input_len < textbox->max_length) {
            memmove(textbox->text + textbox->cursor_pos + input_len, 
                    textbox->text + textbox->cursor_pos, len - textbox->cursor_pos + 1);
            strncpy(textbox->text + textbox->cursor_pos, event.text.text, input_len);
            textbox->cursor_pos += input_len;
            update_visible_lines(textbox);
        }
    } 
    // Handle key presses when textbox is active
    else if (event.type == SDL_KEYDOWN && textbox->is_active) {
        if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (textbox->selection_start != -1) {
                // Delete selected text
                int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
                int sel_end = textbox->selection_start < textbox->cursor_pos ? textbox->cursor_pos : textbox->selection_start;
                memmove(textbox->text + sel_start, textbox->text + sel_end, strlen(textbox->text) - sel_end + 1);
                textbox->cursor_pos = sel_start;
                textbox->selection_start = -1;
            } else if (textbox->cursor_pos > 0) {
                // Delete character before cursor
                memmove(textbox->text + textbox->cursor_pos - 1, 
                        textbox->text + textbox->cursor_pos, strlen(textbox->text) - textbox->cursor_pos + 1);
                textbox->cursor_pos--;
            }
            update_visible_lines(textbox);
        } else if (event.key.keysym.sym == SDLK_DELETE) {
            if (textbox->selection_start != -1) {
                // Delete selected text (same as backspace)
                int sel_start = textbox->selection_start < textbox->cursor_pos ? textbox->selection_start : textbox->cursor_pos;
                int sel_end = textbox->selection_start < textbox->cursor_pos ? textbox->cursor_pos : textbox->selection_start;
                memmove(textbox->text + sel_start, textbox->text + sel_end, strlen(textbox->text) - sel_end + 1);
                textbox->cursor_pos = sel_start;
                textbox->selection_start = -1;
            } else if (textbox->cursor_pos < strlen(textbox->text)) {
                // Delete character after cursor
                memmove(textbox->text + textbox->cursor_pos, 
                        textbox->text + textbox->cursor_pos + 1, strlen(textbox->text) - textbox->cursor_pos);
            }
            update_visible_lines(textbox);
        } else if (event.key.keysym.sym == SDLK_LEFT) {
            if (textbox->cursor_pos > 0) {
                if (mod & KMOD_SHIFT) {
                    // Extend selection with Shift+Left
                    if (textbox->selection_start == -1) textbox->selection_start = textbox->cursor_pos;
                    textbox->cursor_pos--;
                } else {
                    // Move cursor left, clear selection
                    textbox->cursor_pos--;
                    textbox->selection_start = -1;
                }
                update_visible_lines(textbox);
            }
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            if (textbox->cursor_pos < strlen(textbox->text)) {
                if (mod & KMOD_SHIFT) {
                    // Extend selection with Shift+Right
                    if (textbox->selection_start == -1) textbox->selection_start = textbox->cursor_pos;
                    textbox->cursor_pos++;
                } else {
                    // Move cursor right, clear selection
                    textbox->cursor_pos++;
                    textbox->selection_start = -1;
                }
                update_visible_lines(textbox);
            }
        } else if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
            int max_text_width = textbox->w - 2 * logical_padding;
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
                // Compute preferred width (logical)
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
                    } else {
                        textbox->cursor_pos = strlen(textbox->text);
                    }
                }
            }

            free(lines);

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
            // Ctrl+A: Select all text
            if (strlen(textbox->text) > 0) {
                textbox->selection_start = 0;
                textbox->cursor_pos = strlen(textbox->text);
            }
        } else if (event.key.keysym.sym == SDLK_c && (mod & KMOD_CTRL)) {
            // Ctrl+C: Copy selected text to clipboard
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
            // Ctrl+X: Cut selected text (copy to clipboard and delete)
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
            // Ctrl+V: Paste text from clipboard
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

    TTF_CloseFont(font); // Free the font resource
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
