/* --------------------------------------------------------------- */
/*  checkbox.c  –  nice-looking SDL2 checkbox (C)                  */
/* --------------------------------------------------------------- */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_W      800
#define SCREEN_H      600
#define BOX_SIZE      28               /* size of the square */
#define FONT_SIZE     20
#define PADDING       8                /* space between box and text */

typedef struct {
    SDL_Rect  rc;      /* whole control (box + text) */
    SDL_Rect  rcBox;   /* just the square */
    bool      checked;
    bool      hovered;
    const char *text;
} CheckBox;

/* --------------------------------------------------------------- */
static SDL_Window   *win   = NULL;
static SDL_Renderer *ren   = NULL;
static TTF_Font     *font  = NULL;

/* --------------------------------------------------------------- */
static void init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); exit(1);
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError()); exit(1);
    }

    win = SDL_CreateWindow("SDL2 Checkbox – C",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); exit(1); }

    ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); exit(1); }

    /* change path if needed */
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", FONT_SIZE);
    if (!font) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        /* fallback – you can skip text if you want */
    }
}

/* --------------------------------------------------------------- */
static void quit_all(void)
{
    if (font) TTF_CloseFont(font);
    TTF_Quit();
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}

/* --------------------------------------------------------------- */
/*  Draw a thick line by sweeping parallel lines */
static void draw_thick_line(SDL_Renderer *r,
                            int x1, int y1, int x2, int y2,
                            int thickness, Uint8 r0, Uint8 g0, Uint8 b0)
{
    int dx = x2 - x1, dy = y2 - y1;
    float len = SDL_sqrtf(dx*dx + dy*dy);
    if (len == 0) return;

    float nx = -dy / len;
    float ny =  dx / len;
    int half = thickness / 2;

    for (int i = -half; i <= half; ++i) {
        int ox = (int)(nx * i);
        int oy = (int)(ny * i);
        SDL_SetRenderDrawColor(r, r0, g0, b0, 255);
        SDL_RenderDrawLine(r, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
    }
}

/* --------------------------------------------------------------- */
static void checkbox_render(const CheckBox *cb, SDL_Color col)
{
    /* ---- 1. filled white box ---- */
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderFillRect(ren, &cb->rcBox);

    /* ---- 2. border (darker when hovered) ---- */
    SDL_Color border = cb->hovered ? (SDL_Color){60,60,60,255}
                                   : (SDL_Color){130,130,130,255};
    SDL_SetRenderDrawColor(ren, border.r, border.g, border.b, 255);
    SDL_RenderDrawRect(ren, &cb->rcBox);

    /* ---- 3. check-mark (thick) ---- */
    if (cb->checked) {
        SDL_Point p1 = { cb->rcBox.x + 4, cb->rcBox.y + cb->rcBox.h/2 };
        SDL_Point p2 = { cb->rcBox.x + cb->rcBox.w/2, cb->rcBox.y + cb->rcBox.h - 5 };
        SDL_Point p3 = { cb->rcBox.x + cb->rcBox.w - 5, cb->rcBox.y + 5 };

        int thick = BOX_SIZE / 6;               /* adjust thickness here */
        draw_thick_line(ren,
                        p1.x, p1.y, p2.x, p2.y,
                        thick, col.r, col.g, col.b);
        draw_thick_line(ren,
                        p2.x, p2.y, p3.x, p3.y,
                        thick, col.r, col.g, col.b);
    }

    /* ---- 4. text ---- */
    if (font && cb->text) {
        SDL_Surface *surf = TTF_RenderText_Blended(font, cb->text, col);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
            SDL_Rect dst = { cb->rcBox.x + cb->rcBox.w + PADDING,
                             cb->rc.y + (cb->rc.h - surf->h)/2,
                             surf->w, surf->h };
            SDL_RenderCopy(ren, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }
}

/* --------------------------------------------------------------- */
int main(void)
{
    init();

    CheckBox cb = {0};
    cb.text = "Enable feature";
    cb.checked = false;

    /* centre the whole control */
    int text_w = 0, text_h = 0;
    if (font) {
        TTF_SizeText(font, cb.text, &text_w, &text_h);
    }
    int total_w = BOX_SIZE + PADDING + text_w;
    int total_h = (BOX_SIZE > text_h) ? BOX_SIZE : text_h;

    cb.rc.w = total_w;
    cb.rc.h = total_h;
    cb.rc.x = (SCREEN_W - total_w) / 2;
    cb.rc.y = (SCREEN_H - total_h) / 2;

    cb.rcBox.x = cb.rc.x;
    cb.rcBox.y = cb.rc.y;
    cb.rcBox.w = cb.rcBox.h = BOX_SIZE;

    SDL_Color txtCol = {30, 30, 30, 255};

    bool quit = false;
    while (!quit) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) quit = true;
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;
                if (SDL_PointInRect(&(SDL_Point){mx,my}, &cb.rcBox))
                    cb.checked = !cb.checked;
            }
        }

        /* hover */
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        cb.hovered = SDL_PointInRect(&(SDL_Point){mx,my}, &cb.rcBox);

        /* render */
        SDL_SetRenderDrawColor(ren, 240, 240, 240, 255);
        SDL_RenderClear(ren);
        checkbox_render(&cb, txtCol);
        SDL_RenderPresent(ren);
    }

    quit_all();
    return 0;
}
