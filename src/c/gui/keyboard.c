/** @file keyboard.c

  @author  Melwin Svensson.
  @date    24-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define SC(x)  \
  /* Scan-Code shorthand, so that if we ever change the windowing  \
   * frameword, we do not have to change anything except this. */  \
  SDL_SCANCODE_##x

#define KC(x)  \
  /* Key-Code shorthand, so that if we ever change the windowing   \
   * frameword, we do not have to change anything except this. */  \
  SDLK_##x

#define KEY(x)  \
  SDLK_##x

#define SCKC(x)  SDL_SCANCODE_TO_KEYCODE((x))
  

/* Properly name the super key. */
#define SDL_KMOD_LSUPER  SDL_KMOD_LGUI
#define SDL_KMOD_RSUPER  SDL_KMOD_RGUI
#define SDL_KMOD_SUPER   SDL_KMOD_GUI

/* Also properly name the enter. */
#define SDLK_ENTER  SDLK_RETURN

#define M(x)  SDL_KMOD_##x

#define SHIFT_HELD(x)  DO_WHILE(shift_held = (x);)

/* ----------------------------- Ignore modifier builder for `MOD_IG` ----------------------------- */

#define IG_1(i)                       (M(i))
#define IG_2(i0, i1)                  (M(i0) | M(i1))
#define IG_3(i0, i1, i2)              (M(i0) | M(i1) | M(i2))
#define IG_4(i0, i1, i2, i3)          (M(i0) | M(i1) | M(i2) | M(i3))
#define IG_5(i0, i1, i2, i3, i4)      (M(i0) | M(i1) | M(i2) | M(i3) | M(i4))
#define IG_6(i0, i1, i2, i3, i4, i5)  (M(i0) | M(i1) | M(i2) | M(i3) | M(i4) | M(i5))

#define IG(...)  VA_MACRO(IG_, __VA_ARGS__)

/* ----------------------------- Check N modifiers active ----------------------------- */

#define MOD_1(x, m)                                             \
  /* This expression is only `TRUE` when any flag in `m0`      \
   * is set, and no other then all provided flags are set. */  \
  (((x) & (m)) && !((x) & ~(m)))

#define MOD_2(x, m0, m1)                                                          \
  /* This expression is only `TRUE` when any flag in `m0` is set and any         \
   * flag in `m1` is also set, and no other then all provided flags are set. */  \
  (((x) & (m0)) && ((x) & (m1)) && !((x) & ~((m0) | (m1))))

#define MOD_3(x, m0, m1, m2)                                                         \
  /* This expression is only `TRUE` when any flag in m0, and any flag in            \
   * m1, and any flag in m2 is set and no other then all flags is set. */           \
  (((x) & (m0)) && ((x) & (m1)) && ((x) & (m2)) && !((x) & ~((m0) | (m1) | (m2))))

#define MOD_4(x, m0, m1, m2, m3)  \
  (((x) & (m0)) && ((x) & (m1)) && ((x) & (m2)) && ((x) & (m3)) && !((x) & ~((m0) | (m1) | (m2) | (m3))))

#define MOD(x, ...)  VA_MACRO_1ARG(MOD_, x, __VA_ARGS__)

/* ----------------------------- Check N modifiers active, ignoring some ----------------------------- */

#define MOD_IG_1(x, i, m)  \
  (((x) & (m)) && !((x) & ~((m) | (i))))

#define MOD_IG_2(x, i, m0, m1)  \
  (((x) & (m0)) && ((x) & (m1)) && !((x) & ~((m0) | (m1) | (i))))

#define MOD_IG_3(x, i, m0, m1, m2)  \
  (((x) & (m0)) && ((x) & (m1)) && ((x) & (m2)) && !((x) & ~((m0) | (m1) | (m2) | (i))))

#define MOD_IG_4(x, i, m0, m1, m2, m3)  \
  (((x) & (m0)) && ((x) & (m1)) && ((x) & (m2)) && ((x) & (m3)) && !((x) & ~((m0) | (m1) | (m2) | (m3) | (i))))

#define MOD_IG(x, i, ...)   VA_MACRO_2ARG(MOD_IG_, x, i, __VA_ARGS__)
#define MOD_IG_DEF(x, ...)  MOD_IG(x, IG(CAPS, SCROLL, NUM), __VA_ARGS__)

/* ----------------------------- Check no modifiers set ----------------------------- */

