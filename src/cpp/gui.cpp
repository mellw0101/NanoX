#include "../include/prototypes.h"

#include <xcb/xcb.h>

void
init_window(void)
{
    xcb_connection_t *conn    = xcb_connect(NULL, NULL);
    xcb_screen_t     *screen  = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    unsigned int      window  = xcb_generate_id(conn);
    unsigned int      mask    = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    unsigned int      value[] = {screen->black_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS};
    xcb_create_window(conn, 0L, window, screen->root, 0, 0, 800, 600, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT,
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
