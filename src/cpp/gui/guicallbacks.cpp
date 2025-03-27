/** @file guicallbacks.cpp

  @author  Melwin Svensson.
  @date    2-1-2025.

 */
#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

#define DOUBLE_PRESS_THRESHOLD 0.2

typedef enum {
  #define MOUSEFLAG_SIZE 8
  LEFT_MOUSE_BUTTON_HELD,
  #define LEFT_MOUSE_BUTTON_HELD LEFT_MOUSE_BUTTON_HELD
  RIGHT_MOUSE_BUTTON_HELD,
  #define RIGHT_MOUSE_BUTTON_HELD RIGHT_MOUSE_BUTTON_HELD
  WAS_MOUSE_DOUBLE_PRESS,
  #define WAS_MOUSE_DOUBLE_PRESS WAS_MOUSE_DOUBLE_PRESS
  WAS_MOUSE_TRIPPLE_PRESS,
  #define WAS_MOUSE_TRIPPLE_PRESS WAS_MOUSE_TRIPPLE_PRESS
  WAS_EDITOR_TEXT_PRESS,
  #define WAS_EDITOR_TEXT_PRESS WAS_EDITOR_TEXT_PRESS
  WAS_EDITOR_SCROLL_PRESS,
  #define WAS_EDITOR_SCROLL_PRESS WAS_EDITOR_SCROLL_PRESS
} mouseflag_type;

/* Flags to represent what the mouse is currently doing, across callback functions. */
static bit_flag_t<MOUSEFLAG_SIZE> mouse_flag;
/* The element that was last entered, and witch element just ran its enter callback. */
static guielement *entered_element = NULL;
/* The mouse position. */
vec2 mousepos;

SyntaxFile *sf = NULL;

