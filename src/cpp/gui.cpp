// #include "../include/prototypes.h"

// #include <SXL/SXL.h>

// struct gui_t
// {
//     int          w             = 1200;
//     int          h             = 800;
//     TTF_Font    *font          = nullptr;
//     element     *title_element = nullptr;
//     SDL_Texture *title_texture = nullptr;
//     string       win_title;
// };
// static gui_t gui;

// #define GUI_TITLE_COLOR 80, 80, 80, 255

// static void
// init(void) noexcept
// {
//     APP->root->set_size(1400, 800);
//     APP->root->framerate = 240;
//     gui.font = APP->retrieve_new_font(
//         18, "/home/mellw/.vscode-insiders/extensions/narasimapandiyan.jetbrainsmono-1.0.2/JetBrainsMono/"
//             "JetBrainsMono-Medium.ttf");
//     if (gui.font == nullptr)
//     {
//         do_exit();
//     }
//     gui.title_element = APP->root->new_element({GUI_TITLE_COLOR}, {0, 0, APP->width(), 20}, {element::ALIGN_WIDTH});
//     if (openfile->filename != nullptr)
//     {
//         gui.win_title = PROJECT_NAME ": " + string(openfile->filename);
//         APP->root->set_title(gui.win_title.c_str());
//         gui.title_texture =
//             APP->make_text_texture(gui.font, gui.win_title.c_str(), {0, 0, 0, 255}, {255, 255, 255, 255});
//         if (gui.title_texture != nullptr)
//         {
//             gui.title_element->add_text_data(22, -4, gui.title_texture);
//         }
//     }
//     APP->root->set_min_size(400, 200);
// }

// static MVector<pair<element *, SDL_Texture *>> lines;
// #define LINE_HEIGHT 20
// #define LINE_FG     255, 255, 255, 255
// #define LINE_BG     30, 30, 30, 255

// /* All this is temporary i will add a 'sdl_element *' and a 'SDL_Texture *'
//  * to linestruct and create one of each for all lines in the file. */
// static void
// set_lines(void) noexcept
// {
//     if (lines.empty())
//     {
//         for (int i = 1; (i * LINE_HEIGHT) < (APP->height() * 2); i++)
//         {
//             element *line = APP->root->new_element(
//                 {LINE_BG}, {0, (float)(i * LINE_HEIGHT), APP->width(), LINE_HEIGHT}, {element::ALIGN_WIDTH});
//             lines.push_back({line, nullptr});
//         }
//         for (auto &[elem, texture] : lines)
//         {
//             SDL_Texture *line_texture = APP->make_text_texture(gui.font, "hello", {LINE_FG}, {LINE_BG});
//             texture                   = line_texture;
//         }
//         NLOG("lines created: %i.\n", lines.size());
//     }
//     int i = 0;
//     for (linestruct *line = openfile->filetop; line != nullptr && (line->lineno < lines.size()) && i < lines.size();
//          line             = line->next)
//     {
//         lines[i].second = APP->make_destroy_text_texture(lines[i].second, gui.font, line->data, {LINE_FG}, {LINE_BG});
//         lines[i].first->add_text_data(2, -2, lines[i].second);
//         i++;
//     }
// }

// static button_element *left_menu_button = nullptr;
// static bool            left_menu_open   = false;
// static element        *left_menu        = nullptr;

// #define ANIM_DURATION 50

