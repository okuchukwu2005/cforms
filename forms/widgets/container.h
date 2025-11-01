#ifndef CONTAINER_H
#define CONTAINER_H

#include <math.h>  // For roundf in scaling

static inline Parent new_container(Parent* root, int x, int y, int w, int h) {
    if (!root || !root->is_window) {
        printf("invalid parent passed on container!\n");
    }

    Parent parent;
    // Containers don’t own SDL_Window; just reuse renderer from root
    parent.base.sdl_window   = NULL;
    parent.base.sdl_renderer = root->base.sdl_renderer;
    parent.base.dpi_scale    = root->base.dpi_scale;  // Propagate DPI scale from root

    parent.is_window = 0;
    parent.x = x;
    parent.y = y;
    parent.w = w;
    parent.h = h;
    parent.color = (Color){0, 0, 0, 0}; // Default transparent, since color is now from theme

    // Defaults
    parent.moveable = 0;
    parent.title_bar = NULL;
    parent.has_title_bar = false;
    parent.is_dragging = false;
    parent.drag_offset_x = 0;
    parent.drag_offset_y = 0;
    parent.closeable = false;
    parent.resizeable = false;
    parent.is_resizing = false;
    parent.resize_zone = 5;
    parent.is_open = true;
    parent.title_height = 0;

    return parent;
}

static inline void set_container_properties(Parent* container,
                               bool moveable,
                               const char* title,
                               bool has_title_bar,
                               bool closeable/*,
                               bool resizeable*/) {
    if (!container) return;
    container->moveable = moveable;
    container->title_bar = title;
    container->has_title_bar = has_title_bar;
    container->closeable = closeable;
    container->resizeable = false; // should be assigned to resizeable, but feature is off for now
    container->title_height = has_title_bar ? 30 : 0;
}

static inline void draw_title_bar_(Parent* container) {
    if (!container || !container->has_title_bar) return;

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }

    float dpi = container->base.dpi_scale;
    int sx = (int)roundf(container->x * dpi);
    int sy = (int)roundf(container->y * dpi);
    int sw = (int)roundf(container->w * dpi);
    int sth = (int)roundf(container->title_height * dpi);
    int pad = (int)roundf(current_theme->padding * dpi);
    int font_size = (int)roundf(current_theme->default_font_size * dpi);

    draw_rect_(&container->base,
               sx, sy,
               sw, sth,
               current_theme->container_title_bg);  // Use theme-specific title bg

    if (container->title_bar) {
        draw_text_(&container->base,
                   container->title_bar,
                   font_size,
                   sx + pad,
                   sy + pad / 2,  // Approximate vertical centering
                   current_theme->text_primary);  // Use theme text color
    }

    if (container->closeable) {
        // Derive button size from title height for proportionality
        int btn_size = sth - pad;
        int btn_x = sx + sw - btn_size - pad / 2;
        int btn_y = sy + pad / 2;
        // Use slightly smaller font for "X" (90% of default for better fit)
        int close_font_size = (int)roundf(current_theme->default_font_size * 0.9f * dpi);
        draw_text_(&container->base, "X", close_font_size, btn_x + btn_size / 4, btn_y + btn_size / 4,
                   current_theme->text_primary);  // Use theme text color
    }
}

static inline void render_container(Parent* container) {
    if (!container || !container->is_open) return;

    // Fallback if no theme set
    if (!current_theme) {
        current_theme = (Theme*)&THEME_LIGHT;  // Or set a static fallback
    }

    float dpi = container->base.dpi_scale;
    int sx = (int)roundf(container->x * dpi);
    int body_y = (int)roundf((container->y + container->title_height) * dpi);
    int sw = (int)roundf(container->w * dpi);
    int body_h = (int)roundf((container->h - container->title_height) * dpi);

    draw_title_bar_(container);

    // Draw main container rect using theme container_bg
    draw_rect_(&container->base,
               sx,
               body_y,
               sw,
               body_h,
               current_theme->container_bg);
}

