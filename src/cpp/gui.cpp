#include "../include/prototypes.h"

#include <Mlib/Sdl2.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct gui_t
{
    SDL_Window   *win;
    SDL_Renderer *ren;
    SDL_Event     event;
    int           w = 1200;
    int           h = 800;
    bool          running;
    // Set renderer color.
    void set_color(unsigned char r, unsigned char g, unsigned char b,
                   unsigned char a)
    {
        SDL_SetRenderDrawColor(ren, r, g, b, a);
    }
};
static gui_t gui;

struct element_t
{
    SDL_Rect  r;
    SDL_Color c;
};
#define GUI_TITLE_COLOR 255, 255, 255, 255
#define GUI_TITLE_RECT  0, 0, 20, gui.w

void init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        logE("Failed to init sdl, SDL_ERROR: %s.", SDL_GetError());
        exit(1);
    }
    if (TTF_Init() == -1)
    {
        logE("Failed to init ttf, SLD_ERROR: %s.", SDL_GetError());
        exit(1);
    }
    gui.win = SDL_CreateWindow("placeholder", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, gui.w, gui.h,
                               SDL_WINDOW_SHOWN);
    if (gui.win == nullptr)
    {
        logE("Failed to create window, SDL_ERROR: %s.", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    gui.ren = SDL_CreateRenderer(gui.win, -1, SDL_RENDERER_ACCELERATED);
    if (gui.ren == nullptr)
    {
        logE("Failed to create renderer, SDL_ERROR: %s.", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    gui.running = true;
}

void draw_rect(SDL_Rect *rect, SDL_Color *c)
{
    SDL_SetRenderDrawColor(gui.ren, c->r, c->g, c->b, c->a);
    SDL_RenderFillRect(gui.ren, rect);
}

int run_gui(void)
{
    init();
    while (gui.running)
    {
        SDL_SetRenderDrawColor(gui.ren, 0, 0, 0, 255);
        SDL_RenderClear(gui.ren);
        prosses_callback_queue();
        while (SDL_PollEvent(&gui.event))
        {
            if (gui.event.type == SDL_QUIT)
            {
                gui.running = false;
                break;
            }
        }
        Mlib::Sdl2::KeyObject::Instance()->handleKeyEvent();
        SDL_RenderPresent(gui.ren);
        SDL_Delay(((double)1 / 120) * 1000);
    }
    SDL_DestroyWindow(gui.win);
    SDL_DestroyRenderer(gui.ren);
    SDL_Quit();
    return 0;
}

/*
#include <xcb/xcb.h>

void
init_window(void)
{
    xcb_connection_t *conn    = xcb_connect(NULL, NULL);
    xcb_screen_t     *screen  =
xcb_setup_roots_iterator(xcb_get_setup(conn)).data; unsigned int      window  =
xcb_generate_id(conn); unsigned int      mask    = XCB_CW_BACK_PIXEL |
XCB_CW_EVENT_MASK; unsigned int      value[] = {screen->black_pixel,
XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS}; xcb_create_window(conn, 0L,
window, screen->root, 0, 0, 800, 600, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, mask, value);
    xcb_map_window(conn, window);
    xcb_flush(conn);
    xcb_generic_event_t *event;
    bool                 running = TRUE;
    while (running && (event = xcb_wait_for_event(conn)))
    {
        switch (event->response_type & ~0x80)
        {
            case XCB_EXPOSE :
                xcb_flush(conn);
                break;
            case XCB_KEY_PRESS :
                running = FALSE;
                break;
        }
        free(event);
    }
    xcb_disconnect(conn);
}
 */
