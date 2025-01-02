#include "../../include/prototypes.h"

#ifdef HAVE_GLFW

/* Window resize callback. */
void window_resize_callback(GLFWwindow *window, int newwidth, int newheight) {
  window_width  = newwidth;
  window_height = newheight;
  /* Set viewport. */
  glViewport(0, 0, window_width, window_height);
  /* Set the projection. */
  projection = ortho_projection(0.0f, window_width, 0.0f, window_height);
  // matrix4x4_set_orthographic(&projection, 0, window_width, 0, window_height, -1, 1);
  /* Upload it to both the shaders. */
  update_projection_uniform(fontshader);
  update_projection_uniform(rectshader);
  /* Confirm the margin before we calculate the size of the gutter. */
  confirm_margin();
  /* Calculate the font size. */
  resize_element(gutterelement, vec2((FONT_WIDTH(font[GUI_NORMAL_TEXT].font) * margin), window_height));
  move_element(editelement, vec2((gutterelement->pos.x + gutterelement->size.w), editelement->pos.y));
  /* Ensure the edit element is correctly sized. */
  resize_element(editelement, vec2(window_width - (gutterelement->pos.x + gutterelement->size.w), window_height));
  NETLOG("editelement_size: %f:%f\n", editelement->size.w, editelement->size.h);
  /* Calculate the rows and columns. */
  editwinrows = (editelement->size.h / FONT_HEIGHT(font[GUI_NORMAL_TEXT].font));
  if (texture_font_is_mono(font[GUI_NORMAL_TEXT].font)) {
    NETLOG("Font is mono.\n");
    texture_glyph_t *glyph = texture_font_get_glyph(font[GUI_NORMAL_TEXT].font, " ");
    editwincols = (editelement->size.w / glyph->advance_x);
  }
  else {
    editwincols = (editelement->size.w / FONT_WIDTH(font[GUI_NORMAL_TEXT].font)) * 0.9f;
  }
  NETLOG("editwincols: %d\nwincols:%d\n", editwincols, (window_width / FONT_WIDTH(font[GUI_NORMAL_TEXT].font)));
}

/* Maximize callback. */
void window_maximize_callback(GLFWwindow *window, int maximized) {
  ivec2 size;
  glfwGetWindowSize(window, &size.w, &size.h);
  window_resize_callback(window, size.w, size.h);
}

/* Key callback. */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  /* Reset shift. */
  shift_held = FALSE;
  /* What function does the key entail. */
  void (*function)(void) = NULL;
  /* Check what action was done, if any. */
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key) {
      /* Enclose marked region. */
      case GLFW_KEY_A: {
        if (mods == (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) {
          if (openfile->type.is_set<C_CPP>() || openfile->type.is_set<GLSL>()) {
            function = do_block_comment;
          }
        }
        break;
      }
      /* If CTRL+Q is pressed, quit. */
      case GLFW_KEY_Q: {
        if (mods & GLFW_MOD_CONTROL) {
          guiflag.unset<GUI_RUNNING>();
        }
        break;
      }
      /* Line numbers toggle. */
      case GLFW_KEY_N: {
        if (action == GLFW_PRESS) {
          TOGGLE(LINE_NUMBERS);
          confirm_margin();
          window_resize_callback(window, window_width, window_height);
        }
        break;
      }
      /* Undo action. */
      case GLFW_KEY_Z: {
        if (mods == GLFW_MOD_CONTROL) {
          function = do_undo;
        }
        break;
      }
      /* Redo action. */
      case GLFW_KEY_Y: {
        if (mods == GLFW_MOD_CONTROL) {
          function = do_redo;
        }
        break;
      }
      case GLFW_KEY_RIGHT: {
        switch (mods) {
          /* Move to next word with shift held. */
          case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
            shift_held = TRUE;
          }
          /* Move to next word. */
          case GLFW_MOD_CONTROL: {
            function = to_next_word;
            break;
          }
          /* Shift+Right. */
          case GLFW_MOD_SHIFT: {
            shift_held = TRUE;
          }
          /* Move right. */
          case 0: {
            function = do_right;
          }
        }
        break;
      }
      case GLFW_KEY_LEFT: {
        switch (mods) {
          /* Move to prev word with shift held. */
          case (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT): {
            shift_held = TRUE;
          }
          /* Move to prev word. */
          case GLFW_MOD_CONTROL: {
            function = to_prev_word;
            break;
          }
          /* Shift+Left. */
          case GLFW_MOD_SHIFT: {
            shift_held = TRUE;
          }
          /* Move left. */
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
          }
          /* Moves to prev indent block. */
          case GLFW_MOD_CONTROL: {
            function = to_prev_block;
            break;
          }
          /* Shift+Up. */
          case GLFW_MOD_SHIFT: {
            shift_held = TRUE;
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
          }
          /* Moves to next block. */
          case GLFW_MOD_CONTROL: {
            function = to_next_block;
            break;
          }
          /* Shift+Down. */
          case GLFW_MOD_SHIFT: {
            shift_held = TRUE;
          }
          /* Move down. */
          case 0: {
            function = do_down;
          }
        }
        break;
      }
      case GLFW_KEY_BACKSPACE: {
        /* Simple backspace. */
        if (!mods) {
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
          }
          /* Move to the first line in the file. */
          case GLFW_MOD_CONTROL: {
            function = to_first_line;
            break;
          }
          /* Move to home of current line while shift is held. */
          case GLFW_MOD_SHIFT: {
            shift_held = TRUE;
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
          }
          /* Move to the last line in the file. */
          case GLFW_MOD_CONTROL: {
            function = to_last_line;
            break;
          }
          /* Shift was held while moving to end of line. */
          case GLFW_MOD_SHIFT: {
            shift_held = TRUE;
          }
          /* Simple move to end of line. */
          case 0: {
            function = do_end;
          }
        }
        break;
      }
      /* Comment action. */
      case GLFW_KEY_SLASH: {
        if (mods == GLFW_MOD_CONTROL) {
          function = do_comment;
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
  last_key_was_bracket = FALSE;
  last_bracket_char = '\0';
  keep_mark = FALSE;
}

/* Char input callback. */
void char_callback(GLFWwindow *window, Uint ch) {
  if (ISSET(VIEW_MODE)) {
    return;
  }
  else {
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
      openfile->mark = NULL;
    }
    /* If a enclose char is pressed without a having a marked region, we simply enclose in place. */
    else if (is_enclose_char(input)) {
      /* If quote or double quote was just enclosed in place just move once to the right. */
      if ((input == '"' && last_key_was_bracket && last_bracket_char == '"') || (input == '\'' && last_key_was_bracket && last_bracket_char == '\'')) {
        do_right();
        last_key_was_bracket = FALSE;
        last_bracket_char = '\0';
        return;
      }
      /* Exceptions for enclosing quotes. */
      else if (input == '"'
        /* Before current cursor position. */
        && (((is_prev_cursor_word_char() || is_prev_cursor_char_one_of("\"><")))
        /* After current cursor position. */
        || (!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char(';')))) {
        ;
      }
      /* Exceptions for enclosing brackets. */
      else if ((input == '(' || input == '[' || input == '{')
        /* After current cursor position. */
        && ((!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of("\";'")) || (is_cursor_char_one_of("({[")))) {
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
        /* This flag ensures that if backspace is the next key that is pressed it will erase both of the enclose char`s. */
        last_key_was_bracket = TRUE;
        last_bracket_char = input;
        return;
      }
    }
    inject((char *)&ch, 1);
    last_key_was_bracket = FALSE;
    last_bracket_char = '\0';
    keep_mark = FALSE;
  }
}

#endif