static inline void update_container(Parent* container, SDL_Event event) {
    if (!container || !container->is_open) return;

    float dpi = container->base.dpi_scale;
    // Scale bounds to physical
    int s_x = (int)roundf(container->x * dpi);
    int s_y = (int)roundf(container->y * dpi);
    int s_w = (int)roundf(container->w * dpi);
    int s_h = (int)roundf(container->h * dpi);
    int s_title_h = (int)roundf(container->title_height * dpi);
    int s_resize_zone = (int)roundf(container->resize_zone * dpi);

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    bool in_title_bar = container->has_title_bar &&
        mouse_x >= s_x &&
        mouse_x <= s_x + s_w &&
        mouse_y >= s_y &&
        mouse_y <= s_y + s_title_h;

    bool in_close_button = false;
    if (container->closeable) {
        // btn_size logical, scale it
        int btn_size = 20;  // Logical
        int s_btn_size = (int)roundf(btn_size * dpi);
        // Logical btn pos relative to container
        int btn_x = container->x + container->w - btn_size - 5;
        int btn_y = container->y + 5;
        int s_btn_x = (int)roundf(btn_x * dpi);
        int s_btn_y = (int)roundf(btn_y * dpi);
        in_close_button = mouse_x >= s_btn_x && mouse_x <= s_btn_x + s_btn_size &&
                          mouse_y >= s_btn_y && mouse_y <= s_btn_y + s_btn_size;
    }

    bool in_resize_area = container->resizeable &&
        mouse_x >= s_x + s_w - s_resize_zone &&
        mouse_x <= s_x + s_w &&
        mouse_y >= s_y + s_h - s_resize_zone &&
        mouse_y <= s_y + s_h;

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (in_close_button) {
                    container->is_open = false;
                } else if (in_resize_area) {
                    container->is_resizing = true;
                } else if (in_title_bar && container->moveable) {
                    container->is_dragging = true;
                    // Offsets in logical coords (consistent with drag)
                    container->drag_offset_x = (int)roundf(mouse_x / dpi) - container->x;
                    container->drag_offset_y = (int)roundf(mouse_y / dpi) - container->y;
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                container->is_dragging = false;
                container->is_resizing = false;
            }
            break;

        case SDL_MOUSEMOTION:
            if (container->is_dragging) {
                container->x = (int)roundf(mouse_x / dpi) - container->drag_offset_x;
                container->y = (int)roundf(mouse_y / dpi) - container->drag_offset_y;
            } else if (container->is_resizing) {
                container->w = (int)roundf(mouse_x / dpi) - container->x;
                container->h = (int)roundf(mouse_y / dpi) - container->y;
                if (container->h < container->title_height + 50) {
                    container->h = container->title_height + 50;
                }
            }
            break;
    }
}

// registering stuffs

#define MAX_CONTAINERS 100

static Parent* container_widgets[MAX_CONTAINERS];
static int containers_count = 0;

static inline void register_container(Parent* container) {
    if (containers_count < MAX_CONTAINERS) {
        container_widgets[containers_count] = container;
        containers_count++;
    }
}

static inline void render_all_registered_containers(void) {
    for (int i = 0; i < containers_count; i++) {
        render_container(container_widgets[i]);
    }
}

static inline void update_all_registered_containers(SDL_Event event) {
    for (int i = 0; i < containers_count; i++) {
        update_container(container_widgets[i], event);
    }
}

static inline void free_con_(Parent* parent) {
    if (!parent) return;
    if (parent->is_window) {
        destroy_parent(parent);
    }
}

static inline void free_all_registered_containers(void) {
    for (int i = 0; i < containers_count; i++) {
        if (container_widgets[i]) {
            free_con_(container_widgets[i]);
            container_widgets[i] = NULL;
        }
    }
    containers_count = 0;
}

#endif /* CONTAINER_H */