// int
// run_gui(void) noexcept
// {
//     init();
//     left_menu_button = APP->root->new_button_element({0, 0, 0, 255}, {2, 2, 16, 16}, {});
//     left_menu_button->set_highlight_on_hover(RED);
//     left_menu        = APP->root->new_element({255, 255, 255, 255}, {-100, 0, 100, APP->height()}, {});
//     left_menu_button->action([](SDL_MouseButtonEvent e) {
//         if (!left_menu_button->flags.is_set<element::IN_ANIMATION>())
//         {
//             if (left_menu_open == false)
//             {
//                 left_menu_open = true;
//                 gui.title_element->animate(200, 0, APP->width() - 200, 20, ANIM_DURATION);
//                 left_menu_button->animate(202, 2, 16, 16, ANIM_DURATION);
//                 left_menu->animate(0, 0, 200, APP->height(), ANIM_DURATION);
//                 for (const auto &[element, texture] : lines)
//                 {
//                     element->animate(202, element->rect.y, APP->width() - 200, LINE_HEIGHT, ANIM_DURATION);
//                     if (element->rect.y > APP->height())
//                     {
//                         break;
//                     }
//                 }
//             }
//             else
//             {
//                 left_menu_open = false;
//                 gui.title_element->animate(0, 0, APP->width(), 20, ANIM_DURATION);
//                 left_menu_button->animate(2, 2, 16, 16, ANIM_DURATION);
//                 left_menu->animate(-200, 0, 200, APP->height(), ANIM_DURATION);
//                 for (const auto &[element, texture] : lines)
//                 {
//                     element->animate(2, element->rect.y, APP->width(), LINE_HEIGHT, ANIM_DURATION);
//                     if (element->rect.y > APP->height())
//                     {
//                         break;
//                     }
//                 }
//             }
//         }
//     });
//     set_lines();
//     APP->set_main_loop([]() {
//         gui.title_element->draw();
//         left_menu_button->draw();
//         left_menu->draw();
//     });
//     EVENT_HANDLER->event_action(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, [](SDL_Event ev) {
//         SDL_WindowEvent e = ev.window;
//         gui.title_element->rect.w = e.data1;

//         left_menu->rect.h         = e.data2;
//         for (const auto &[element, texture] : lines)
//         {
//             if (left_menu_open == false)
//             {
//                 element->rect.w = e.data1;
//             }
//             else
//             {
//                 element->rect.x = 200;
//                 element->rect.w = e.data1 - 200;
//             }
//             if (element->rect.y > e.data2)
//             {
//                 break;
//             }
//         }
//     });
//     EVENT_HANDLER->event_action(SDL_EVENT_KEY_DOWN, [](SDL_Event ev) {
//         SDL_KeyboardEvent e = ev.key;
//         switch (e.scancode)
//         {
//             case SDL_SCANCODE_Q :
//             {
//                 if (e.mod & SDL_KMOD_LCTRL)
//                 {
//                     APP->quit();
//                 }
//                 break;
//             }
//             case SDL_SCANCODE_B :
//             {
//                 if (e.mod & SDL_KMOD_LCTRL)
//                 {
//                     SDL_MouseButtonEvent e;
//                     left_menu_button->_action(e);
//                 }
//                 break;
//             }
//             default :
//             {
//                 break;
//             }
//         }
//     });
//     APP->run();
//     do_exit();
//     return 0;
// }

// /*
// #include <xcb/xcb.h>

// void
// init_window(void)
// {
//     xcb_connection_t *conn    = xcb_connect(NULL, NULL);
//     xcb_screen_t     *screen  =
// xcb_setup_roots_iterator(xcb_get_setup(conn)).data; unsigned int      window  =
// xcb_generate_id(conn); unsigned int      mask    = XCB_CW_BACK_PIXEL |
// XCB_CW_EVENT_MASK; unsigned int      value[] = {screen->black_pixel,
// XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS}; xcb_create_window(conn, 0L,
// window, screen->root, 0, 0, 800, 600, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT,
//                       screen->root_visual, mask, value);
//     xcb_map_window(conn, window);
//     xcb_flush(conn);
//     xcb_generic_event_t *event;
//     bool                 running = TRUE;
//     while (running && (event = xcb_wait_for_event(conn)))
//     {
//         switch (event->response_type & ~0x80)
//         {
//             case XCB_EXPOSE :
//                 xcb_flush(conn);
//                 break;
//             case XCB_KEY_PRESS :
//                 running = FALSE;
//                 break;
//         }
//         free(event);
//     }
//     xcb_disconnect(conn);
// }
//  */
