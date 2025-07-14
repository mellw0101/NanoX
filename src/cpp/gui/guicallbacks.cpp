/** @file guicallbacks.cpp

  @author  Melwin Svensson.
  @date    2-1-2025.

 */
#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

// #define DOUBLE_PRESS_THRESHOLD 0.2

// typedef enum {
//   #define MOUSEFLAG_SIZE 8
//   LEFT_MOUSE_BUTTON_HELD,
//   #define LEFT_MOUSE_BUTTON_HELD LEFT_MOUSE_BUTTON_HELD
//   RIGHT_MOUSE_BUTTON_HELD,
//   #define RIGHT_MOUSE_BUTTON_HELD RIGHT_MOUSE_BUTTON_HELD
//   WAS_MOUSE_DOUBLE_PRESS,
//   #define WAS_MOUSE_DOUBLE_PRESS WAS_MOUSE_DOUBLE_PRESS
//   WAS_MOUSE_TRIPPLE_PRESS,
//   #define WAS_MOUSE_TRIPPLE_PRESS WAS_MOUSE_TRIPPLE_PRESS
// } mouseflag_type;

/* Flags to represent what the mouse is currently doing, across callback functions. */
// static bit_flag_t<MOUSEFLAG_SIZE> mouse_flag;
/* The element that was last entered, and witch element just ran its enter callback. */
static Element *entered_element = NULL;
/* The mouse position. */
// vec2 mousepos;

SyntaxFile *sf = NULL;

/* Window resize callback. */
void window_resize_callback(GLFWwindow *window, int width, int height) {
  ASSERT_MSG(gui, "gui has not been init.");
  static ivec2 was_size = -1;
  /* To reduce the number of updates, only update the size when it changes. */
  if (was_size != ivec2(width, height)) {
    was_size = ivec2(width, height);
    gui->width  = width;
    gui->height = height;
    gui_width  = width;
    gui_height = height;
    /* Set viewport. */
    glViewport(0, 0, gui->width, gui->height);
    /* Set the projection. */
    matrix4x4_set_orthographic(gui->projection, 0, gui->width, gui->height, 0, -1.0f, 1.0f);
    /* Upload it to both the shaders. */
    update_projection_uniform(font_shader);
    update_projection_uniform(rect_shader);
    /* Calculate the rows for all editors. */
    CLIST_ITER(starteditor, editor,
      if (texture_font_is_mono(gui_font_get_font(textfont))) {
        texture_glyph_t *glyph = texture_font_get_glyph(gui_font_get_font(textfont), " ");
        if (!glyph) {
          die("%s: Atlas is to small.\n", __func__);
        }
        editor->cols = (editor->text->width / glyph->advance_x);
      }
      else {
        editor->cols = ((editor->text->width / FONT_WIDTH(gui_font_get_font(textfont))) * 0.9f);
      }
      editor_resize(editor);
    );
    element_resize(gui->root, gui->width, gui->height);
    refresh_needed = TRUE;
  }
}

/* Maximize callback. */
void window_maximize_callback(GLFWwindow *window, int maximized) {
  ivec2 size;
  glfwGetWindowSize(window, &size.w, &size.h);
  window_resize_callback(window, size.w, size.h);
}

/* Framebuffer resize callback. */
void framebuffer_resize_callback(GLFWwindow *window, int width, int height) {
  window_resize_callback(window, width, height);
}

