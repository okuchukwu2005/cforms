#include <stdlib.h> // Provides memory allocation functions like malloc and free
#include <string.h> // Includes string manipulation functions like strlen, strcat, strdup
#include <SDL2/SDL.h> // SDL library for handling graphics, events (e.g., SDL_Event, SDLK_* for key codes)
#include <SDL2/SDL_ttf.h> // SDL_ttf for rendering text (e.g., TTF_SizeText for text measurements)
#include <math.h>   // For mathematical functions like roundf used in DPI scaling

// Defines a struct for a text entry widget, representing an input field in a GUI
typedef struct {
    Parent* parent;            // Pointer to the parent window/container holding this entry
    int x, y;                  // Logical (unscaled) position of the entry relative to parent
    int w, h;                  // Logical width and height of the entry
    char* place_holder;        // Placeholder text shown when the entry is empty
    int max_length;            // Maximum number of characters allowed in the input
    char* text;                // Buffer storing the user-entered text
    int is_active;             // Flag indicating if the entry is currently focused (1 = active, 0 = inactive)
    int cursor_pos;            // Index of the cursor's position in the text (character index)
    int selection_start;       // Starting index of text selection (-1 if no selection)
    int visible_text_start;    // Index of the first visible character (for scrolling text)
    int is_mouse_selecting;    // Flag to track if mouse is being used to select text
} Entry;

// Forward declaration of function to register an entry widget in a global array
void register_widget_entry(Entry* entry);

// Creates a new text entry widget with specified properties
// Parameters:
// - parent: The parent window/container
// - x, y: Logical position relative to parent
// - w: Logical width of the entry
// - max_length: Maximum number of characters allowed
// Returns: Pointer to the new Entry or NULL on failure
Entry* new_entry_(Parent* parent, int x, int y, int w, int max_length) {
    // Validate the parent and its renderer to ensure they exist
    if (!parent || !parent->base.sdl_renderer) {
        printf("Invalid parent or renderer\n");
        return NULL; // Return NULL to indicate failure
    }

    // If no theme is set, default to a light theme to ensure consistent styling
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Get default font size and padding from the current theme for consistent styling
    int logical_font_size = current_theme->default_font_size;
    int logical_padding = current_theme->padding;

    // Allocate memory for the new Entry struct
    Entry* new_entry = (Entry*)malloc(sizeof(Entry));
    if (!new_entry) {
        printf("Failed to allocate memory for entry\n");
        return NULL; // Return NULL if memory allocation fails
    }

    // Initialize the entry's parent pointer
    new_entry->parent = parent;

    // Allocate and set a default placeholder (single space) to avoid null pointer issues
    new_entry->place_holder = strdup(" ");
    if (!new_entry->place_holder) {
        printf("Failed to allocate memory for placeholder\n");
        free(new_entry); // Clean up allocated memory
        return NULL; // Return NULL if placeholder allocation fails
    }

    // Set position and dimensions (logical coordinates)
    new_entry->x = x;
    new_entry->y = y;
    new_entry->w = w;
    new_entry->h = logical_font_size + 2 * logical_padding; // Height based on font size plus padding

    // Set maximum text length
    new_entry->max_length = max_length;

    // Allocate memory for the text buffer, including space for null terminator
    new_entry->text = (char*)malloc(max_length + 1);
    if (!new_entry->text) {
        printf("Failed to allocate memory for entry text\n");
        free(new_entry->place_holder); // Clean up placeholder
        free(new_entry); // Clean up entry
        return NULL; // Return NULL if text buffer allocation fails
    }

    // Initialize text buffer as empty
    new_entry->text[0] = '\0';
    new_entry->is_active = 0; // Entry is not active by default
    new_entry->cursor_pos = 0; // Cursor starts at the beginning
    new_entry->selection_start = -1; // No selection initially
    new_entry->visible_text_start = 0; // Start displaying text from the beginning
    new_entry->is_mouse_selecting = 0; // Initialize mouse selection flag

    // Register the entry in the global widget array for rendering and updates
    register_widget_entry(new_entry);
    return new_entry; // Return the created entry
}

