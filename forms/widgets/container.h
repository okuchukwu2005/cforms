#ifndef CONTAINER_H
#define CONTAINER_H


#define MAX_CONTAINERS 100

static Parent* container_widgets[MAX_CONTAINERS];
static int containers_count = 0;

static inline void register_widget_container(Parent* container) {
    if (containers_count < MAX_CONTAINERS) {
        container_widgets[containers_count] = container;
        containers_count++;
    }
}

static inline Parent* new_container_(Parent* root, int x, int y, int w, int h, Color color) {
    if (!root || !root->is_window) {
        return NULL;
    }

    Parent* parent = malloc(sizeof(Parent));
    if (!parent) return NULL;
    memset(parent, 0, sizeof(Parent));

    // Containers don’t own SDL_Window; just reuse renderer from root
    parent->base.sdl_window   = NULL;
    parent->base.sdl_renderer = root->base.sdl_renderer;

    parent->is_window = 0;
    parent->x = x;
    parent->y = y;
    parent->w = w;
    parent->h = h;
    parent->color = color;

    // Defaults
    parent->moveable = 0;
    parent->title_bar = NULL;
    parent->has_title_bar = false;
    parent->is_dragging = false;
    parent->drag_offset_x = 0;
    parent->drag_offset_y = 0;
    parent->closeable = false;
    parent->resizeable = false;
    parent->is_resizing = false;
    parent->resize_zone = 5;
    parent->is_open = true;
    parent->title_height = 0;

    register_widget_container(parent);
    return parent;
}

static inline void set_container_properties_(Parent* container,
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

    draw_rect_(&container->base,
               container->x, container->y,
               container->w, container->title_height,
               COLOR_BLACK);

    if (container->title_bar) {
        draw_text_(&container->base,
                   container->title_bar,
                   16,
                   container->x + 10,
                   container->y + 5,
                   COLOR_WHITE);
    }

    if (container->closeable) {
        Color close_color = {200, 0, 0, 255};
        int btn_size = 20;
        int btn_x = container->x + container->w - btn_size - 5;
        int btn_y = container->y + 5;
        draw_rect_(&container->base, btn_x, btn_y, btn_size, btn_size, close_color);
        draw_text_(&container->base, "X", 14, btn_x + 5, btn_y + 2,
                   (Color){255, 255, 255, 255});
    }
}

static inline void render_container(Parent* container) {
    if (!container || !container->is_open) return;

    draw_title_bar_(container);

    draw_rect_(&container->base,
               container->x,
               container->y + container->title_height,
               container->w,
               container->h - container->title_height,
               container->color);
}

static inline void update_container(Parent* container, SDL_Event event) {
    if (!container || !container->is_open) return;

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    bool in_title_bar = container->has_title_bar &&
        mouse_x >= container->x &&
        mouse_x <= container->x + container->w &&
        mouse_y >= container->y &&
        mouse_y <= container->y + container->title_height;

    bool in_close_button = false;
    if (container->closeable) {
        int btn_size = 20;
        int btn_x = container->x + container->w - btn_size - 5;
        int btn_y = container->y + 5;
        in_close_button = mouse_x >= btn_x && mouse_x <= btn_x + btn_size &&
                          mouse_y >= btn_y && mouse_y <= btn_y + btn_size;
    }

    bool in_resize_area = container->resizeable &&
        mouse_x >= container->x + container->w - container->resize_zone &&
        mouse_x <= container->x + container->w &&
        mouse_y >= container->y + container->h - container->resize_zone &&
        mouse_y <= container->y + container->h;

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (in_close_button) {
                    container->is_open = false;
                } else if (in_resize_area) {
                    container->is_resizing = true;
                } else if (in_title_bar && container->moveable) {
                    container->is_dragging = true;
                    container->drag_offset_x = mouse_x - container->x;
                    container->drag_offset_y = mouse_y - container->y;
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
                container->x = mouse_x - container->drag_offset_x;
                container->y = mouse_y - container->drag_offset_y;
            } else if (container->is_resizing) {
                container->w = mouse_x - container->x;
                container->h = mouse_y - container->y;
                if (container->h < container->title_height + 50) {
                    container->h = container->title_height + 50;
                }
            }
            break;
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
    } else {
        free(parent);
    }
}

#endif /* CONTAINER_H */