/* Key callback. */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  /* Key-callbacks for when we are when inside the prompt-mode. */
  if (gui->flag.is_set<GUI_PROMPT>()) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      switch (key) {
        case GLFW_KEY_ENTER: {
          if (!mods) {
            gui_promptmenu_enter_action();
          }
          break;
        }
        /* Cancel the prompt-mode. */
        case GLFW_KEY_C:
        case GLFW_KEY_Q: {
          if (mods != GLFW_MOD_CONTROL) {
            break;
          }
          _FALLTHROUGH;
        }
        case GLFW_KEY_ESCAPE: {
          gui_promptmode_leave();
          break;
        }
        /* Undo */
        case GLFW_KEY_Z: {
          if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_undo();
          }
          break;
        }
        /* Redo */
        case GLFW_KEY_Y: {
          if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_redo();
          }
          break;
        }
        /* Move actions. */
        case GLFW_KEY_RIGHT: {
          if (!mods) {
            do_statusbar_right();
          }
          else if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_next_word();
          }
          break;
        }
        case GLFW_KEY_LEFT: {
          if (!mods) {
            do_statusbar_left();
          }
          else if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_prev_word();
          }
          break;
        }
        case GLFW_KEY_DOWN: {
          gui_promptmenu_selected_down();
          break;
        }
        case GLFW_KEY_UP: {
          gui_promptmenu_selected_up();
          break;
        }
        case GLFW_KEY_HOME: {
          if (!mods) {
            do_statusbar_home();
          }
          break;
        }
        case GLFW_KEY_END: {
          if (!mods) {
            do_statusbar_end();
          }
          break;
        }
        /* Erase actions. */
        case GLFW_KEY_BACKSPACE: {
          if (!mods) {
            do_statusbar_backspace(TRUE);
            gui_promptmenu_completions_search();
          }
          else if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_chop_prev_word();
            gui_promptmenu_completions_search();
          }
          break;
        }
        case GLFW_KEY_DELETE: {
          if (!mods) {
            do_statusbar_delete();
            gui_promptmenu_completions_search();
          }
          else if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_chop_next_word();
            gui_promptmenu_completions_search();
          }
          break;
        }
      }
      gui->promptmenu->text_refresh_needed = TRUE;
      scrollbar_refresh_needed(gui->promptmenu->sb);
    }
  }
  /* When inside the guieditor key mode, do nothing as we handle further keys in the char callback. */
  else if (gui->flag.is_set<GUI_EDITOR_MODE_KEY>()) {
    if ((action == GLFW_PRESS || action == GLFW_REPEAT) && mods) {
      gui->flag.unset<GUI_EDITOR_MODE_KEY>();
    }
  }
  /* Otherwise, do the text bindings. */
  else {
    // mouse_flag.clear();
    clear_mouse_flags();
    /* Reset shift. */
    shift_held = FALSE;
    /* What function does the key entail. */
    void (*function)(void) = NULL;
    /* Check what action was done, if any. */
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      switch (key) {
        case GLFW_KEY_A: {
          /* Shift+Ctrl+A.  Enclose marked region. */
          if (mods == (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) {
            /* When the file type is c based allow for creating block comment around the marked region. */
            if (GUI_OF->is_c_file || GUI_OF->is_cxx_file || GUI_OF->is_glsl_file /* GUI_OF->type.is_set<C_CPP>() || GUI_OF->type.is_set<GLSL>() */) {
              function = do_block_comment;
            }
          }
          /* Ctrl+A.  Mark the entire file. */
          else if (mods == GLFW_MOD_CONTROL) {
            GUI_OF->mark      = GUI_OF->filetop;
            GUI_OF->mark_x    = 0;
            GUI_OF->current   = GUI_OF->filebot;
            GUI_OF->current_x = strlen(GUI_OF->filebot->data);
            GUI_OF->softmark  = TRUE;
            refresh_needed      = TRUE;
          }
          break;
        }
        case GLFW_KEY_C: {
          /* Copy action.  TODO: Make this more complete. */
          if (mods == GLFW_MOD_CONTROL) {
            if (GUI_OF->mark) {
              copy_marked_region();
              linestruct *bottom = cutbuffer;
              while (bottom->next) {
                bottom = bottom->next;
              }
              char *string = (char *)xmalloc(1);
              *string = '\0';
              Ulong len = 0;
              for (linestruct *line = cutbuffer; line; line = line->next) {
                if (!*line->data) {
                  len = append_to(&string, len, "\n", 1);
                }
                else {
                  len = append_to(&string, line->data);
                  len = append_to(&string, len, "\n", 1);
                }
              }
              string[len - 1] = '\0';
              free_lines(cutbuffer);
              cutbuffer = NULL;
              glfwSetClipboardString(NULL, string);
              free(string);
            }
            /* When there is not a marked region, just copy the data on the current line. */
            else {
              char *string = fmtstr("%s\n", GUI_OF->current->data);
              glfwSetClipboardString(window, string);
              free(string);
            }
          }
          break;
        }
        case GLFW_KEY_E: {
          if (mods == GLFW_MOD_CONTROL) {
            gui->flag.set<GUI_EDITOR_MODE_KEY>();
          }
          break;
        }
        case GLFW_KEY_N: {
          /* Line numbers toggle. */
          if (action == GLFW_PRESS && mods == GLFW_MOD_ALT) {
            TOGGLE(LINE_NUMBERS);
            show_toggle_statusmsg(LINE_NUMBERS);
            editor_update_all();
          }
          /* Open a new editor. */
          else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)) {
            editor_create(TRUE);
            editor_redecorate(openeditor);
            editor_resize(openeditor);
          }
          /* Open a new empty buffer in the currently open editor. */
          else if (mods == GLFW_MOD_CONTROL) {
            editor_open_new_empty_buffer();
          }
          break;
        }
        case GLFW_KEY_O: {
          if (mods == GLFW_MOD_CONTROL) {
            gui_promptmenu_open_file();
          }
          break;
        }
        case GLFW_KEY_P: {
          /* Debuging. */
          if (mods == GLFW_MOD_CONTROL) {
            if (sf) {
              syntaxfile_free(sf);
            }
            sf = syntaxfile_create();
            syntaxfile_read(sf, GUI_OF->filename);
            syntaxfile_parse_csyntax(sf);
            for (SyntaxFileError *err = sf->errtop; err; err = err->next) {
              printf("%s:[%d:%d]: %s\n", err->file, err->pos->row, err->pos->column, err->msg);
            }
            // hashmap_forall(sf->objects, [](const char *const restrict path, void *arg) {
            //   // writef("%s\n", path);
            //   SyntaxObject *obj = (SyntaxObject *)arg;
            //   while (obj) {
            //     if (obj->type == SYNTAX_OBJECT_TYPE_C_STRUCT) {
            //       CSyntaxMacro *macro = (CSyntaxMacro *)obj->data;
            //       writef("\nMacro: %s\n", path);
            //     }
            //   }
            // });
            // hashmap_forall(sf->objects, [](const char *const __restrict key, void *value) {
            //   SyntaxObject *obj = (SyntaxObject *)value;
            //   while (obj) {
            //     if (obj->type == SYNTAX_OBJECT_TYPE_C_MACRO) {
            //       CSyntaxMacro *macro = (CSyntaxMacro *)obj->data;
            //       printf("\nMacro: %s\n", key);
            //       printf("  Start pos: (%d:%d)\n", macro->expandstart->row, macro->expandstart->column);
            //       printf("  End pos: (%d:%d)\n", macro->expandend->row, macro->expandend->column);
            //       printf("  Full macro:\n");
            //       printf("    %s", key);
            //       if (macro->argv) {
            //         printf("( ");
            //         for (char **arg = macro->argv; *arg; ++arg) {
            //           printf("%s ", *arg);
            //         }
            //         printf(") ");
            //       }
            //       else {
            //         printf(" ");
            //       }
            //       if (macro->expanded) {
            //         char *trimmed = stripleadblanks(copy_of(macro->expanded), NULL);
            //         printf("%s\n", trimmed);
            //         free(trimmed);
            //       }
            //     }
            //     obj = obj->next;
            //   }
            // });
            // syntaxfile_free(sf);
            refresh_needed = TRUE;
          }
          else if (mods == GLFW_MOD_ALT) {
            syntaxfile_test_read();
          }
          else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) {
            // gui_element_move_resize(openeditor->main, vec2(((float)gui->width / 2), openeditor->main->pos.y), vec2(((float)gui->width / 2), openeditor->main->size.h));
            // gui_editor_rows_cols(openeditor);
            // gui_etb_text_refresh_needed(openeditor->etb);
            // if (begpar(openfile->current, 0)) {
            //   writef("Line is begpar.\n");
            // }
            // else {
            //   writef("Line is not begpar.\n");
            // }
            // Ulong len;
            // float *array = pixpositions("\t\tballe", 0, &len, textfont);
            // for (Ulong i=0; i<len; ++i) {
            //   writef("Index %lu: %.2f\n", i, (double)array[i]);
            // }
            // Ulong index = closest_index(array, len, 17.5, textfont);
            // writef("\nClosest Index: %lu\n", index);
            // free(array);

            // static int toggle=0;
            // if (!(toggle % 2)) {
            //   set_gui_font("/usr/share/fonts/balle.ttf", gui->font_size);
            // }
            // else {
            //   set_gui_font("/usr/share/fonts/TTF/Hack-Regular.ttf", gui->font_size);
            // }
            // ++toggle;
            // refresh_needed = TRUE;
          }
          /* Prompt-Menu */
          else if (mods == (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) {
            // gui_ask_user(">", GUI_PROMPT_MENU);
            if (GUI_OF->current->data[GUI_OF->current_x] == '}') {
              linestruct *line;
              Ulong x;
              if (get_previous_char_match(GUI_OF->current, GUI_OF->current_x, "{", FALSE, &line, &x)) {
                writef("hello\n");
                GUI_OF->current = line;
                GUI_OF->current_x = x;
                refresh_needed = TRUE;
              }
            }
          }
          break;
        }
        case GLFW_KEY_Q: {
          /* If CTRL+Q is pressed, quit. */
          if (mods == GLFW_MOD_CONTROL) {
            if (!GUI_OF->modified || ISSET(VIEW_MODE)) {
              if (gui_quit()) {
                return;
              }
            }
            else if (ISSET(SAVE_ON_EXIT) && *GUI_OF->filename) {
              ;
            }
            else {
              char *question = fmtstr("Close [%s] without saving?", (*GUI_OF->filename ? GUI_OF->filename : "Nameless"));
              gui_ask_user(question, GUI_PROMPT_EXIT_NO_SAVE);
              free(question);
              gui->promptmenu->closing_file = GUI_OF;
            }
          }
          break;
        }
        case GLFW_KEY_S: {
          /* Saving action. */
          if (mods == GLFW_MOD_CONTROL) {
            /* True if the current file has a name. */
            bool has_name = *GUI_OF->filename;
            /* If the openfile has a name, and if the file already exists. */
            if (has_name && file_exists(GUI_OF->filename)) {
              function = do_savefile;
            }
            /* Otherwise, ask if the user wants to save the file. */
            else {
              gui_ask_user("Save file", GUI_PROMPT_SAVEFILE);
              /* If the file has a name but is not a file yet then add that name to the answer. */
              if (has_name) {
                answer   = free_and_assign(answer, copy_of(GUI_OF->filename));
                typing_x = strlen(answer);
              }
              return;
            }
          }
          /* Softwrap toggle. */
          else if (action == GLFW_PRESS && mods == GLFW_MOD_ALT) {
            TOGGLE(SOFTWRAP);
            if (!ISSET(SOFTWRAP)) {
              GUI_OF->firstcolumn = 0;
            }
            show_toggle_statusmsg(SOFTWRAP);
            refresh_needed = TRUE;
          }
          break;
        }
        case GLFW_KEY_T: {
          /* Toggle tabs to spaces. */
          if (action == GLFW_PRESS && mods == GLFW_MOD_ALT) {
            TOGGLE(TABS_TO_SPACES);
            show_toggle_statusmsg(TABS_TO_SPACES);
            refresh_needed = TRUE;
          }
          break;
        }
        case GLFW_KEY_V: {
          /* Paste action. */
          if (mods == GLFW_MOD_CONTROL) {
            const char *string = glfwGetClipboardString(NULL);
            if (string) {
              /* Free the cutbuffer if there is anything in it. */
              free_lines(cutbuffer);
              cutbuffer = NULL;
              Ulong linenum;
              char **line_strings = split_string_nano(string, '\n', &linenum);
              if (line_strings) {
                linestruct *item = make_new_node(NULL);
                linestruct *head = item;
                for (Ulong i = 0; i < linenum; ++i) {
                  item->data = line_strings[i];
                  recode_NUL_to_LF(item->data, strlen(item->data));
                  if ((i + 1) < linenum) {
                    item->next = make_new_node(item);
                    item = item->next;
                  }
                }
                free(line_strings);
                cutbuffer = head;
                function = paste_text;
              }
            }
          }
          break;
        }
        case GLFW_KEY_X: {
          /* Cut line or marked region. */
          if (mods == GLFW_MOD_CONTROL) {
            function = cut_text;
          }
          break;
        }
        case GLFW_KEY_Y: {
          /* Redo action. */
          if (mods == GLFW_MOD_CONTROL) {
            function = do_redo;
          }
          break;
        }
        case GLFW_KEY_Z: {
          /* Undo action. */
          if (mods == GLFW_MOD_CONTROL) {
            function = do_undo;
          }
          break;
        }
        case GLFW_KEY_ESCAPE: {
          if (menu_get_active()) {
            menu_show(menu_get_active(), FALSE);
          }
          break;
        }
        case GLFW_KEY_RIGHT: {
          if (menu_get_active() && menu_allows_arrow_navigation(menu_get_active()) && !mods) {
            menu_enter_submenu(menu_get_active());
            return;
          }
          switch (mods) {
            /* Alt+Shift+Right.  Switch to the next editor. */
            case (GLFW_MOD_ALT | GLFW_MOD_SHIFT): {
              function = editor_switch_to_next;
              break;
            }
            /* Alt+Right.  Switch to the next buffer inside this editor. */
            case GLFW_MOD_ALT: {
              function = editor_switch_openfile_to_next;
              break;
            }
            /* Ctrl+Shift+Right.  Move to next word with shift held. */
            case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Ctrl+Right.  Move to next word. */
            case GLFW_MOD_CONTROL: {
              function = to_next_word;
              break;
            }
            /* Shift+Right.  Move to the right with shift held. */
            case GLFW_MOD_SHIFT: {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Right.  Move right. */
            case 0: {
              function = do_right;
            }
          }
          break;
        }
        case GLFW_KEY_LEFT: {
          if (menu_get_active() && menu_allows_arrow_navigation(menu_get_active()) && !mods) {
            menu_exit_submenu(menu_get_active());
            return;
          }
          switch (mods) {
            /* Alt+Shift+Left.  Switch to the previous editor. */
            case (GLFW_MOD_ALT | GLFW_MOD_SHIFT): {
              function = editor_switch_to_prev;
              break;
            }
            /* Alt+Left.  Switch to the previous buffer inside this editor. */
            case GLFW_MOD_ALT: {
              function = editor_switch_openfile_to_prev;
              break;
            }
            /* Ctrl+Shift+Left.  Move to prev word with shift held. */
            case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Ctrl+Left.  Move to prev word. */
            case GLFW_MOD_CONTROL: {
              function = to_prev_word;
              break;
            }
            /* Shift+Left.  Move left with shift held. */
            case GLFW_MOD_SHIFT: {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Left.  Move left. */
            case 0: {
              function = do_left;
            }
          }
          break;
        }
        case GLFW_KEY_UP: {
          if (menu_get_active() && !mods) {
            menu_selected_up(menu_get_active());
            return;
          }
          /* Otherwise, handle normaly. */
          switch (mods) {
            /* Move line or lines up. */
            case GLFW_MOD_ALT: {
              function = move_lines_up;
              break;
            }
            /* Moves to prev indent block with shift held. */
            case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Moves to prev indent block. */
            case GLFW_MOD_CONTROL: {
              function = to_prev_block;
              break;
            }
            /* Shift+Up. */
            case GLFW_MOD_SHIFT: {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Move up. */
            case 0: {
              function = do_up;
            }
          }
          break;
        }
        case GLFW_KEY_DOWN: {
          /* If there is an active menu. */
          if (menu_get_active()) {
            switch (mods) {
              case 0: {
                menu_selected_down(menu_get_active());
                return;
              }
            }
          }
          switch (mods) {
            /* Moves line or lines down. */
            case GLFW_MOD_ALT: {
              function = move_lines_down;
              break;
            }
            /* Moves to next block with shift held. */
            case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Moves to next block. */
            case GLFW_MOD_CONTROL: {
              function = to_next_block;
              break;
            }
            /* Shift+Down. */
            case GLFW_MOD_SHIFT: {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Move down. */
            case 0: {
              function = do_down;
              if (GUI_OF->cursor_row == (openeditor->rows - 1)) {
                scrollbar_refresh_needed(openeditor->sb);
              }
            }
          }
          break;
        }
        case GLFW_KEY_BACKSPACE: {
          /* Simple backspace. */
          if (!mods || mods == GLFW_MOD_SHIFT) {
            function = do_backspace;
          }
          /* Backspace+CTRL. */
          else if (mods == GLFW_MOD_CONTROL) {
            function = chop_previous_word;
          }
          break;
        }
        case GLFW_KEY_DELETE: {
          /* Simple delete. */
          if (!mods) {
            function = do_delete;
          }
          /* Delete+CTRL. */
          else if (mods == GLFW_MOD_CONTROL) {
            function = chop_next_word;
          }
          break;
        }
        case GLFW_KEY_ENTER: {
          if (menu_get_active()) {
            switch (mods) {
              case 0: {
                menu_accept_action(menu_get_active());
                return;
              }
            }
          }
          /* Simple enter. */
          if (!mods) {
            function = do_enter;
          }
          /* Add a new line below the current line no matter where in the line we currently are. */
          else if (mods == GLFW_MOD_CONTROL) {
            function = do_insert_empty_line_below;
          }
          /* Add a new line above current line no matter where in the line we currently are. */
          else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)) {
            function = do_insert_empty_line_above;
          }
          break;
        }
        case GLFW_KEY_TAB: {
          /* If there is an active menu, and that menu accepts on tab. */
          if (menu_get_active() && menu_should_accept_on_tab(menu_get_active())) {
            switch (mods) {
              case 0: {
                menu_accept_action(menu_get_active());
                return;
              }
            }
          }
          /* Simple tab. */
          if (!mods) {
            function = do_tab;
          }
          /* Tab+Shift. */
          else if (mods == GLFW_MOD_SHIFT) {
            function = do_unindent;
          }
          break;
        }
        case GLFW_KEY_HOME: {
          switch (mods) {
            /* Move to the first line in the file with shift held. */
            case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Move to the first line in the file. */
            case GLFW_MOD_CONTROL: {
              function = to_first_line;
              break;
            }
            /* Move to home of current line while shift is held. */
            case GLFW_MOD_SHIFT: {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Simple move to home of the current line. */
            case 0: {
              function = do_home;
            }
          }
          break;
        }
        case GLFW_KEY_END: {
          switch (mods) {
            /* Move to the last line in the file with shift held. */
            case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Move to the last line in the file. */
            case GLFW_MOD_CONTROL: {
              function = to_last_line;
              break;
            }
            /* Shift was held while moving to end of line. */
            case GLFW_MOD_SHIFT: {
              shift_held = TRUE;
              _FALLTHROUGH;
            }
            /* Simple move to end of line. */
            case 0: {
              function = do_end;
            }
          }
          break;
        }
        case GLFW_KEY_SLASH: {
          /* Comment action. */
          if (mods == GLFW_MOD_CONTROL) {
            function = do_comment;
          }
          break;
        }
        case GLFW_KEY_MINUS: {
          /* Decrease the font size. */
          if (mods == GLFW_MOD_CONTROL) {
            gui_font_decrease_size(textfont);
            statusline(AHEM, "Font size: %u", gui_font_get_size(textfont));
            editor_update_all();
            refresh_needed = TRUE;
          }
          /* Decrease the font line height. */
          else if (mods == (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) {
            gui_font_decrease_line_height(textfont);
            statusline(AHEM, "Font line height: %ld", gui_font_get_line_height(textfont));
            editor_update_all();
            refresh_needed = TRUE;
          }
          break;
        }
        case GLFW_KEY_EQUAL: {
          /* increase the font size. */
          if (mods == GLFW_MOD_CONTROL) {
            gui_font_increase_size(textfont);
            statusline(AHEM, "Font size: %u", gui_font_get_size(textfont));
            editor_update_all();
            refresh_needed = TRUE;
          }
          /* increase the font line height. */
          else if (mods == (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) {
            gui_font_increase_line_height(textfont);
            statusline(AHEM, "Font line height: %ld", gui_font_get_line_height(textfont));
            editor_update_all();
            refresh_needed = TRUE;
          }
          break;
        }
        case GLFW_KEY_F11: {
          /* Fullscreen toggle. */
          if (!mods) {
            do_fullscreen(window);
          }
          break;
        }
      }
    }
    if (function != do_cycle) {
      cycling_aim = 0;
    }
    if (!function) {
      pletion_line = NULL;
      keep_cutbuffer = FALSE;
      return;
    }
    /* When in restricted mode dont execute the function. */
    if (ISSET(VIEW_MODE) && changes_something(function)) {
      return;
    }
    /* When not cutting or copying text, drop the cutbuffer the next time. */
    if (function != cut_text && function != copy_text) {
      if (function != zap_text) {
        keep_cutbuffer = FALSE;
      }
    }
    if (function != complete_a_word) {
      pletion_line = NULL;
    }
    linestruct *was_current = GUI_OF->current;
    Ulong was_x = GUI_OF->current_x;
    /* If shifted movement occurs, set the mark. */
    if (shift_held && !GUI_OF->mark) {
      GUI_OF->mark     = GUI_OF->current;
      GUI_OF->mark_x   = GUI_OF->current_x;
      GUI_OF->softmark = TRUE;
    }
    /* Execute the function. */
    function();
    /* When the marked region changes without Shift being held, discard a soft mark. And when the set of lines changes, reset the "last line too" flag. */
    if (GUI_OF->mark && GUI_OF->softmark && !shift_held && (GUI_OF->current != was_current || GUI_OF->current_x != was_x || wanted_to_move(function)) && !keep_mark) {
      GUI_OF->mark = NULL;
    }
    /* Adjust the viewport if we move to the last or first line. */
    if (function == to_first_line || function == to_last_line) {
      if (current_is_offscreen()) {
        adjust_viewport((focusing || ISSET(JUMPY_SCROLLING)) ? CENTERING : FLOWING);
      }
    }
    else if (GUI_OF->current != was_current) {
      also_the_last = FALSE;
    }
    /* When we have moved or changed something, tell the openeditor it needs to update the scrollbar. */
    if (wanted_to_move(function) || changes_something(function) || function == do_undo || function == do_redo) {
      scrollbar_refresh_needed(openeditor->sb);
    }
    /* If we are already showing suggestions, then update them. */
    if (menu_len(gui->suggestmenu->menu)) {
      gui_suggestmenu_run();
    }
    last_key_was_bracket = FALSE;
    last_bracket_char = '\0';
    keep_mark = FALSE;
    refresh_needed = TRUE;
  }
}

/* Char input callback. */
void char_callback(GLFWwindow *window, Uint ch) {
  int len;
  char mb[MAXCHARLEN + 1];
  char input;
  /* When in the gui prompt mode. */
  if (gui->flag.is_set<GUI_PROMPT>()) {
    input = (char)ch;
    gui_promptmenu_char_action(input);
  }
  /* Otherwise, when we are inside gui editor mode. */
  else if (gui->flag.is_set<GUI_EDITOR_MODE_KEY>()) {
    input = (char)ch;
    /* Open a new seperate editor. */
    if (is_char_one_of(&input, 0, "Nn")) {
      editor_create(TRUE);
      editor_hide(openeditor->prev, TRUE);
      editor_redecorate(openeditor);
      editor_resize(openeditor);
    }
    gui->flag.unset<GUI_EDITOR_MODE_KEY>();
  }
  else {
    /* Not a valid utf-8 codepoint. */
    if (!(len = widetomb(ch, mb))) {
      return;
    }
    /* Non ascii codepoint. */
    else if (len != 1) {
      inject_into_buffer(GUI_CTX, mb, len);
      return;
    }
    else {
      input = *mb;
    }
    /* Adjust the viewport if the cursor is offscreen. */
    if (current_is_offscreen()) {
      adjust_viewport(CENTERING);
    }
    /* If we are in restricted move dont add anything to the editor. */
    if (ISSET(VIEW_MODE)) {
      return;
    }
    /* If region is marked, and 'input' is an enclose char, then we enclose the marked region with that char. */
    if (GUI_OF->mark && is_enclose_char((char)ch)) {
      const char *s1, *s2;
      input == '"' ? s1 = "\"", s2 = s1 : input == '\'' ? s1 = "'", s2 = s1 : input == '(' ? s1 = "(", s2 = ")" :
      input == '{' ? s1 = "{", s2 = "}" : input == '[' ? s1 = "[", s2 = "]" : input == '<' ? s1 = "<", s2 = ">" : 0;
      enclose_marked_region(s1, s2);
      return;
    }
    else if (GUI_OF->mark && GUI_OF->softmark) {
      zap_replace_text(&input, 1);
      keep_mark            = FALSE;
      last_key_was_bracket = FALSE;
      last_bracket_char    = '\0';
      GUI_OF->mark         = NULL;
      return;
    }
    /* If a enclose char is pressed without a having a marked region, we simply enclose in place. */
    else if (is_enclose_char(input)) {
      /* If quote or double quote was just enclosed in place just move once to the right. */
      if ((input == '"' && last_key_was_bracket && last_bracket_char == '"') || (input == '\'' && last_key_was_bracket && last_bracket_char == '\'')) {
        do_right();
        keep_mark = FALSE;
        last_key_was_bracket = FALSE;
        last_bracket_char = '\0';
        refresh_needed = TRUE;
        return;
      }
      /* Exceptions for enclosing double quotes. */
      else if (input == '"'
       /* Before current cursor position. */
       && (((is_prev_cursor_word_char(FALSE) || is_prev_cursor_char_one_of("\"><")))
       /* After current cursor position. */
       || (!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of(")}]")))) {
        ;
      }
      /* Exceptions for enclosing single quotes. */
      else if (input == '\''
       /* Before current cursor position. */
       && (((is_prev_cursor_word_char(FALSE) || is_prev_cursor_char_one_of("'><")))
       /* After current cursor position. */
       || (!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of(")}]")))) {
        ;
      }
      /* Exceptions for enclosing brackets. */
      else if ((input == '(' || input == '[' || input == '{')
       /* We only allow the insertion of double brackets when the cursor is either at a blank char or at EOL, or at any of the
        * given chars, all other sinarios with other chars at the cursor will result in only the start bracket beeing inserted. */
       && ((!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of("\":;')}],")))) {
        ;
      }
      /* If '<' is pressed without being in a c/cpp file and at an include line, we simply do nothing. */
      else if (input == '<' && GUI_OF->current->data[indentlen(GUI_OF->current->data)] != '#' && (GUI_OF->is_c_file || GUI_OF->is_cxx_file) /* GUI_OF->type.is_set<C_CPP>() */) {
        ;
      }
      else {
        const char *s1, *s2;
        input == '"'  ? s1 = "\"", s2 = s1 :
        input == '\'' ? s1 = "'",  s2 = s1 :
        input == '('  ? s1 = "(",  s2 = ")" :
        input == '{'  ? s1 = "{",  s2 = "}" :
        input == '['  ? s1 = "[",  s2 = "]" :
        input == '<'  ? s1 = "<",  s2 = ">" : 0;
        /* 'Set' the mark, so that 'enclose_marked_region()' dosent exit because there is no marked region. */
        GUI_OF->mark = GUI_OF->current;
        GUI_OF->mark_x = GUI_OF->current_x;
        enclose_marked_region(s1, s2);
        /* Set the flag in the undo struct just created, marking an exception for future undo-redo actions. */
        GUI_OF->undotop->xflags |= SHOULD_NOT_KEEP_MARK;
        GUI_OF->mark = NULL;
        keep_mark      = FALSE;
        /* This flag ensures that if backspace is the next key that is pressed it will erase both of the enclose char`s. */
        last_key_was_bracket = TRUE;
        last_bracket_char = input;
        return;
      }
    }
    inject(&input, 1);
    last_key_was_bracket = FALSE;
    last_bracket_char    = '\0';
    keep_mark            = FALSE;
    refresh_needed       = TRUE;
    gui_suggestmenu_run();
  }
}

/* Mouse button callback. */
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  /* Some static vars to track double clicking. */
  // static double last_time     = 0.0;
  // static int    last_button   = 0;
  // static vec2   last_mousepos = 0.0f;
  Element *test;
  update_mouse_state(action, button);
  /* Determen if this was a double or tripple click. */
  // if (action == GLFW_PRESS) {
  //   /* Give some wiggle room for the position of the mouse click as it is represented as while numbers only. */
  //   if (last_button == button && (glfwGetTime() - last_time) < DOUBLE_PRESS_THRESHOLD && mousepos > (last_mousepos - 3.0f) && mousepos < (last_mousepos + 3.0f)) {
  //     /* Check tripple press first. */
  //     (is_mouse_flag_set(MOUSE_PRESS_WAS_DOUBLE) && !is_mouse_flag_set(MOUSE_PRESS_WAS_TRIPPLE))
  //       ? mouse_flag.set<WAS_MOUSE_TRIPPLE_PRESS>() : mouse_flag.unset<WAS_MOUSE_TRIPPLE_PRESS>();
  //     /* Then check for double press. */
  //     (!is_mouse_flag_set(MOUSE_PRESS_WAS_DOUBLE) && !is_mouse_flag_set(MOUSE_PRESS_WAS_TRIPPLE))
  //       ? mouse_flag.set<WAS_MOUSE_DOUBLE_PRESS>() : mouse_flag.unset<WAS_MOUSE_DOUBLE_PRESS>();
  //   }
  //   /* Otherwise, clear both double and tripple flags. */
  //   else {
  //     mouse_flag.unset<WAS_MOUSE_DOUBLE_PRESS>();
  //     mouse_flag.unset<WAS_MOUSE_TRIPPLE_PRESS>();
  //   }
  // }
  /* Left mouse button. */
  if (button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS) {
      /* Indicate that the left mouse button is currently held. */
      // mouse_flag.set<LEFT_MOUSE_BUTTON_HELD>();
      /* When in prompt-mode. */
      if (gui->flag.is_set<GUI_PROMPT>()) {
        long row;
        /* When the mouse y position is inside the gui promptmenu element. */
        if (gui_font_row_from_pos(uifont, gui->promptmenu->element->y, (gui->promptmenu->element->y + gui->promptmenu->element->height), get_mouse_ypos(), &row)) {
          if (row == 0) {
            typing_x = gui_font_index_from_pos(uifont, answer, strlen(answer), get_mouse_xpos(), (gui->promptmenu->element->x + font_breadth(uifont, " ") + font_breadth(uifont, prompt)));
            gui->promptmenu->text_refresh_needed = TRUE;
          }
        }
        /* Otherwise, when outside the promptmenu element, we should leave promptmode. */
        else {
          gui_promptmode_leave();
        }
      }
      test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
      // test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
      if (test) {
        gui->clicked = test;
        if (menu_get_active() && !menu_owns_element(menu_get_active(), test)) {
          menu_show(menu_get_active(), FALSE);
        }
        else if (menu_get_active() && test->dt == ELEMENT_DATA_MENU && menu_is_ancestor(test->dp_menu, menu_get_active()) && menu_element_is_main(test->dp_menu, test)) {
          menu_click_action(test->dp_menu, get_mouse_xpos(), get_mouse_ypos());
        }
        else if (test->dt == ELEMENT_DATA_EDITOR && test == test->dp_editor->text) {
          /* When a click occurs in the text element of a editor, make that editor the currently active editor. */
          editor_set_open(test->dp_editor);
          /* Get the line and index from the mouse position. */
          editor_get_text_line_index(test->dp_editor, get_mouse_xpos(), get_mouse_ypos(), &GUI_OF->current, &GUI_OF->current_x);
          GUI_OF->mark     = GUI_OF->current;
          GUI_OF->mark_x   = GUI_OF->current_x;
          GUI_OF->softmark = TRUE;
          SET_PWW(GUI_OF);
          /* If this was a double click then select the current word, if any. */
          if (is_mouse_flag_set(MOUSE_PRESS_WAS_DOUBLE)) {
            Ulong st, end;
            st  = get_prev_cursor_word_start_index(TRUE);
            end = get_current_cursor_word_end_index(TRUE);
            /* If the double click was inside of a word. */
            if (st != GUI_OF->current_x && end != GUI_OF->current_x) {
              GUI_OF->mark_x    = st;
              GUI_OF->current_x = end;
            }
            /* The double click was done at the end of the word.  So we select that word. */
            else if (st != GUI_OF->current_x && end == GUI_OF->current_x) {
              GUI_OF->mark_x = st;
            }
            /* The double click was done at the begining of the word.  So we select that word. */
            else if (st == GUI_OF->current_x && end != GUI_OF->current_x) {
              GUI_OF->mark_x    = st;
              GUI_OF->current_x = end;
            }
          }
          /* Otherwise, if this was a tripple click then select the whole line. */
          else if (is_mouse_flag_set(MOUSE_PRESS_WAS_TRIPPLE)) {
            GUI_OF->mark_x = 0;
            GUI_OF->current_x = strlen(GUI_OF->current->data);
          }
        }
        else if (test->dt == ELEMENT_DATA_FILE && test->parent && test->parent->dt == ELEMENT_DATA_EDITOR && etb_element_is_main(test->parent->dp_editor->tb, test->parent) && !gui->flag.is_set<GUI_PROMPT>()) {
          if (test->dp_file && test->dp_file != test->parent->dp_editor->openfile) {
            test->parent->dp_editor->openfile = test->dp_file;
            editor_redecorate(test->parent->dp_editor);
            editor_resize(test->parent->dp_editor);
            etb_active_refresh_needed(test->parent->dp_editor->tb);
          }
        }
        else if (test == gui->promptmenu->element) {
          gui_promptmenu_click_action(get_mouse_ypos());
        }
      }
    }
    else if (action == GLFW_RELEASE) {
      // mouse_flag.unset<LEFT_MOUSE_BUTTON_HELD>();
      /* If when the left mouse button is relesed without moving, set the mark to NULL. */
      if (GUI_OF->mark == GUI_OF->current && GUI_OF->mark_x == GUI_OF->current_x) {
        GUI_OF->mark = NULL;
      }
      if (gui->clicked && gui->clicked->dt == ELEMENT_DATA_SB) {
        if (scrollbar_element_is_thumb(gui->clicked->dp_sb, gui->clicked) && gui->clicked != element_grid_get(get_mouse_xpos(), get_mouse_ypos())) {
          scrollbar_set_thumb_color(gui->clicked->dp_sb, FALSE);
        }
        scrollbar_refresh_needed(gui->clicked->dp_sb);
      }
      gui->clicked = NULL;
    }
  }
  /* Right mouse button. */
  else if (button == GLFW_MOUSE_BUTTON_2) {
    if (action == GLFW_PRESS) {
      // mouse_flag.set<RIGHT_MOUSE_BUTTON_HELD>();
      // test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
      test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
      if (test && test->dt == ELEMENT_DATA_FILE && test->parent && test->parent->dt == ELEMENT_DATA_EDITOR && etb_element_is_main(test->parent->dp_editor->tb, test->parent)) {
        etb_show_context_menu(test->parent->dp_editor->tb, test, TRUE);
      }
      else if (test && test->dt == ELEMENT_DATA_EDITOR && etb_element_is_main(test->dp_editor->tb, test)) {
        etb_show_context_menu(test->dp_editor->tb, test, TRUE);
      }
      else if (test && test->dt == ELEMENT_DATA_MENU) {
        menu_show(test->dp_menu, FALSE);
      }
      else {
        context_menu_show(gui->context_menu, TRUE);
      }
    }
    else if (action == GLFW_RELEASE) {
      // mouse_flag.unset<RIGHT_MOUSE_BUTTON_HELD>();
    }
  }
  /* Middle mouse button. */
  else if (button == GLFW_MOUSE_BUTTON_3) {
    NETLOG("Mouse button 3 pressed.\n");
  }
  else if (button == GLFW_MOUSE_BUTTON_4) {
    NETLOG("Mouse button 4 pressed.\n");
  }
  else if (button == GLFW_MOUSE_BUTTON_5) {
    NETLOG("Mouse button 5 pressed.\n");
  }
  else if (button == GLFW_MOUSE_BUTTON_6) {
    NETLOG("Mouse button 6 pressed.\n");
  }
  else if (button == GLFW_MOUSE_BUTTON_7) {
    NETLOG("Mouse button 7 pressed.\n");
  }
  else if (button == GLFW_MOUSE_BUTTON_8) {
    NETLOG("Mouse button 8 pressed.\n");
  }
  /* Set the static vars for the next callback.  But only do this for actual singular presses. */
  // if (action == GLFW_PRESS) {
  //   /* If we just had a tripple click ensure that the next press will not be detected as a double press. */
  //   last_time     = (is_mouse_flag_set(MOUSE_PRESS_WAS_TRIPPLE) ? 0.0 : glfwGetTime());
  //   last_button   = button;
  //   last_mousepos = mousepos;
  // }
  refresh_needed = TRUE;
}

