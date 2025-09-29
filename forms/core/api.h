/**
* @author Okuchukwu2005
* @date August 7, 2025
*/

#ifndef API_H
#define API_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color.h"

// Assume Color is defined elsewhere; if not, define it here.
// typedef struct { uint8_t r, g, b, a; } Color;

/**
 * @brief Base struct that bundles SDL_Window and SDL_Renderer.
 */
typedef struct {
    SDL_Window* sdl_window;
    SDL_Renderer* sdl_renderer;
} Base;

/**
 * @brief This struct represents both root windows and containers.
 *        For root windows, is_window is 1 and SDL fields in Base are initialized.
 *        For containers, is_window is 0 and Base fields are NULL.
 */
typedef struct {
    Base base;                 // Holds SDL_Window + SDL_Renderer
    int is_window;             // 1 for root window, 0 for container
    int x, y, w, h;            // Position and size (x,y=0 for root window)
    Color color;               // Background color (transparent for root window)
    int moveable;              // Can be moved (0 for root window)
    const char* title_bar;     // Title bar text (NULL for root window)
    bool has_title_bar;        // Has title bar (false for root window)
    bool is_dragging;          // Is being dragged
    int drag_offset_x;         // Drag offset X
    int drag_offset_y;         // Drag offset Y
    bool closeable;            // Can be closed (false for root window)
    bool resizeable;           // Can be resized (false for root window)
    bool is_resizing;          // Is being resized
    int resize_zone;           // Size of edge zone for resizing
    bool is_open;              // Is open (true for root window)
    int title_height;          // Height of title bar (0 for root window)
} Parent;

/**
 * @brief Creates a new root window as a Parent struct.
 * @param title The title of the window.
 * @param w The width of the window.
 * @param h The height of the window.
 * @return Pointer to the created Parent, or NULL on failure.
 */
static inline Parent* new_window_(char* title, int w, int h) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return NULL;
    }

    if (TTF_Init() == -1) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Window* sdl_win = SDL_CreateWindow(title,
                                           SDL_WINDOWPOS_CENTERED,
                                           SDL_WINDOWPOS_CENTERED,
                                           w, h,
                                           SDL_WINDOW_SHOWN);
    if (!sdl_win) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    SDL_Renderer* sdl_ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl_ren) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl_win);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    Parent* parent = (Parent*)malloc(sizeof(Parent));
    if (!parent) {
        SDL_DestroyRenderer(sdl_ren);
        SDL_DestroyWindow(sdl_win);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }

    memset(parent, 0, sizeof(Parent));
    parent->base.sdl_window = sdl_win;
    parent->base.sdl_renderer = sdl_ren;
    parent->is_window = 1;
    parent->w = w;
    parent->h = h;
    parent->color = (Color){0, 0, 0, 0};  // Transparent
    parent->is_open = true;

    return parent;
}

/**
 * @brief Destroys the Parent struct, cleaning up SDL resources if it's a root window.
 * @param parent Pointer to the Parent to destroy.
 */
static inline void destroy_parent(Parent* parent) {
    if (!parent) return;

    if (parent->is_window) {
        if (parent->base.sdl_renderer) {
            SDL_DestroyRenderer(parent->base.sdl_renderer);
        }
        if (parent->base.sdl_window) {
            SDL_DestroyWindow(parent->base.sdl_window);
        }
        TTF_Quit();
        SDL_Quit();
    }

    free(parent);
}

#endif /* API_H */