#define MOD_NONE(x)  (!(x))
#define MOD_NONE_IG(x, ...)  (!(x) || MOD(x, IG(__VA_ARGS__)))
#define MOD_NONE_IG_DEF(x)   MOD_NONE_IG(x, CAPS, SCROLL, NUM)


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


/* This is used as absolute and singular flags,
 * after we filter them, so we only need to filter once. */
typedef enum {
  KB_MOD_NOT_SUPPORTED = -1,
  KB_MOD_NONE,
  KB_MOD_SHIFT_CTRL,
  KB_MOD_SHIFT_ALT,
  KB_MOD_SHIFT,
  KB_MOD_CTRL,
  KB_MOD_ALT,
# define KB_MOD_NOT_SUPPORTED  KB_MOD_NOT_SUPPORTED
# define KB_MOD_NONE           KB_MOD_NONE
# define KB_MOD_SHIFT_CTRL     KB_MOD_SHIFT_CTRL
# define KB_MOD_SHIFT_ALT      KB_MOD_SHIFT_ALT
# define KB_MOD_SHIFT          KB_MOD_SHIFT
# define KB_MOD_CTRL           KB_MOD_CTRL
# define KB_MOD_ALT            KB_MOD_ALT
} KbModType;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


_UNUSED
static inline void get_enclose_chars(const char ch, const char **const s1, const char **const s2) {
  switch (ch) {
    case '"': {
      *s1 = *s2 = "\"";
      break;
    }
    case '\'': {
      *s1 = *s2 = "'";
      break;
    }
    case '(': {
      *s1 = "(";
      *s2 = ")";
      break;
    }
    case '{': {
      *s1 = "{";
      *s2 = "}";
      break;
    }
    case '[': {
      *s1 = "[";
      *s2 = "]";
      break;
    }
    case '<': {
      *s1 = "<";
      *s2 = ">";
      break;
    }
  }
}

/* ----------------------------- Kb filter mod ----------------------------- */