/* Mouse pos callback. */
void mouse_pos_callback(GLFWwindow *window, double x, double y) {
  // static vec2 last_mousepos = mousepos;
  // guielement *element;
  Element *test;
  update_mouse_pos(x, y);
  /* Set the global mouse position. */
  // mousepos = vec2(x, y);
  // get_mouse_xpos() = x;
  // get_mouse_ypos() = y;
  /* If the left mouse button is being held. */
  // if (mouse_flag.is_set<LEFT_MOUSE_BUTTON_HELD>()) {
  if (is_mouse_flag_set(MOUSE_BUTTON_HELD_LEFT)) {
    /* When inside prompt mode. */
    if (gui->flag.is_set<GUI_PROMPT>()) {
      long index = prompt_index_from_mouse(TRUE);
      if (index != -1) {
        typing_x = index;
        gui->promptmenu->text_refresh_needed = TRUE;
      }
    }
    if (gui->clicked) {
      /* Scrollbar */
      if (gui->clicked->dt == ELEMENT_DATA_SB) {
        // scrollbar_mouse_pos_routine(gui->clicked->dp_sb, gui->clicked, last_mousepos.y, get_mouse_ypos());
        scrollbar_mouse_pos_routine(gui->clicked->dp_sb, gui->clicked, get_last_mouse_ypos(), get_mouse_ypos());
      }
      /* Editor-Text */
      else if (gui->clicked->dt == ELEMENT_DATA_EDITOR && gui->clicked == gui->clicked->dp_editor->text) {
        // editor_get_text_line_index(gui->clicked->dp_editor, get_mouse_xpos(), get_mouse_ypos(), &GUI_OF->current, &GUI_OF->current_x);
        editor_get_text_line_index(gui->clicked->dp_editor, get_mouse_xpos(), get_mouse_ypos(), &GUI_OF->current, &GUI_OF->current_x);
        /* If the left press was a tripple press, adjust the mark based on the current cursor line. */
        // if (is_mouse_flag_set(MOUSE_PRESS_WAS_TRIPPLE)) {
        if (is_mouse_flag_set(MOUSE_PRESS_WAS_TRIPPLE)) {
          Ulong st, end;
          st  = 0;
          end = strlen(GUI_OF->mark->data);
          /* If the current cursor line is the same as mark, mark the entire line no matter the cursor pos inside it. */
          if (GUI_OF->current == GUI_OF->mark) {
            GUI_OF->mark_x    = st;
            GUI_OF->current_x = end;
          }
          /* If the current line is above the line where the mark was placed, set the mark index at the end of the line. */
          if (GUI_OF->current->lineno < GUI_OF->mark->lineno) {
            GUI_OF->mark_x = end;
          }
          /* Otherwise, place it at the start. */
          else {
            GUI_OF->mark_x = st;
          }
        }
        /* If the left press was a double press. */
        // else if (is_mouse_flag_set(MOUSE_PRESS_WAS_DOUBLE)) {
        else if (is_mouse_flag_set(MOUSE_PRESS_WAS_DOUBLE)) {
          /* Get the start and end of the word that has been marked.  It does not matter is the mark is currently at start or end of that word. */
          Ulong st, end;
          st  = get_prev_word_start_index(GUI_OF->mark->data, GUI_OF->mark_x, TRUE);
          end = get_current_word_end_index(GUI_OF->mark->data, GUI_OF->mark_x, TRUE);
          /* If the current line is the same as the mark line. */
          if (GUI_OF->current == GUI_OF->mark) {
            /* When the current cursor pos is inside the the word, just mark the entire word. */
            if (GUI_OF->current_x >= st && GUI_OF->current_x <= end) {
              GUI_OF->mark_x    = st;
              GUI_OF->current_x = end;
            }
            /* Or, when the cursor if before the word, set the mark at the end of the word. */
            else if (GUI_OF->current_x < st) {
              GUI_OF->mark_x = end;
            }
            /* And when the cursor is after the word, place the mark at the start. */
            else if (GUI_OF->current_x > end) {
              GUI_OF->mark_x = st;
            }
          }
          /* Otherwise, when the cursor line is above the mark, place the mark at the end of the word. */
          else if (GUI_OF->current->lineno < GUI_OF->mark->lineno) {
            GUI_OF->mark_x = end;
          }
          /* And when the cursor line is below the mark line, place the mark at the start of the word. */
          else {
            GUI_OF->mark_x = st;
          }
          GUI_OF->placewewant = xplustabs();
        }
      }
    }
  }
  else {
    /* Get the element that the mouse is on. */
    // test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
    test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
    if (test) {
      /* If the element currently hovered on has a diffrent cursor then the active one, change it. */
      if (test->cursor != gui->current_cursor_type) {
        set_cursor_type(window, test->cursor);
        gui->current_cursor_type = test->cursor;
      }
      if (!gui->clicked && menu_get_active() && test->dt == ELEMENT_DATA_MENU && menu_is_ancestor(test->dp_menu, menu_get_active()) && menu_element_is_main(test->dp_menu, test)) {
        menu_hover_action(test->dp_menu, get_mouse_xpos(), get_mouse_ypos());
      }
      else if (!gui->clicked && test == gui->promptmenu->element) {
        gui_promptmenu_hover_action(get_mouse_ypos());
      }
      if (!entered_element || test != entered_element) {
        /* If there is no clicked element currently and the newly entered element is the thumb of any scrollbar, highlight the color of it. */
        if (!gui->clicked && test->dt == ELEMENT_DATA_SB && scrollbar_element_is_thumb(test->dp_sb, test)) {
          scrollbar_set_thumb_color(test->dp_sb, TRUE);
        }
        /* If the old entered element was not NULL. */
        if (entered_element) {
          /* If there is no clicked element currently and the recently left element is the thumb of any scrollbar, reset the color of it. */
          if (!gui->clicked && entered_element->dt == ELEMENT_DATA_SB && scrollbar_element_is_thumb(entered_element->dp_sb, entered_element)) {
            scrollbar_set_thumb_color(entered_element->dp_sb, FALSE);
          }
        }
        entered_element = test;
      }
    }
    /* In all other cases reset the cursor type to the arrow when its not the current cursor. */
    else if (gui->current_cursor_type != GLFW_ARROW_CURSOR) {
      set_cursor_type(window, GLFW_ARROW_CURSOR);
      gui->current_cursor_type = GLFW_ARROW_CURSOR;
    }
  }
  /* Save the current mouse position. */
  // last_mousepos = mousepos;
  refresh_needed = TRUE;
}