// Renders the text entry widget to the screen
// Parameters:
// - entry: The Entry widget to render
// Renders the text entry widget to the screen
// Parameters:
// - entry: The Entry widget to render
void render_entry(Entry* entry) {
    // Validate inputs to ensure the entry, its parent, and renderer are valid
    if (!entry || !entry->parent || !entry->parent->base.sdl_renderer || !entry->parent->is_open) {
        printf("Invalid entry, renderer, or parent is not open\n");
        return;
    }

    // Default to light theme if none is set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Get DPI scale for converting logical coordinates to physical pixels
    float dpi = entry->parent->base.dpi_scale;

    // Calculate absolute position in logical coordinates, accounting for parent position and title bar
    int abs_x = entry->x + entry->parent->x;
    int abs_y = entry->y + entry->parent->y + entry->parent->title_height;

    // Convert to physical coordinates using DPI scaling
    int sx = (int)roundf(abs_x * dpi);
    int sy = (int)roundf(abs_y * dpi);
    int sw = (int)roundf(entry->w * dpi);
    int sh = (int)roundf(entry->h * dpi);
    int border_width = (int)roundf(2 * dpi);
    int padding = (int)roundf(current_theme->padding * dpi);
    int cursor_width = (int)roundf(2 * dpi);
    int font_size = (int)roundf(current_theme->default_font_size * dpi);

    // Use theme font or default to "FreeMono.ttf" if none specified
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
    Color outline_color = current_theme->accent; // Border color
    Color bg_color = current_theme->bg_secondary; // Background color
    Color cursor_color = current_theme->accent; // Cursor color
    Color highlight_color = current_theme->accent_hovered; // Selection highlight color

    // Draw the entry's border rectangle
    draw_rect_(&entry->parent->base, sx, sy, sw, sh, outline_color);

    // Draw the entry's background (slightly inset to account for border)
    draw_rect_(&entry->parent->base, sx + border_width, sy + border_width, 
               sw - 2 * border_width, sh - 2 * border_width, bg_color);

    // Choose text to display: user text if active or non-empty, otherwise placeholder
    char* display_text = (entry->is_active || entry->text[0] != '\0') 
                        ? entry->text + entry->visible_text_start : entry->place_holder;

    // Set text color based on whether placeholder or user text is shown
    Color text_color = (display_text == entry->place_holder) 
                      ? current_theme->text_secondary : current_theme->text_primary;

    // Calculate text position (centered vertically)
    int text_x = sx + padding;
    int text_y = sy + (sh - font_height) / 2;

    // Set a clipping rectangle to prevent text from drawing outside the entry
    SDL_Rect clip_rect = {sx + border_width, sy + border_width, 
                         sw - 2 * border_width, sh - 2 * border_width};
    SDL_RenderSetClipRect(entry->parent->base.sdl_renderer, &clip_rect);

    // If there's a text selection and the entry is active, draw the highlight
    if (entry->selection_start != -1 && entry->is_active) {
        // Determine selection start and end indices
        int sel_start = (entry->selection_start < entry->cursor_pos 
                        ? entry->selection_start : entry->cursor_pos) - entry->visible_text_start;
        int sel_end = (entry->selection_start < entry->cursor_pos 
                      ? entry->cursor_pos : entry->selection_start) - entry->visible_text_start;

        // Clamp selection indices to valid range
        if (sel_start < 0) sel_start = 0;
        if (sel_end > strlen(display_text)) sel_end = strlen(display_text);

        if (sel_start < sel_end) {
            // Calculate pixel offset for the start of the selection
            char temp_start[entry->max_length + 1];
            strncpy(temp_start, display_text, sel_start);
            temp_start[sel_start] = '\0';
            int highlight_offset = 0;
            TTF_SizeText(font, temp_start, &highlight_offset, NULL);
            int highlight_x = text_x + highlight_offset;

            // Calculate width of the selected text
            strncpy(temp_start, display_text + sel_start, sel_end - sel_start);
            temp_start[sel_end - sel_start] = '\0';
            int highlight_w = 0;
            TTF_SizeText(font, temp_start, &highlight_w, NULL);

            // Draw the selection highlight rectangle
            draw_rect_(&entry->parent->base, highlight_x, text_y, highlight_w, 
                      font_height, highlight_color);
        }
    }

    // Render the visible text (user input or placeholder)
    draw_text_from_font_(&entry->parent->base, font, display_text, text_x, text_y, 
                        text_color, ALIGN_LEFT);

    // Render cursor if the entry is active
    if (entry->is_active) {
        int cursor_offset = 0;
        if (entry->text[0] != '\0') {
            // Calculate pixel offset to the cursor position
            char temp[entry->max_length + 1];
            strncpy(temp, display_text, entry->cursor_pos - entry->visible_text_start);
            temp[entry->cursor_pos - entry->visible_text_start] = '\0';
            TTF_SizeText(font, temp, &cursor_offset, NULL);
        }
        int cursor_x = text_x + cursor_offset;
        // Draw the cursor as a thin vertical rectangle
        draw_rect_(&entry->parent->base, cursor_x, text_y, cursor_width, 
                  font_height, cursor_color);
    }

    // Disable clipping after rendering
    SDL_RenderSetClipRect(entry->parent->base.sdl_renderer, NULL);
    TTF_CloseFont(font); // Free the font resource
}
// Updates the visible portion of the text when the cursor moves or text changes
// Parameters:
// - entry: The Entry widget to update
void update_visible_text(Entry* entry) {
    // Validate inputs
    if (!entry || !entry->parent) {
        printf("Invalid entry or parent\n");
        return; // Exit if validation fails
    }

    // Default to light theme if none is set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Get theme properties
    int logical_font_size = current_theme->default_font_size;
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
    int logical_padding = current_theme->padding;

    // Load the font for text measurements
    TTF_Font* font = TTF_OpenFont(font_file, logical_font_size);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return; // Exit if font loading fails
    }

    // Calculate how many characters can fit within the entry's width (logical)
    int max_visible_width = entry->w - 2 * logical_padding;
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
            break; // Stop when text exceeds visible width
        }
    }

    // Calculate pixel position of the cursor
    int cursor_pixel_x = 0;
    if (entry->cursor_pos > 0) {
        char temp[entry->max_length + 1];
        strncpy(temp, entry->text, entry->cursor_pos);
        temp[entry->cursor_pos] = '\0';
        TTF_SizeText(font, temp, &cursor_pixel_x, NULL);
    }

    // If cursor is beyond visible area, scroll text to keep cursor in view
    if (cursor_pixel_x > max_visible_width) {
        while (cursor_pixel_x > max_visible_width && entry->visible_text_start < strlen(entry->text)) {
            char ch[2] = {entry->text[entry->visible_text_start], '\0'};
            int char_width;
            TTF_SizeText(font, ch, &char_width, NULL);
            cursor_pixel_x -= char_width;
            entry->visible_text_start++;
        }
    } else if (entry->cursor_pos < entry->visible_text_start) {
        // If cursor moves before visible text, scroll back
        entry->visible_text_start = entry->cursor_pos;
    }

    // If there's a selection, try to show as much of it as possible
    if (entry->selection_start != -1) {
        int sel_end = entry->selection_start > entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
        if (sel_end < entry->visible_text_start) {
            entry->visible_text_start = sel_end > max_visible_chars ? sel_end - max_visible_chars : 0;
        }
    }

    // Ensure visible_text_start stays within bounds
    if (entry->visible_text_start > strlen(entry->text) - max_visible_chars) {
        entry->visible_text_start = strlen(entry->text) > max_visible_chars 
                                   ? strlen(entry->text) - max_visible_chars : 0;
    }

    TTF_CloseFont(font); // Free the font resource
}

