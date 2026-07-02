#include "../../../include/c_proto.h"


/* ---------------------------------------------------------- Defines ---------------------------------------------------------- */


#define MOUSE_FLAGS(flag)     mouse_flags[((flag) / (sizeof(__TYPE(mouse_flags[0])) * 8))]
#define MOUSE_FLAGMASK(flag)  ((__TYPE(mouse_flags[0]))1 << ((flag) % (sizeof(__TYPE(mouse_flags[0])) * 8)))
#define MOUSE_SET(flag)       MOUSE_FLAGS(flag) |= MOUSE_FLAGMASK(flag)
#define MOUSE_UNSET(flag)     MOUSE_FLAGS(flag) &= ~MOUSE_FLAGMASK(flag)
#define MOUSE_ISSET(flag)     (MOUSE_FLAGS(flag) & MOUSE_FLAGMASK(flag))
#define MOUSE_TOGGLE(flag)    MOUSE_FLAGS(flag) ^= MOUSE_FLAGMASK(flag)

#define DOUBLE_CLICK_THRESHOLD  MILLI_TO_NANO(200)


/* ---------------------------------------------------------- Variables ---------------------------------------------------------- */


static Uint mouse_flags[1];

static int mouse_x      = 0;
static int mouse_y      = 0;
static int mouse_last_x = 0;
static int mouse_last_y = 0;

static Llong mouse_last_click_time = 0;

static int mouse_last_button = 0;


/* ---------------------------------------------------------- Static-Functions ---------------------------------------------------------- */


static Llong get_current_time(void) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return ((now.tv_sec * 1000000000LL) + now.tv_nsec);
}

static void tui_curses_mouse_update_pos(int x, int y) {
  /* Save the current position. */
  mouse_last_x = mouse_x;
  mouse_last_y = mouse_y;
  /* Then, set the new position. */
  mouse_x = x;
  mouse_y = y;
}

static void tui_curses_mouse_update_state(bool press, int button) {
  if (press) {
    /* Check for double/triple clicks. */
    if (button == mouse_last_button && (get_current_time() - mouse_last_click_time) < DOUBLE_CLICK_THRESHOLD
    && mouse_x == mouse_last_x && mouse_y == mouse_last_y)
    {
      /* First check for a triple click. */
      if (MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE) && !MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
        MOUSE_SET(MOUSE_PRESS_WAS_TRIPPLE);
      }
      else {
        MOUSE_UNSET(MOUSE_PRESS_WAS_TRIPPLE);
      }
      /* Then check for a double click. */
      if (!MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE) && !MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
        MOUSE_SET(MOUSE_PRESS_WAS_DOUBLE);
      }
      else {
        MOUSE_UNSET(MOUSE_PRESS_WAS_DOUBLE);
      }
    }
    /* Otherwise, unset both states. */
    else {
      MOUSE_UNSET(MOUSE_PRESS_WAS_DOUBLE);
      MOUSE_UNSET(MOUSE_PRESS_WAS_TRIPPLE);
    }
    /* If the left mouse button was pressed, set it as held. */
    if (button == 1) {
      MOUSE_SET(MOUSE_BUTTON_HELD_LEFT);
    }
    /* If the right mouse button was pressed, set it as held. */
    else if (button == 2) {
      MOUSE_SET(MOUSE_BUTTON_HELD_RIGHT);
    }
    /* If we just had a triple click, ensure the next click will not be detected as a double click. */
    mouse_last_click_time = (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE) ? 0 : get_current_time());
    mouse_last_button     = button;
    mouse_last_x          = mouse_x;
    mouse_last_y          = mouse_y;
  }
  else {
    if (button == 1) {
      MOUSE_UNSET(MOUSE_BUTTON_HELD_LEFT);
    }
    else if (button == 2) {
      MOUSE_UNSET(MOUSE_BUTTON_HELD_RIGHT);
    }
  }
}