/* Window entering and leaving callback. */
void window_enter_callback(GLFWwindow *window, int entered) {
  float x;
  float y;
  if (!entered) {
    x = get_mouse_xpos();
    y = get_mouse_ypos();
    if (x <= ((float)gui->width / 2)) {
      x = -30.0f;
    }
    else if (x >= ((float)gui->width / 2)) {
      x = (gui->width + 30.0f);
    }
    if (y <= ((float)gui->height / 2)) {
      y = -30.0f;
    }
    else if (y >= ((float)gui->height / 2)) {
      y = (gui->height + 30.0f);
    }
    update_mouse_pos(x, y);
    /* If there is a currently entered element, call its leave callback.  If it has one. */
    if (entered_element) {
      /* Reset the color of any scrollbar if it was the last element the mouse was on and its not currently clicked. */
      if (!gui->clicked && entered_element->dt == ELEMENT_DATA_SB && scrollbar_element_is_thumb(entered_element->dp_sb, entered_element)) {
        scrollbar_set_thumb_color(entered_element->dp_sb, FALSE);
      }
      entered_element = NULL;
    }
  }
}

/* Scroll callback. */
void scroll_callback(GLFWwindow *window, double x, double y) {
  Element *test = element_grid_get(get_mouse_xpos(), get_mouse_ypos());
  Ulong index;
  linestruct *line;
  if (test) {
    if (test->dt == ELEMENT_DATA_EDITOR && test == test->dp_editor->text) {
      /* If the mouse left mouse button is held while scrolling, update the cursor pos so that the marked region gets updated. */
      // if (mouse_flag.is_set<LEFT_MOUSE_BUTTON_HELD>()) {
      if (is_mouse_flag_set(MOUSE_BUTTON_HELD_LEFT)) {
        line = line_and_index_from_mousepos(gui_font_get_font(textfont), &index);
        test->dp_editor->openfile->current   = line;
        test->dp_editor->openfile->current_x = index;
      }
      /* Up and down scroll. */
      edit_scroll((y > 0.0) ? BACKWARD : FORWARD);
      refresh_needed = TRUE;
      scrollbar_refresh_needed(test->dp_editor->sb);
      /* If the suggestmenu is active then make sure it updates the position and text. */
      if (menu_len(gui->suggestmenu->menu)) {
        menu_text_refresh_needed(gui->suggestmenu->menu);
        menu_pos_refresh_needed(gui->suggestmenu->menu);
      }
    }
    if (menu_get_active() && test->dt == ELEMENT_DATA_MENU && menu_is_ancestor(test->dp_menu, menu_get_active()) && menu_element_is_main(test->dp_menu, test)) {
      menu_scroll_action(test->dp_menu, ((y > 0) ? BACKWARD : FORWARD), get_mouse_xpos(), get_mouse_ypos());
    }
    /* If this element is the gui promptmenu main element.  Then call the scroll function. */
    else if (test == gui->promptmenu->element) {
      gui_promptmenu_scroll_action(((y > 0) ? BACKWARD : FORWARD), get_mouse_ypos());
    }
  }
}

#endif