// Updates the text entry widget based on SDL events (mouse, keyboard, text input)
// Parameters:
// - entry: The Entry widget to update
// - event: The SDL event to process
void update_entry(Entry* entry, SDL_Event event) {
    // Validate inputs to ensure the entry, its parent, and parent state are valid
    if (!entry || !entry->parent || !entry->parent->is_open) {
        printf("Invalid entry, parent, or parent is not open\n");
        return;
    }

    // Default to light theme if none is set for consistent styling
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;
    }

    // Get DPI scale for converting logical to physical coordinates
    float dpi = entry->parent->base.dpi_scale;
    Uint16 mod = SDL_GetModState(); // Get current keyboard modifier state (e.g., Shift, Ctrl)

    // Calculate absolute position in logical coordinates, accounting for parent and title bar
    int abs_x = entry->x + entry->parent->x;
    int abs_y = entry->y + entry->parent->y + entry->parent->title_height;
    // Convert to physical coordinates for hit testing
    int s_abs_x = (int)roundf(abs_x * dpi);
    int s_abs_y = (int)roundf(abs_y * dpi);
    int s_w = (int)roundf(entry->w * dpi);
    int s_h = (int)roundf(entry->h * dpi);

    // Load font for cursor position calculations
    int logical_font_size = current_theme->default_font_size;
    char* font_file = current_theme->font_file ? current_theme->font_file : "FreeMono.ttf";
    int logical_padding = current_theme->padding;
    TTF_Font* font = TTF_OpenFont(font_file, logical_font_size);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return; // Exit if font loading fails
    }

    // Handle mouse button down event (left click)
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        // Check if click is within the entry's bounds
        if (mouseX >= s_abs_x && mouseX <= s_abs_x + s_w &&
            mouseY >= s_abs_y && mouseY <= s_abs_y + s_h) {
            entry->is_active = 1; // Activate the entry
            printf("Entry clicked! Active\n");
            entry->is_mouse_selecting = 1; // Enable mouse-based selection
            entry->selection_start = -1; // Clear existing selection

            // Calculate cursor position from click location
            int logical_mouse_x = (int)roundf(mouseX / dpi);
            int click_offset = logical_mouse_x - (abs_x + logical_padding);
            int cum_width = 0;
            entry->cursor_pos = 0;
            for (int i = 0; i < strlen(entry->text); i++) {
                char ch[2] = {entry->text[i], '\0'};
                int char_w;
                TTF_SizeText(font, ch, &char_w, NULL);
                if (cum_width + char_w / 2 > click_offset) { // Closest character by midpoint
                    break;
                }
                cum_width += char_w;
                entry->cursor_pos = i + 1;
            }
            update_visible_text(entry); // Update visible text to reflect cursor position
        } else {
            entry->is_active = 0; // Deactivate if clicked outside
            entry->is_mouse_selecting = 0; // Stop mouse selection
            printf("Clicked outside entry! Inactive\n");
            entry->selection_start = -1; // Clear selection
        }
    } 
    // Handle mouse button up event (left click)
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        entry->is_mouse_selecting = 0; // End mouse-based selection
    } 
    // Handle mouse motion for drag selection
    else if (event.type == SDL_MOUSEMOTION && entry->is_mouse_selecting && (event.motion.state & SDL_BUTTON_LMASK)) {
        int mouseX = event.motion.x;
        int mouseY = event.motion.y;
        // Update selection if mouse is within entry bounds (optional: can remove bounds check)
        if (mouseX >= s_abs_x && mouseX <= s_abs_x + s_w &&
            mouseY >= s_abs_y && mouseY <= s_abs_y + s_h) {
            // Set selection anchor on first drag motion if not already set
            if (entry->selection_start == -1) {
                entry->selection_start = entry->cursor_pos;
            }
            // Calculate new cursor position from mouse position
            int logical_mouse_x = (int)roundf(mouseX / dpi);
            int click_offset = logical_mouse_x - (abs_x + logical_padding);
            int cum_width = 0;
            int new_cursor_pos = 0;
            for (int i = 0; i < strlen(entry->text); i++) {
                char ch[2] = {entry->text[i], '\0'};
                int char_w;
                TTF_SizeText(font, ch, &char_w, NULL);
                if (cum_width + char_w / 2 > click_offset) {
                    break;
                }
                cum_width += char_w;
                new_cursor_pos = i + 1;
            }
            entry->cursor_pos = new_cursor_pos; // Update cursor to new position
            update_visible_text(entry); // Update visible text to keep cursor/selection in view
        }
    } 
    // Handle text input when entry is active
    else if (event.type == SDL_TEXTINPUT && entry->is_active) {
        // If there's a selection, delete it before inserting new text
        if (entry->selection_start != -1) {
            int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
            int sel_end = entry->selection_start < entry->cursor_pos ? entry->cursor_pos : entry->selection_start;
            memmove(entry->text + sel_start, entry->text + sel_end, strlen(entry->text) - sel_end + 1);
            entry->cursor_pos = sel_start;
            entry->selection_start = -1;
        }
        // Insert new text at cursor position
        int len = strlen(entry->text);
        int input_len = strlen(event.text.text);
        if (len + input_len < entry->max_length) {
            memmove(entry->text + entry->cursor_pos + input_len, 
                    entry->text + entry->cursor_pos, len - entry->cursor_pos + 1);
            strncpy(entry->text + entry->cursor_pos, event.text.text, input_len);
            entry->cursor_pos += input_len;
            update_visible_text(entry);
        }
    } 
    // Handle key presses when entry is active
    else if (event.type == SDL_KEYDOWN && entry->is_active) {
        if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (entry->selection_start != -1) {
                // Delete selected text
                int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                int sel_end = entry->selection_start < entry->cursor_pos ? entry->cursor_pos : entry->selection_start;
                memmove(entry->text + sel_start, entry->text + sel_end, strlen(entry->text) - sel_end + 1);
                entry->cursor_pos = sel_start;
                entry->selection_start = -1;
            } else if (entry->cursor_pos > 0) {
                // Delete character before cursor
                memmove(entry->text + entry->cursor_pos - 1, 
                        entry->text + entry->cursor_pos, strlen(entry->text) - entry->cursor_pos + 1);
                entry->cursor_pos--;
            }
            update_visible_text(entry);
        } else if (event.key.keysym.sym == SDLK_DELETE) {
            if (entry->selection_start != -1) {
                // Delete selected text (same as backspace)
                int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                int sel_end = entry->selection_start < entry->cursor_pos ? entry->cursor_pos : entry->selection_start;
                memmove(entry->text + sel_start, entry->text + sel_end, strlen(entry->text) - sel_end + 1);
                entry->cursor_pos = sel_start;
                entry->selection_start = -1;
            } else if (entry->cursor_pos < strlen(entry->text)) {
                // Delete character after cursor
                memmove(entry->text + entry->cursor_pos, 
                        entry->text + entry->cursor_pos + 1, strlen(entry->text) - entry->cursor_pos);
            }
            update_visible_text(entry);
        } else if (event.key.keysym.sym == SDLK_LEFT) {
            if (entry->cursor_pos > 0) {
                if (mod & KMOD_SHIFT) {
                    // Extend selection with Shift+Left
                    if (entry->selection_start == -1) entry->selection_start = entry->cursor_pos;
                    entry->cursor_pos--;
                } else {
                    // Move cursor left, clear selection
                    entry->cursor_pos--;
                    entry->selection_start = -1;
                }
                update_visible_text(entry);
            }
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            if (entry->cursor_pos < strlen(entry->text)) {
                if (mod & KMOD_SHIFT) {
                    // Extend selection with Shift+Right
                    if (entry->selection_start == -1) entry->selection_start = entry->cursor_pos;
                    entry->cursor_pos++;
                } else {
                    // Move cursor right, clear selection
                    entry->cursor_pos++;
                    entry->selection_start = -1;
                }
                update_visible_text(entry);
            }
        } else if (event.key.keysym.sym == SDLK_RETURN) {
            // Deactivate entry on Enter key
            entry->is_active = 0;
            entry->selection_start = -1;
        } else if (event.key.keysym.sym == SDLK_a && (mod & KMOD_CTRL)) {
            // Ctrl+A: Select all text
            if (strlen(entry->text) > 0) {
                entry->selection_start = 0;
                entry->cursor_pos = strlen(entry->text);
            }
        } else if (event.key.keysym.sym == SDLK_c && (mod & KMOD_CTRL)) {
            // Ctrl+C: Copy selected text to clipboard
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
            // Ctrl+X: Cut selected text (copy to clipboard and delete)
            if (entry->selection_start != -1) {
                int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                int sel_len = abs(entry->cursor_pos - entry->selection_start);
                char* sel_text = malloc(sel_len + 1);
                strncpy(sel_text, entry->text + sel_start, sel_len);
                sel_text[sel_len] = '\0';
                SDL_SetClipboardText(sel_text);
                free(sel_text);
                // Delete selected text
                memmove(entry->text + sel_start, entry->text + sel_start + sel_len, 
                        strlen(entry->text) - (sel_start + sel_len) + 1);
                entry->cursor_pos = sel_start;
                entry->selection_start = -1;
                update_visible_text(entry);
            }
        } else if (event.key.keysym.sym == SDLK_v && (mod & KMOD_CTRL)) {
            // Ctrl+V: Paste text from clipboard
            if (SDL_HasClipboardText()) {
                char* paste_text = SDL_GetClipboardText();
                if (paste_text) {
                    int paste_len = strlen(paste_text);
                    int len = strlen(entry->text);
                    // Delete selection if any
                    if (entry->selection_start != -1) {
                        int sel_start = entry->selection_start < entry->cursor_pos ? entry->selection_start : entry->cursor_pos;
                        int sel_len = abs(entry->cursor_pos - entry->selection_start);
                        memmove(entry->text + sel_start, entry->text + sel_start + sel_len, 
                                len - (sel_start + sel_len) + 1);
                        entry->cursor_pos = sel_start;
                        entry->selection_start = -1;
                        len -= sel_len;
                    }
                    // Insert pasted text if within max length
                    if (len + paste_len < entry->max_length) {
                        memmove(entry->text + entry->cursor_pos + paste_len, 
                                entry->text + entry->cursor_pos, len - entry->cursor_pos + 1);
                        strncpy(entry->text + entry->cursor_pos, paste_text, paste_len);
                        entry->cursor_pos += paste_len;
                        update_visible_text(entry);
                    }
                    SDL_free(paste_text);
                }
            }
        }
    }

    TTF_CloseFont(font); // Free the font resource
}
// Frees the memory allocated for an Entry widget
// Parameters:
// - entry: The Entry widget to free
void free_entry(Entry* entry) {
    if (entry) {
        free(entry->text); // Free the text buffer
        free(entry->place_holder); // Free the placeholder text
        free(entry); // Free the entry itself
    }
}