static void tui_curses_mouse_routine_button_dn(int button, int x, int y) {
  if (button == 1) {
    /* Edit-Window */
    if (wenclose(midwin, y, x)) {
      x -= margin;
      CLAMP_MIN(x, 0);
      TUI_OF->current = line_from_number_for(
        TUI_OF,
        lclamp(
          (TUI_OF->edittop->lineno + y),
          TUI_OF->edittop->lineno,
          TUI_OF->filebot->lineno
        )
      );
      TUI_OF->current_x = actual_x(TUI_OF->current->data, x);
      /* Set the mark on all clicks. */
      TUI_OF->mark     = TUI_OF->current;
      TUI_OF->mark_x   = TUI_OF->current_x;
      TUI_OF->softmark = TRUE;
      SET_PWW(TUI_OF);
      if (MOUSE_ISSET(MOUSE_PRESS_WAS_DOUBLE)) {
        FCIO_LOG_INFO("Double");
      }
      else if (MOUSE_ISSET(MOUSE_PRESS_WAS_TRIPPLE)) {
        FCIO_LOG_INFO("Triple");
      }
      refresh_needed = TRUE;
    }
  }
}

static void tui_curses_mouse_routine_button_up(int button, int x, int y) {
  if (button == 1) {
    if (wenclose(midwin, y, x) && TUI_OF->mark == TUI_OF->current && TUI_OF->mark_x == TUI_OF->current_x) {
      TUI_OF->mark = NULL;
      refresh_needed = TRUE;
    }
  }
}

static void tui_curses_mouse_routine_position(int x, int y) {
  if (MOUSE_ISSET(MOUSE_BUTTON_HELD_LEFT)) {
    if (wenclose(midwin, y, x)) {
      x -= margin;
      CLAMP_MIN(x, 0);
      TUI_OF->current = line_from_number_for(
        TUI_OF,
        lclamp(
          (TUI_OF->edittop->lineno + y),
          TUI_OF->edittop->lineno,
          TUI_OF->filebot->lineno
        )
      );
      TUI_OF->current_x = actual_x(TUI_OF->current->data, x);
      refresh_needed = TRUE;
    }
  }
}


/* ---------------------------------------------------------- Global-Functions ---------------------------------------------------------- */


void tui_curses_mouse_handle_events(void) {
  MEVENT ev;
  if (getmouse(&ev) == ERR) {
    return;
  }
  if (ev.bstate & (BUTTON1_PRESSED | BUTTON2_PRESSED)) {
    FCIO_LOG_INFO("(BUTTON1_PRESSED | BUTTON2_PRESSED)");
    tui_curses_mouse_update_state(TRUE, ((ev.bstate & BUTTON1_PRESSED) ? 1 : 2));
    tui_curses_mouse_routine_button_dn(((ev.bstate & BUTTON1_PRESSED) ? 1 : 2), ev.x, ev.y);
  }
  /* These seam to be when a button is pressed and then released very
   * fast, this probebly has something to do with 'mouseinterval(*ms*)'. */
  if (ev.bstate & (BUTTON1_CLICKED | BUTTON2_CLICKED)) {
    FCIO_LOG_INFO("(BUTTON1_CLICKED | BUTTON2_CLICKED)");
  }
  if (ev.bstate & (BUTTON1_RELEASED | BUTTON2_RELEASED)) {
    FCIO_LOG_INFO("(BUTTON1_RELEASED | BUTTON2_RELEASED)");
    tui_curses_mouse_update_state(FALSE, ((ev.bstate & BUTTON1_RELEASED) ? 1 : 2));
    tui_curses_mouse_routine_button_up(((ev.bstate & BUTTON1_RELEASED) ? 1 : 2), ev.x, ev.y);
  }
  if (ev.bstate & REPORT_MOUSE_POSITION) {
    FCIO_LOG_INFO("REPORT_MOUSE_POSITION");
    tui_curses_mouse_update_pos(ev.x, ev.y);
    tui_curses_mouse_routine_position(ev.x, ev.y);
  }
}