/* Returns a exclusive modifier type, so that we can switch case correctly on any combination, regardless of order. */
static inline KbModType kb_filter_mod(Ushort mod) {
  if (MOD_NONE_IG_DEF(mod)) {
    return KB_MOD_NONE;
  }
  else if (MOD_IG_DEF(mod, M(SHIFT), M(CTRL))) {
    return KB_MOD_SHIFT_CTRL;
  }
  else if (MOD_IG_DEF(mod, M(SHIFT), M(ALT))) {
    return KB_MOD_SHIFT_ALT;
  }
  else if (MOD_IG_DEF(mod, M(SHIFT))) {
    return KB_MOD_SHIFT;
  }
  else if (MOD_IG_DEF(mod, M(CTRL))) {
    return KB_MOD_CTRL;
  }
  else if (MOD_IG_DEF(mod, M(ALT))) {
    return KB_MOD_ALT;
  }
  else {
    return KB_MOD_NOT_SUPPORTED;
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Kb key pressed ----------------------------- */

void kb_key_pressed(Uint key, Uint _UNUSED scan, Ushort mod, bool repeat) {
  VoidFuncPtr func = NULL;
  linestruct *was_current;
  Ulong was_x;
  gl_mouse_flag_clear_all();
  /* Reset shift. */
  SHIFT_HELD(FALSE);
  switch (kb_filter_mod(mod)) {
    case KB_MOD_NOT_SUPPORTED: {
      break;
    }
    case KB_MOD_NONE: {
      switch (key) {
        case KC(ESCAPE): {
          if (menu_get_active()) {
            menu_show(menu_get_active(), FALSE);
          }
          break;
        }
        case KC(RIGHT): {
          if (menu_get_active() && menu_allows_arrow_depth_navigation(menu_get_active())) {
            menu_submenu_enter(menu_get_active());
            return;
          }
          else {
            func = do_right;
          }
          break;
        }
        case KC(LEFT): {
          if (menu_get_active() && menu_allows_arrow_depth_navigation(menu_get_active())) {
            menu_submenu_exit(menu_get_active());
            return;
          }
          else {
            func = do_left;
          }
          break;
        }
        case KC(UP): {
          if (menu_get_active()) {
            menu_selected_up(menu_get_active());
            return;
          }
          else {
            func = do_up;
          }
          break;
        }
        case KC(DOWN): {
          if (menu_get_active()) {
            menu_selected_down(menu_get_active());
            return;
          }
          else {
            func = do_down;
          }
          break;
        }
        case KC(BACKSPACE): {
          func = do_backspace;
          break;
        }
        case KC(DELETE): {
          func = do_delete;
          break;
        }
        case KC(ENTER): {
          if (menu_get_active()) {
            menu_routine_accept(menu_get_active());
            return;
          }
          else {
            func = do_enter;
          }
          break;
        }
        case KC(TAB): {
          if (menu_get_active() && menu_allows_accept_on_tab(menu_get_active())) {
            menu_routine_accept(menu_get_active());
            return;
          }
          else {
            func = do_tab;
          }
          break;
        }
        case KC(HOME): {
          func = do_home;
          break;
        }
        case KC(END): {
          func = do_end;
          break;
        }
        case KC(F11): {
          if (!repeat) {
            gl_window_borderless_fullscreen();
          }
          break;
        }
      }
      break;
    }
    case KB_MOD_SHIFT_CTRL: {
      switch (key) {
        case KC(A): {
          if (GUI_OF->is_c_file || GUI_OF->is_cxx_file || GUI_OF->is_glsl_file) {
            func = do_block_comment;
          }
          break;
        }
        case KC(N): {
          editor_create(TRUE);
          editor_redecorate(openeditor);
          editor_resize(openeditor);
          break;
        }
        case KC(P): {
          promptmenu_ask(PROMPTMENU_TYPE_NONE);
          break;
        }
        case KC(RIGHT): {
          shift_held = TRUE;
          func = to_next_word;
          break;
        }
        case KC(LEFT): {
          SHIFT_HELD(TRUE);
          func = to_prev_word;
          break;
        }
        case KC(UP): {
          SHIFT_HELD(TRUE);
          func = to_prev_block;
          break;
        }
        case KC(DOWN): {
          SHIFT_HELD(TRUE);
          func = to_next_block;
          break;
        }
        case KC(ENTER): {
          func = do_insert_empty_line_above;
          break;
        }
        case KC(HOME): {
          SHIFT_HELD(TRUE);
          func = to_first_line;
          break;
        }
        case KC(END): {
          SHIFT_HELD(TRUE);
          func = to_last_line;
          break;
        }
        case KC(MINUS): {
          font_decrease_line_height(textfont);
          statusline(AHEM, "Font line height: %ld", font_get_line_height(textfont));
          editor_update_all();
          refresh_needed = TRUE;
          break;
        }
        case KC(EQUALS): {
          font_increase_line_height(textfont);
          statusline(AHEM, "Font line height: %ld", font_get_line_height(textfont));
          editor_update_all();
          refresh_needed = TRUE;
          break;
        }
      }
      break;
    }
    case KB_MOD_SHIFT_ALT: {
      switch (key) {
        case KC(RIGHT): {
          func = editor_switch_to_next;
          break;
        }
        case KC(LEFT): {
          func = editor_switch_to_prev;
          break;
        }
      }
      break;
    }
    case KB_MOD_SHIFT: {
      switch (key) {
        case KC(RIGHT): {
          SHIFT_HELD(TRUE);
          func = do_right;
          break;
        }
        case KC(LEFT): {
          SHIFT_HELD(TRUE);
          func = do_left;
          break;
        }
        case KC(UP): {
          SHIFT_HELD(TRUE);
          func = do_up;
          break;
        }
        case KC(DOWN): {
          SHIFT_HELD(TRUE);
          func = do_down;
          break;
        }
        case KC(BACKSPACE): {
          func = do_backspace;
          break;
        }
        case KC(DELETE): {
          func = do_delete;
          break;
        }
        case KC(TAB): {
          func = do_unindent;
          break;
        }
        case KC(HOME): {
          SHIFT_HELD(TRUE);
          func = do_home;
          break;
        }
        case KC(END): {
          SHIFT_HELD(TRUE);
          func = do_end;
        }
      }
      break;
    }
    case KB_MOD_CTRL: {
      switch (key) {
        case KC(A): {
          func = mark_whole_file;
          break;
        }
        case KC(N): {
          editor_open_new_empty_buffer();
          break;
        }
        case KC(O): {
          promptmenu_ask(PROMPTMENU_TYPE_FILE_OPEN);
          break;
        }
        case KC(S): {
          if (*GUI_OF->filename && file_exists(GUI_OF->filename)) {
            func = do_savefile;
          }
          else {
            promptmenu_ask(PROMPTMENU_TYPE_FILE_SAVE);
            return;
          }
          break;
        }
        case KC(Q): {
          if (!GUI_OF->modified && !ISSET(VIEW_MODE)) {
            if (gl_window_quit()) {
              return;
            }
          }
          /* TODO: Fix this. */
          else if (ISSET(SAVE_ON_EXIT) && *GUI_OF->filename) {
            ;
          }
          else {
            promptmenu_ask(PROMPTMENU_TYPE_FILE_SAVE_MODIFIED);
          }
          break;
        }
        case KC(X): {
          func = cut_text;
          break;
        }
        case KC(Y): {
          func = do_redo;
          break;
        }
        case KC(Z): {
          func = do_undo;
          break;
        }
        case KC(RIGHT): {
          func = to_next_word;
          break;
        }
        case KC(LEFT): {
          func = to_prev_word;
          break;
        }
        case KC(UP): {
          func = to_prev_block;
          break;
        }
        case KC(DOWN): {
          func = to_next_block;
          break;
        }
        case KC(BACKSPACE): {
          func = chop_previous_word;
          break;
        }
        case KC(DELETE): {
          func = chop_next_word;
          break;
        }
        case KC(ENTER): {
          func = do_insert_empty_line_below;
          break;
        }
        case KC(HOME): {
          func = to_first_line;
          break;
        }
        case KC(END): {
          func = to_last_line;
          break;
        }
        case KC(SLASH): {
          func = do_comment;
          break;
        }
        case KC(MINUS): {
          font_decrease_size(textfont);
          statusline(AHEM, "Font size: %u", font_get_size(textfont));
          editor_update_all();
          refresh_needed = TRUE;
          break;
        }
        case KC(EQUALS): {
          font_increase_size(textfont);
          statusline(AHEM, "Font size: %u", font_get_size(textfont));
          editor_update_all();
          refresh_needed = TRUE;
          break;
        }
      }
      break;
    }
    case KB_MOD_ALT: {
      switch (key) {
        case KC(N): {
          if (!repeat) {
            TOGGLE(LINE_NUMBERS);
            editor_update_all();
          }
          break;
        }
        case KC(T): {
          if (!repeat) {
            TOGGLE(TABS_TO_SPACES);
            refresh_needed = TRUE;
          }
          break;
        }
        case KC(RIGHT): {
          func = editor_switch_openfile_to_next;
          break;
        }
        case KC(LEFT): {
          func = editor_switch_openfile_to_prev;
          break;
        }
        case KC(UP): {
          func = move_lines_up;
          break;
        }
        case KC(DOWN): {
          func = move_lines_down;
          break;
        }
      }
      break;
    }
  }
  if (func != do_cycle) {
    cycling_aim = 0;
  }
  if (!func) {
    pletion_line   = NULL;
    keep_cutbuffer = FALSE;
    return;
  }
  /* When in restricted-mode, and the function changes something, just return. */
  if (ISSET(VIEW_MODE) && changes_something(func)) {
    return;
  }
  /* When not cutting or copying text, drop the cutbuffer the next time. */
  if (func != cut_text && func != copy_text && func != zap_text) {
    keep_cutbuffer = FALSE;
  }
  if (func != complete_a_word) {
    pletion_line = NULL;
  }
  was_current = GUI_OF->current;
  was_x       = GUI_OF->current_x;
  /* If shifted movement occurs, set the mark. */
  if (shift_held && !GUI_OF->mark) {
    GUI_OF->mark     = GUI_OF->current;
    GUI_OF->mark_x   = GUI_OF->current_x;
    GUI_OF->softmark = TRUE;
  }
  /* Do the thing... */
  func();
  /* When the marked region changes whitout <Shift> being held, discard a soft mark.
   * And when the set of lines changes, reset the 'last line to' flag.  What...? */
  if (GUI_OF->mark && GUI_OF->softmark && !shift_held && !keep_mark
  && (GUI_OF->current != was_current || GUI_OF->current_x != was_x || wanted_to_move(func)))
  {
    GUI_OF->mark = NULL;
  }
  /* Adjust the viewport if we move to the last or first line. */
  if (func == to_first_line || func == to_last_line) {
    if (current_is_offscreen_for(GUI_CTX)) {
      adjust_viewport_for(GUI_CTX, ((focusing || ISSET(JUMPY_SCROLLING)) ? CENTERING : FLOWING));
    }
  }
  else if (GUI_OF->current != was_current) {
    also_the_last = FALSE;
  }
  /* Currently just shoe-horn in this right here. */
  if (func == do_enter && current_is_offscreen_for(GUI_CTX)) {
    adjust_viewport_for(GUI_CTX, CENTERING);
  }
  /* When we have moved or changed something, tell the open-editor it needs to update the scrollbar. */
  if (wanted_to_move(func) || changes_something(func) || func == do_undo || func == do_redo) {
    scrollbar_refresh(openeditor->sb);
  }
  /* If the suggest-menu has entries in it, we should update it. */
  if (menu_len(suggestmenu())) {
    suggestmenu_run();
  }
  last_key_was_bracket = FALSE;
  last_bracket_char    = NUL;
  keep_mark            = FALSE;
  refresh_needed       = TRUE;
}

/* ----------------------------- Kb char input ----------------------------- */

/* TODO: Fix this shit. */
void kb_char_input(const char *const restrict data, Ushort mod) {
  Ulong len;
  char *burst;
  char ch;
  const char *s1;
  const char *s2;
  /* Only accept char input when no mods are set, ignoring CAPS, SCROLL and NUM. */
  if (MOD_NONE_IG(mod, CAPS, NUM, SCROLL, SHIFT) && (len = STRLEN(data))) {
    burst = measured_copy(data, len);
    /* Non ASCII */
    if (len != 1) {
      inject_into_buffer(GUI_CTX, burst, len);
    }
    else {
      ch = *burst;
      /* Adjust the viewport if the cursor is offscreen. */
      if (current_is_offscreen_for(GUI_CTX)) {
        adjust_viewport_for(GUI_CTX, CENTERING);
      }
      /* When in restricted mode, just leave. */
      if (ISSET(VIEW_MODE)) {
        return;
      }
      /* If a region is marked, and input is a enclose char, then we enclose the marked region with that char. */
      if (GUI_OF->mark && is_enclose_char(ch)) {
        get_enclose_chars(ch, &s1, &s2);
        enclose_marked_region_for(GUI_OF, s1, s2);
        free(burst);
        return;
      }
      else if (GUI_OF->mark && GUI_OF->softmark) {
        zap_replace_text_for(GUI_CTX, &ch, 1);
        keep_mark            = FALSE;
        last_key_was_bracket = FALSE;
        last_bracket_char    = NUL;
        GUI_OF->mark         = NULL;
        free(burst);
        return;
      }
      /* If an enclose char is pressed without having a marked region, we enclose in place. */
      else if (is_enclose_char(ch)) {
        /* If quote or double quote was just enclosed in place just move once to the right. */
        if ((ch == '"' && last_key_was_bracket && last_bracket_char == '"') || (ch == '\'' && last_key_was_bracket && last_bracket_char == '\'')) {
          do_right();
          keep_mark = FALSE;
          last_key_was_bracket = FALSE;
          last_bracket_char = '\0';
          refresh_needed = TRUE;
          free(burst);
          return;
        }
        /* Exceptions for enclosing double quotes. */
        else if (ch == '"'
        /* Before current cursor position. */
        && (((is_prev_cursor_word_char(FALSE) || is_prev_cursor_char_one_of("\"><")))
        /* After current cursor position. */
        || (!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of(")}]")))) {
          ;
        }
        /* Exceptions for enclosing single quotes. */
        else if (ch == '\''
        /* Before current cursor position. */
        && (((is_prev_cursor_word_char(FALSE) || is_prev_cursor_char_one_of("'><")))
        /* After current cursor position. */
        || (!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of(")}]")))) {
          ;
        }
        /* Exceptions for enclosing brackets. */
        else if ((ch == '(' || ch == '[' || ch == '{')
        /* We only allow the insertion of double brackets when the cursor is either at a blank char or at EOL, or at any of the
          * given chars, all other sinarios with other chars at the cursor will result in only the start bracket beeing inserted. */
        && ((!is_cursor_blank_char() && !is_cursor_char('\0') && !is_cursor_char_one_of("\":;')}],")))) {
          ;
        }
        /* If '<' is pressed without being in a c/cpp file and at an include line, we simply do nothing. */
        else if (ch == '<' && GUI_OF->current->data[indentlen(GUI_OF->current->data)] != '#' && (GUI_OF->is_c_file || GUI_OF->is_cxx_file) /* GUI_OF->type.is_set<C_CPP>() */) {
          ;
        }
        else {
          ch == '"'  ? s1 = "\"", s2 = s1 :
          ch == '\'' ? s1 = "'",  s2 = s1 :
          ch == '('  ? s1 = "(",  s2 = ")" :
          ch == '{'  ? s1 = "{",  s2 = "}" :
          ch == '['  ? s1 = "[",  s2 = "]" :
          ch == '<'  ? s1 = "<",  s2 = ">" : 0;
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
          last_bracket_char = ch;
          free(burst);
          return;
        }
      }
      inject_into_buffer(GUI_CTX, &ch, 1);
      last_key_was_bracket = FALSE;
      last_bracket_char    = NUL;
      keep_mark            = FALSE;
      refresh_needed       = TRUE;
      suggestmenu_run();
    }
    free(burst);
  }
}

/* ----------------------------- Kb prompt key pressed ----------------------------- */

void kb_key_pressed_prompt(Uint key, Uint _UNUSED scan, Ushort mod, bool _UNUSED repeat) {
  if (promptmenu_yn_mode()) {
    switch (kb_filter_mod(mod)) {
      case KB_MOD_NONE:
      case KB_MOD_SHIFT: {
        switch (key) {
          case KC(ESCAPE): {
            promptmenu_close();
            break;
          }
          case KC(N): {
            promptmenu_routine_no();
            break;
          }
          case KC(Y): {
            promptmenu_routine_yes();
            break;
          }
        }
        break;
      }
      case KB_MOD_CTRL: {
        switch (key) {
          case KC(C):
          case KC(Q):
          case KC(ESCAPE): {
            promptmenu_close();
            break;
          }
        }
        break;
      }
      default: {
        break;
      }
    } 
  }
  else {
    SHIFT_HELD(FALSE);
    switch (kb_filter_mod(mod)) {
      case KB_MOD_NOT_SUPPORTED: {
        break;
      }
      case KB_MOD_NONE: {
        switch (key) {
          case KC(ESCAPE): {
            promptmenu_close();
            break;
          }
          case KC(ENTER): {
            promptmenu_routine_enter();
            break;
          }
          case KC(HOME): {
            do_statusbar_home();
            break;
          }
          case KC(END): {
            do_statusbar_end();
            break;
          }
          case KC(RIGHT): {
            st_marked = FALSE;
            do_statusbar_right();
            break;
          }
          case KC(LEFT): {
            st_marked = FALSE;
            do_statusbar_left();
            break;
          }
          case KC(DOWN): {
            if (menu_get_active()) {
              menu_selected_down(menu_get_active());
            }
            break;
          }
          case KC(UP): {
            if (menu_get_active()) {
              menu_selected_up(menu_get_active());
            }
            break;
          }
          case KC(BACKSPACE): {
            do_statusbar_backspace(TRUE);
            promptmenu_routine_completions_search();
            break;
          }
          case KC(DELETE): {
            do_statusbar_delete();
            promptmenu_routine_completions_search();
            break;
          }
          case KC(TAB): {
            promptmenu_routine_tab();
            break;
          }
        }
        break;
      }
      case KB_MOD_SHIFT_CTRL: {
        break;
      }
      case KB_MOD_SHIFT_ALT: {
        break;
      }
      case KB_MOD_SHIFT: {
        break;
      }
      case KB_MOD_CTRL: {
        switch (key) {
          case KC(C):
          case KC(Q):
          case KC(ESCAPE): {
            promptmenu_close();
            break;
          }
          case KC(Z): {
            do_statusbar_undo();
            promptmenu_routine_completions_search();
            break;
          }
          case KC(Y): {
            do_statusbar_redo();
            promptmenu_routine_completions_search();
            break;
          }
          case KC(RIGHT): {
            do_statusbar_next_word();
            break;
          }
          case KC(LEFT): {
            do_statusbar_prev_word();
            break;
          }
          case KC(BACKSPACE): {
            do_statusbar_chop_prev_word();
            promptmenu_routine_completions_search();
            break;
          }
          case KC(DELETE): {
            do_statusbar_chop_next_word();
            promptmenu_routine_completions_search();
            break;
          }
        }
        break;
      }
      case KB_MOD_ALT: {
        break;
      }
    }
    promptmenu_refresh_text();
  }
}

/* ----------------------------- Kb prompt char input ----------------------------- */

/* Handle char input when the `prompt-menu` is active. */
void kb_char_input_prompt(const char *const restrict data, Ushort mod) {
  Ulong len;
  char *burst;
  /* Only ever perform any action when no modifiers (excluding CAPS, SCROLL and NUM).  And when not in Y/N mode. */
  if (!promptmenu_yn_mode() && MOD_NONE_IG(mod, CAPS, NUM, SCROLL, SHIFT) && (len = STRLEN(data))) {
    burst = measured_copy(data, len);
    inject_into_answer(burst, len);
    free(burst);
    promptmenu_refresh_text();
    promptmenu_routine_completions_search();
  }
}