// ___________________
// Global array to store all registered entry widgets (up to MAX_ENTRYS)
#define MAX_ENTRYS 100
Entry* entry_widgets[MAX_ENTRYS];
int entrys_count = 0; // Tracks the number of registered entries

// Registers an entry widget in the global array
// Parameters:
// - entry: The Entry widget to register
void register_widget_entry(Entry* entry) {
    if (entrys_count < MAX_ENTRYS) {
        entry_widgets[entrys_count] = entry; // Add entry to the array
        entrys_count++; // Increment the count
    }
}

// Renders all registered entry widgets
void render_all_registered_entrys(void) {
    for (int i = 0; i < entrys_count; i++) {
        if (entry_widgets[i]) {
            render_entry(entry_widgets[i]); // Render each valid entry
        }
    }
}

// Updates all registered entry widgets based on an SDL event
// Parameters:
// - event: The SDL event to process
void update_all_registered_entrys(SDL_Event event) {
    for (int i = 0; i < entrys_count; i++) {
        if (entry_widgets[i]) {
            update_entry(entry_widgets[i], event); // Update each valid entry
        }
    }
}

void free_all_registered_entrys(void) {
    for (int i = 0; i < entrys_count; i++) {
        if (entry_widgets[i]) {
            free_entry(entry_widgets[i]);
            entry_widgets[i] = NULL;
        }
    }
    entrys_count = 0;
}