/* Window resize callback. */
void window_resize_callback(GLFWwindow *window, int width, int height) {
  ASSERT_MSG(gui, "gui has not been init.");
  gui->width  = width;
  gui->height = height;
  /* Set viewport. */
  glViewport(0, 0, gui->width, gui->height);
  /* Set the projection. */
  matrix4x4_set_orthographic(gui->projection, 0, gui->width, gui->height, 0, -1.0f, 1.0f);
  /* Upload it to both the shaders. */
  update_projection_uniform(gui->font_shader);
  update_projection_uniform(gui->rect_shader);
  /* Calculate the rows and columns. */
  editwinrows = (openeditor->text->size.h / FONT_HEIGHT(gui->font));
  /* If the font is a mono font then calculate the number of columns by the gui->width of ' '. */
  if (texture_font_is_mono(gui->font)) {
    texture_glyph_t *glyph = texture_font_get_glyph(gui->font, " ");
    if (glyph == 0) {
      die("%s: Atlas is not big egnofe.\n", __func__);
    }
    editwincols = (openeditor->text->size.w / glyph->advance_x);
  }
  /* Otherwise, just guess for now. */
  else {
    editwincols = ((openeditor->text->size.w / FONT_WIDTH(gui->font)) * 0.9f);
  }
  refresh_needed = TRUE;
  resize_element(gui->root, vec2(gui->width, gui->height));
  /* Calculate the rows for all editors. */
  ITER_OVER_ALL_OPENEDITORS(starteditor, editor,
    move_resize_element(editor->main, vec2(0, (gui->topbar->pos.y + gui->topbar->size.h)), vec2(gui->width, (gui->height - (gui->topbar->pos.y + gui->topbar->size.h) - gui->botbar->size.h)));
    editor->rows = (editor->text->size.h / FONT_HEIGHT(gui->font));
    if (texture_font_is_mono(gui->font)) {
      texture_glyph_t *glyph = texture_font_get_glyph(gui->font, " ");
      if (glyph == 0) {
        die("%s: Atlas is to small.\n", __func__);
      }
      editor->cols = (editor->text->size.w / glyph->advance_x);
    }
    else {
      editor->cols = ((editor->text->size.w / FONT_WIDTH(gui->font)) * 0.9f);
    }
    editor->flag.set<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>();
  );
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
  // ASSERT_WHOLE_CIRCULAR_LIST(guieditor *, openeditor);
  // ASSERT_WHOLE_CIRCULAR_LIST(openfilestruct *, openeditor->openfile);
  /* Key-callbacks for when we are when inside the prompt-mode. */
  if (gui->flag.is_set<GUI_PROMPT>()) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      switch (key) {
        case GLFW_KEY_ENTER: {
          if (!mods) {
            if (gui_prompt_type == GUI_PROMPT_SAVEFILE) {
              if (*answer) {
                /* Get the full path to of the answer. */
                char *full_path = get_full_path(answer);
                /* If the currently open file has a name, and that name does not match the answer. */
                if (*openfile->filename && strcmp(answer, openfile->filename) != 0) {
                  /* When the given path does not exist. */
                  if (!full_path) {
                    NETLOG("Save file using diffrent name?\n");
                  }
                  else {
                    NETLOG("Overwrite file: '%s'?\n", answer);
                  }
                }
                /* Otherwise if the current file has no name, then just save using the answer. */
                else {
                  show_statusmsg(INFO, 5, "Saving file: %s", answer);
                  /* Free the openfile filename, and assign answer to it. */
                  openfile->filename = free_and_assign(openfile->filename, copy_of(answer));
                  /* Then save the file. */
                  if (write_it_out(FALSE, FALSE) == 2) {
                    logE("Failed to save file, this needs fixing and the reason needs to be found out.");
                    close_and_go();
                  }
                  gui_leave_prompt_mode();
                }
                free(full_path);
              }
            }
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
          statusbar_discard_all_undo_redo();
          gui_leave_prompt_mode();
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
          }
          else if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_chop_prev_word();
          }
          break;
        }
        case GLFW_KEY_DELETE: {
          if (!mods) {
            do_statusbar_delete();
          }
          else if (mods == GLFW_MOD_CONTROL) {
            do_statusbar_chop_next_word();
          }
          break;
        }
      }
      refresh_needed = TRUE;
    }
  }
  /* When inside the guieditor key mode, do nothing as we handle further keys in the char callback. */
  else if (gui->flag.is_set<GUI_EDITOR_MODE_KEY>()) {
    ;
  }
  /* Otherwise, do the text bindings. */
  else {
    mouse_flag.clear();
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
            if (openfile->type.is_set<C_CPP>() || openfile->type.is_set<GLSL>()) {
              function = do_block_comment;
            }
          }
          /* Ctrl+A.  Mark the entire file. */
          else if (mods == GLFW_MOD_CONTROL) {
            openfile->mark      = openfile->filetop;
            openfile->mark_x    = 0;
            openfile->current   = openfile->filebot;
            openfile->current_x = strlen(openfile->filebot->data);
            openfile->softmark  = TRUE;
            refresh_needed      = TRUE;
          }
          break;
        }
        case GLFW_KEY_E: {
          if (mods == GLFW_MOD_CONTROL) {
            gui->flag.set<GUI_EDITOR_MODE_KEY>();
          }
          break;
        }
        case GLFW_KEY_Q: {
          /* If CTRL+Q is pressed, quit. */
          if (mods == GLFW_MOD_CONTROL) {
            if (!openfile->modified || ISSET(VIEW_MODE)) {
              if (gui_close_and_go()) {
                glfwSetWindowShouldClose(window, TRUE);
              }
            }
            else if (ISSET(SAVE_ON_EXIT) && *openfile->filename) {
              ;
            }
            else {
              gui_ask_user("Close without saving? ", GUI_PROMPT_EXIT_NO_SAVE);
            }
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
        case GLFW_KEY_N: {
          /* Line numbers toggle. */
          if (action == GLFW_PRESS && mods == GLFW_MOD_ALT) {
            TOGGLE(LINE_NUMBERS);
            confirm_margin();
            window_resize_callback(window, gui->width, gui->height);
            show_toggle_statusmsg(LINE_NUMBERS);
          }
          else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)) {
            make_new_editor(TRUE);
            gui_redecorate_after_switch();
          }
          else if (mods == GLFW_MOD_CONTROL) {
            gui_open_new_empty_buffer();
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
        case GLFW_KEY_Y: {
          /* Redo action. */
          if (mods == GLFW_MOD_CONTROL) {
            function = do_redo;
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
        case GLFW_KEY_C: {
          /* Copy action.  TODO: Make this more complete. */
          if (mods == GLFW_MOD_CONTROL) {
            if (openfile->mark) {
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
              char *string = fmtstr("%s\n", openfile->current->data);
              glfwSetClipboardString(window, string);
              free(string);
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
        case GLFW_KEY_S: {
          /* Saving action. */
          if (mods == GLFW_MOD_CONTROL) {
            /* True if the current file has a name. */
            bool has_name = *openfile->filename;
            /* If the openfile has a name, and if the file already exists. */
            if (has_name && is_file_and_exists(openfile->filename)) {
              function = do_savefile;
            }
            /* Otherwise, ask if the user wants to save the file. */
            else {
              gui_ask_user("Save file", GUI_PROMPT_SAVEFILE);
              /* If the file has a name but is not a file yet then add that name to the answer. */
              if (has_name) {
                answer   = copy_of(openfile->filename);
                typing_x = strlen(answer);
              }
              return;
            }
          }
          /* Softwrap toggle. */
          else if (action == GLFW_PRESS && mods == GLFW_MOD_ALT) {
            TOGGLE(SOFTWRAP);
            if (!ISSET(SOFTWRAP)) {
              openfile->firstcolumn = 0;
            }
            show_toggle_statusmsg(SOFTWRAP);
            refresh_needed = TRUE;
          }
          break;
        }
        /* Debuging. */
        case GLFW_KEY_P: {
          if (mods == GLFW_MOD_CONTROL) {
            if (sf) {
              syntaxfile_free(sf);
            }
            sf = syntaxfile_create();
            syntaxfile_read(sf, openfile->filename);
            syntaxfile_parse_csyntax(sf);
            for (SyntaxFileError *err = sf->errtop; err; err = err->next) {
              printf("%s:[%d:%d]: %s\n", err->file, err->pos->row, err->pos->column, err->msg);
            }
            hashmap_forall(sf->objects, [](const char *const restrict path, void *arg) {
              writef("%s\n", path);
            });
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
            static int toggle=0;
            if (!(toggle % 2)) {
              set_gui_font("/usr/share/fonts/balle.ttf", gui->font_size);
            }
            else {
              set_gui_font("/usr/share/fonts/TTF/Hack-Regular.ttf", gui->font_size);
            }
            ++toggle;
            refresh_needed = TRUE;
          }
          break;
        }
        case GLFW_KEY_RIGHT: {
          switch (mods) {
            /* Alt+Shift+Right.  Switch to the next editor. */
            case (GLFW_MOD_ALT | GLFW_MOD_SHIFT): {
              function = switch_to_next_editor;
              break;
            }
            /* Alt+Right.  Switch to the next buffer inside this editor. */
            case GLFW_MOD_ALT: {
              function = gui_switch_to_next_buffer;
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
          switch (mods) {
            /* Alt+Shift+Left.  Switch to the previous editor. */
            case (GLFW_MOD_ALT | GLFW_MOD_SHIFT): {
              function = switch_to_prev_editor;
              break;
            }
            /* Alt+Left.  Switch to the previous buffer inside this editor. */
            case GLFW_MOD_ALT: {
              function = gui_switch_to_prev_buffer;
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
              if (openeditor->openfile->cursor_row == (openeditor->rows - 1)) {
                openeditor->flag.set<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>();
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
          if (mods == GLFW_MOD_CONTROL) {
            writef("%s\n", gui->font_path);
            change_gui_font_size(gui->font_size - 1);
            refresh_needed = TRUE;
          }
          break;
        }
        case GLFW_KEY_EQUAL: {
          if (mods == GLFW_MOD_CONTROL) {
            change_gui_font_size(gui->font_size + 1);
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
    linestruct *was_current = openfile->current;
    Ulong was_x = openfile->current_x;
    /* If shifted movement occurs, set the mark. */
    if (shift_held && !openfile->mark) {
      openfile->mark     = openfile->current;
      openfile->mark_x   = openfile->current_x;
      openfile->softmark = TRUE;
    }
    /* Execute the function. */
    function();
    /* When the marked region changes without Shift being held, discard a soft mark. And when the set of lines changes, reset the "last line too" flag. */
    if (openfile->mark && openfile->softmark && !shift_held && (openfile->current != was_current || openfile->current_x != was_x || wanted_to_move(function)) && !keep_mark) {
      openfile->mark = NULL;
    }
    /* Adjust the viewport if we move to the last or first line. */
    if (function == to_first_line || function == to_last_line) {
      if (current_is_offscreen()) {
        adjust_viewport((focusing || ISSET(JUMPY_SCROLLING)) ? CENTERING : FLOWING);
      }
    }
    else if (openfile->current != was_current) {
      also_the_last = FALSE;
    }
    /* When we have moved or changed something, tell the openeditor it needs to update the scrollbar. */
    if (wanted_to_move(function) || changes_something(function) || function == do_undo || function == do_redo) {
      openeditor->flag.set<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>();
    }
    last_key_was_bracket = FALSE;
    last_bracket_char = '\0';
    keep_mark = FALSE;
    refresh_needed = TRUE;
  }
}

/* Char input callback. */
void char_callback(GLFWwindow *window, Uint ch) {
  /* When in the gui prompt mode. */
  if (gui->flag.is_set<GUI_PROMPT>()) {
    char input = (char)ch;
    if (gui_prompt_type == GUI_PROMPT_EXIT_NO_SAVE) {
      if (is_char_one_of(&input, 0, "Yy")) {
        glfwSetWindowShouldClose(window, TRUE);
      }
      else if (is_char_one_of(&input, 0, "Nn")) {
        gui_leave_prompt_mode();
      }
    }
    else {
      inject_into_answer(&input, 1);
    }
    refresh_needed = TRUE;
  }
  /* Otherwise, when we are inside gui editor mode. */
  else if (gui->flag.is_set<GUI_EDITOR_MODE_KEY>()) {
    char input = (char)ch;
    /* Open a new seperate editor. */
    if (is_char_one_of(&input, 0, "Nn")) {
      make_new_editor(TRUE);
      hide_editor(openeditor->prev, TRUE);
      gui_redecorate_after_switch();
    }
    gui->flag.unset<GUI_EDITOR_MODE_KEY>();
  }
  else {
    /* If we are in restricted move dont add anything to the editor. */
    if (ISSET(VIEW_MODE)) {
      return;
    }
    char input = (char)ch;
    /* If region is marked, and 'input' is an enclose char, then we enclose the marked region with that char. */
    if (openfile->mark && is_enclose_char((char)ch)) {
      const char *s1, *s2;
      input == '"' ? s1 = "\"", s2 = s1 : input == '\'' ? s1 = "'", s2 = s1 : input == '(' ? s1 = "(", s2 = ")" :
      input == '{' ? s1 = "{", s2 = "}" : input == '[' ? s1 = "[", s2 = "]" : input == '<' ? s1 = "<", s2 = ">" : 0;
      enclose_marked_region(s1, s2);
      return;
    }
    else if (openfile->mark && openfile->softmark) {
      zap_replace_text(&input, 1);
      keep_mark            = FALSE;
      last_key_was_bracket = FALSE;
      last_bracket_char    = '\0';
      openfile->mark       = NULL;
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
      /* Exceptions for enclosing quotes. */
      else if (input == '"'
       /* Before current cursor position. */
       && (((is_prev_cursor_word_char() || is_prev_cursor_char_one_of("\"><")))
       /* After current cursor position. */
       || (!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of(";)}]")))) {
        ;
      }
      /* Exceptions for enclosing brackets. */
      else if ((input == '(' || input == '[' || input == '{')
       /* After current cursor position. */
       && ((!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of("\";')}]")) || (is_cursor_char_one_of("({[")))) {
        ;
      }
      /* If '<' is pressed without being in a c/cpp file and at an include line, we simply do nothing. */
      else if (input == '<' && openfile->current->data[indent_length(openfile->current->data)] != '#' && openfile->type.is_set<C_CPP>()) {
        ;
      }
      else {
        const char *s1, *s2;
        input == '"' ? s1 = "\"", s2 = s1 : input == '\'' ? s1 = "'", s2 = s1 : input == '(' ? s1 = "(", s2 = ")" :
        input == '{' ? s1 = "{", s2 = "}" : input == '[' ? s1 = "[", s2 = "]" : input == '<' ? s1 = "<", s2 = ">" : 0;
        /* 'Set' the mark, so that 'enclose_marked_region()' dosent exit because there is no marked region. */
        openfile->mark = openfile->current;
        openfile->mark_x = openfile->current_x;
        enclose_marked_region(s1, s2);
        /* Set the flag in the undo struct just created, marking an exception for future undo-redo actions. */
        openfile->undotop->xflags |= SHOULD_NOT_KEEP_MARK;
        openfile->mark = NULL;
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
  }
}

/* Mouse button callback. */
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  /* Some static vars to track double clicking. */
  static double last_time     = 0.0;
  static int    last_button   = 0;
  static vec2   last_mousepos = 0.0f;
  /* Determen if this was a double or tripple click. */
  if (action == GLFW_PRESS) {
    /* Give some wiggle room for the position of the mouse click as it is represented as while numbers only. */
    if (last_button == button && (glfwGetTime() - last_time) < DOUBLE_PRESS_THRESHOLD && mousepos > (last_mousepos - 3.0f) && mousepos < (last_mousepos + 3.0f)) {
      /* Check tripple press first. */
      (mouse_flag.is_set<WAS_MOUSE_DOUBLE_PRESS>() && !mouse_flag.is_set<WAS_MOUSE_TRIPPLE_PRESS>())
        ? mouse_flag.set<WAS_MOUSE_TRIPPLE_PRESS>() : mouse_flag.unset<WAS_MOUSE_TRIPPLE_PRESS>();
      /* Then check for double press. */
      (!mouse_flag.is_set<WAS_MOUSE_DOUBLE_PRESS>() && !mouse_flag.is_set<WAS_MOUSE_TRIPPLE_PRESS>())
        ? mouse_flag.set<WAS_MOUSE_DOUBLE_PRESS>() : mouse_flag.unset<WAS_MOUSE_DOUBLE_PRESS>();
    }
    /* Otherwise, clear both double and tripple flags. */
    else {
      mouse_flag.unset<WAS_MOUSE_DOUBLE_PRESS>();
      mouse_flag.unset<WAS_MOUSE_TRIPPLE_PRESS>();
    }
  }
  /* Left mouse button. */
  if (button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS) {
      /* When in prompt-mode. */
      if (gui->flag.is_set<GUI_PROMPT>()) {
        mouse_flag.set<LEFT_MOUSE_BUTTON_HELD>();
        /* Get the index in the prompt. */
        long index = prompt_index_from_mouse(FALSE);
        /* If the mouse press was on anything other then inside the top-bar, exit prompt-mode. */
        if (index == -1) {
          gui_leave_prompt_mode();
        }
        /* Otherwise, set prompt x pos. */
        else {
          typing_x = index;
        }
      }
      guielement *element = element_from_mousepos();
      if (element) {
        /* When the mouse is pressed in the text element of a editor. */
        if (element->flag.is_set<GUIELEMENT_HAS_EDITOR_DATA>()) {
          if (element == element->data.editor->text) {
            /* When a click occurs in the text element of a editor, make that editor the currently active editor. */
            set_openeditor(element->data.editor);
            mouse_flag.set<LEFT_MOUSE_BUTTON_HELD>();
            /* Get the line and index from the mouse position. */
            Ulong index;
            linestruct *line = line_and_index_from_mousepos(gui->font, &index);
            if (line) {
              openfile->current     = line;
              openfile->mark        = line;
              openfile->current_x   = index;
              openfile->mark_x      = index;
              openfile->softmark    = TRUE;
              openfile->placewewant = xplustabs();
              if (mouse_flag.is_set<WAS_MOUSE_DOUBLE_PRESS>()) {
                /* If this was a double click then select the current word, if any. */
                Ulong st, end;
                st  = get_prev_cursor_word_start_index(TRUE);
                end = get_current_cursor_word_end_index(TRUE);
                if (st != openfile->current_x && end != openfile->current_x) {
                  /* If the double click was inside of a word. */
                  openfile->mark_x    = st;
                  openfile->current_x = end;
                }
                else if (st != openfile->current_x && end == openfile->current_x) {
                  /* The double click was done at the end of the word.  So we select that word. */
                  openfile->mark_x = st;
                }
                else if (st == openfile->current_x && end != openfile->current_x) {
                  /* The double click was done at the begining of the word.  So we select that word. */
                  openfile->mark_x    = st;
                  openfile->current_x = end;
                }
              }
              else if (mouse_flag.is_set<WAS_MOUSE_TRIPPLE_PRESS>()) {
                openfile->mark_x = 0;
                openfile->current_x = strlen(openfile->current->data);
              }
            }
          }
          else if (element == element->data.editor->scrollbar) {
            mouse_flag.set<LEFT_MOUSE_BUTTON_HELD>();
            mouse_flag.set<WAS_EDITOR_SCROLL_PRESS>();
            gui->clicked = element;
            NETLOG("Clicked scrollbar.\n");
          }
        }
        /* For when the top bar is pressed, like when in prompt-mode. */
        /* And, when pressed in any other element. */
        else if (element->callback) {
          element->callback(element, GUIELEMENT_CLICK_CALLBACK);
        }
      }
    }
    else if (action == GLFW_RELEASE) {
      /* If when the left mouse button is relesed without moving, set the mark to NULL. */
      if (openfile->mark == openfile->current && openfile->mark_x == openfile->current_x) {
        openfile->mark = NULL;
      }
      mouse_flag.unset<LEFT_MOUSE_BUTTON_HELD>();
      mouse_flag.unset<WAS_EDITOR_SCROLL_PRESS>();
      gui->clicked = NULL;
    }
  }
  /* Right mouse button. */
  else if (button == GLFW_MOUSE_BUTTON_2) {
    if (action == GLFW_PRESS) {
      mouse_flag.set<RIGHT_MOUSE_BUTTON_HELD>();
    }
    else if (action == GLFW_RELEASE) {
      mouse_flag.unset<RIGHT_MOUSE_BUTTON_HELD>();
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
  if (action == GLFW_PRESS) {
    /* If we just had a tripple click ensure that the next press will not be detected as a double press. */
    last_time     = (mouse_flag.is_set<WAS_MOUSE_TRIPPLE_PRESS>() ? 0.0 : glfwGetTime());
    last_button   = button;
    last_mousepos = mousepos;
  }
  refresh_needed = TRUE;
}

/* Mouse pos callback. */
void mouse_pos_callback(GLFWwindow *window, double x, double y) {
  static vec2 last_mousepos = mousepos;
  mousepos = vec2(x, y);
  if (mouse_flag.is_set<LEFT_MOUSE_BUTTON_HELD>()) {
    if (gui->flag.is_set<GUI_PROMPT>()) {
      long index = prompt_index_from_mouse(TRUE);
      if (index != -1) {
        typing_x = index;
      }
    }
    /* When the clicked element is the scrollbar of any editor, scroll that editor. */
    else if (element_has_editor_data(gui->clicked) && gui->clicked == gui->clicked->data.editor->scrollbar) {
      /* Only update the postion when the mouse y position has changed. */
      if (mousepos.y != last_mousepos.y) {
        vec2 newpos = gui->clicked->pos;
        newpos.y += (mousepos.y - last_mousepos.y);
        newpos.y = fclamp(
          newpos.y,
          gui->clicked->parent->pos.y,
          (gui->clicked->parent->pos.y + gui->clicked->parent->size.h - gui->clicked->size.h)
        );
        move_element(gui->clicked, newpos);
        gui->clicked->data.editor->openfile->edittop = gui_line_from_number(
          gui->clicked->data.editor,
          get_lineno_from_scrollbar_position(gui->clicked->data.editor, (gui->clicked->pos.y - gui->clicked->parent->pos.y))
        );
        refresh_needed = TRUE;
      }
    }
    else {
      Ulong index;
      linestruct *line = line_and_index_from_mousepos(gui->font, &index);
      if (line) {
        openfile->current   = line;
        openfile->current_x = index;
        /* If the left press was a tripple press, adjust the mark based on the current cursor line. */
        if (mouse_flag.is_set<WAS_MOUSE_TRIPPLE_PRESS>()) {
          Ulong st, end;
          st  = 0;
          end = strlen(openfile->mark->data);
          /* If the current cursor line is the same as mark, mark the entire line no matter the cursor pos inside it. */
          if (openfile->current == openfile->mark) {
            openfile->mark_x    = st;
            openfile->current_x = end;
          }
          /* If the current line is above the line where the mark was placed, set the mark index at the end of the line. */
          if (openfile->current->lineno < openfile->mark->lineno) {
            openfile->mark_x = end;
          }
          /* Otherwise, place it at the start. */
          else {
            openfile->mark_x = st;
          }
        }
        /* If the left press was a double press. */
        else if (mouse_flag.is_set<WAS_MOUSE_DOUBLE_PRESS>()) {
          /* Get the start and end of the word that has been marked.  It does not matter is the mark is currently at start or end of that word. */
          Ulong st, end;
          st  = get_prev_word_start_index(openfile->mark->data, openfile->mark_x, TRUE);
          end = get_current_word_end_index(openfile->mark->data, openfile->mark_x, TRUE);
          /* If the current line is the same as the mark line. */
          if (openfile->current == openfile->mark) {
            /* When the current cursor pos is inside the the word, just mark the entire word. */
            if (openfile->current_x >= st && openfile->current_x <= end) {
              openfile->mark_x    = st;
              openfile->current_x = end;
            }
            /* Or, when the cursor if before the word, set the mark at the end of the word. */
            else if (openfile->current_x < st) {
              openfile->mark_x = end;
            }
            /* And when the cursor is after the word, place the mark at the start. */
            else if (openfile->current_x > end) {
              openfile->mark_x = st;
            }
          }
          /* Otherwise, when the cursor line is above the mark, place the mark at the end of the word. */
          else if (openfile->current->lineno < openfile->mark->lineno) {
            openfile->mark_x = end;
          }
          /* And when the cursor line is below the mark line, place the mark at the start of the word. */
          else {
            openfile->mark_x = st;
          }
        }
        openfile->placewewant = xplustabs();
      }
    }
  }
  /* Get the element that the mouse is on. */
  guielement *mouse_element = element_from_mousepos();
  if (mouse_element) {
    /* If the current mouse element is not the current entered element. */
    if (mouse_element != entered_element) {
      /* Run the enter element for the mouse element, if any. */
      if (mouse_element->callback) {
        mouse_element->callback(mouse_element, GUIELEMENT_ENTER_CALLBACK);
      }
      /* If the old entered element was not NULL. */
      if (entered_element) {
        /* Run the leave event for the old entered element, if any. */
        if (entered_element->callback) {
          entered_element->callback(entered_element, GUIELEMENT_LEAVE_CALLBACK);
        }
      }
      entered_element = mouse_element;
    }
  }
  last_mousepos = mousepos;
  refresh_needed = TRUE;
}

/* Window entering and leaving callback. */
void window_enter_callback(GLFWwindow *window, int entered) {
  if (!entered) {
    if (mousepos.x <= ((float)gui->width / 2)) {
      mousepos.x = -30.0f;
    }
    else if (mousepos.x >= ((float)gui->width / 2)) {
      mousepos.x = (gui->width + 30.0f);
    }
    if (mousepos.y <= ((float)gui->height / 2)) {
      mousepos.y = -30.0f;
    }
    else if (mousepos.y >= ((float)gui->height / 2)) {
      mousepos.y = (gui->height + 30.0f);
    }
    /* If there is a currently entered element, call its leave callback.  If it has one. */
    if (entered_element) {
      if (entered_element->callback) {
        entered_element->callback(entered_element, GUIELEMENT_LEAVE_CALLBACK);
      }
      entered_element = NULL;
    }
  }
}

/* Scroll callback. */
void scroll_callback(GLFWwindow *window, double x, double y) {
  guielement *element = element_from_mousepos();
  if (element) {
    /* Check if the element belongs to a editor. */
    if (element->flag.is_set<GUIELEMENT_HAS_EDITOR_DATA>()) {
      /* If the scroll happend where the text is then adjust that editor, even if not the currently focused one. */
      if (element == element->data.editor->text) {
        /* If the mouse left mouse button is held while scrolling, update the cursor pos so that the marked region gets updated. */
        if (mouse_flag.is_set<LEFT_MOUSE_BUTTON_HELD>()) {
          Ulong index;
          linestruct *line = line_and_index_from_mousepos(gui->font, &index);
          if (line) {
            element->data.editor->openfile->current   = line;
            element->data.editor->openfile->current_x = index;
          }
        }
        /* Up and down scroll. */
        edit_scroll((y > 0.0) ? BACKWARD : FORWARD);
      }
      /* Right and left scroll. */
      if (x < 0.0) {
        NETLOG("scroll right.\n");
      }
      else if (x > 0.0) {
        NETLOG("scroll left.\n");
      }
      refresh_needed = TRUE;
      element->data.editor->flag.set<GUIEDITOR_SCROLLBAR_REFRESH_NEEDED>();
    }
    /* Otherwise check if this is a child to a element that belongs to a editor. */
    else if (element->parent && element->parent->flag.is_set<GUIELEMENT_HAS_EDITOR_DATA>()) {
      /* Check if the element is a child of the editors topbar. */
      if (element->parent == element->parent->data.editor->topbar) {
        NETLOG("Is topbar ancestor.\n");
      }
    }
  }
}

#endif
