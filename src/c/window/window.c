#include "window.h"

#include "../event/nfdwriter.h"
#include "../term/move.h"
#include "../term/tui.h"

#include <sys/ioctl.h>
#include <errno.h>

typedef struct {
  Uchar r;      /* Red */
  Uchar g;      /* Green */
  Uchar b;      /* Blue */
  bool enabled; /* If this has been set. */
} nwindow_rgb;

typedef struct {
  char *data;
} nwindow_line;

struct nwindow {
  nwindow_rgb fg; /* Foreground rgb color. */
  nwindow_rgb bg; /* Background rgb color. */
  short cur_x;    /* Cursor x pos. */
  short cur_y;    /* Cursor y pos. */
  short beg_x;    /* X cordinate of the top-left column in the screen. */
  short beg_y;    /* Y cordinate of the top-left row in screen. */
  short size_x;
  short size_y;
  nwindow_line *line;
};

/* `Internal`  Turn on the `window` rgb colors, if any. */
static void nwindow_enable_rgb_color(nwindow *window, bool enable) {
  if (enable) {
    if (window->fg.enabled) {
      tui_set_foreground_color(window->fg.r, window->fg.g, window->fg.b);
    }
    if (window->bg.enabled) {
      tui_set_background_color(window->bg.r, window->bg.g, window->bg.b);
    }
  }
  else {
    if (window->fg.enabled || window->bg.enabled) {
      tui_reset_color();
    }
  }
}

/* `Internal`  Set a `nwindow_rgb` struct's `enabled` flag to `FALSE` when `encoded` is set to -1, otherwise set the `r,g,b` code based on the encoded data. */
static void nwindow_rgb_set_encoded(nwindow_rgb *rgb, int encoded) {
  if (encoded == -1) {
    rgb->enabled = FALSE;
    return;
  }
  rgb->r = (encoded & 0xff);          /* Red */
  rgb->g = ((encoded >> 8) & 0xff);   /* Green */
  rgb->b = ((encoded >> 16) & 0xff);  /* Blue */
  /* Set the enabled flag so that it is used when printing. */
  rgb->enabled = TRUE;
}

nwindow *nwindow_create(short rows, short columns, short screen_row, short screen_column) {
  nwindow *window = xmalloc(sizeof(*window));
  window->beg_y = screen_row; 
  window->beg_x = screen_column;
  window->cur_x = 0;
  window->cur_y = 0;
  window->size_x = columns;
  window->size_y = rows;
  window->line = xmalloc(sizeof(*window->line) * window->size_y);
  for (short y = 0; y < window->size_y; ++y) {
    window->line[y].data = xmalloc(window->size_x);
    memset(window->line[y].data, ' ', window->size_x);
  }
  window->fg.enabled = FALSE;
  window->bg.enabled = FALSE;
  return window;
}

void nwindow_free(nwindow *window) {
  if (!window) {
    return;
  }
  for (short y = 0; y < window->size_y; ++y) {
    free(window->line[y].data);
  }
  free(window);
  window = NULL;
}

bool nwindow_move(nwindow *window, short row, short column) {
  /* If the passed `window` is invalid, or if the passed row or column is outside the scope of `window`, just return. */
  if (!window || row >= window->size_y || column >= window->size_x || row < 0 || column < 0) {
    errno = EINVAL;
    return FALSE;
  }
  window->cur_x = column;
  window->cur_y = row;
  tui_move((window->beg_y + window->cur_y), (window->beg_x + window->cur_x));
  return TRUE;
}

void nwindow_add_nstr(nwindow *window, const char *string, Ulong len) {
  /* Return early upon bad parameters. */
  if (!window || !string || !len) {
    errno = EINVAL;
    return;
  }
  if (len == (Ulong)-1) {
    len = strlen(string);
  }
  /* If the string length overshots the window size, just print what fits. */
  if ((short)(window->cur_x + len) >= window->size_x) {
    len = (window->size_x - window->cur_x);
  }
  /* Insert the data in the line. */
  memcpy((window->line[window->cur_y].data + window->cur_x), string, len);
  tui_move((window->beg_y + window->cur_y), (window->beg_x + window->cur_x));
  nwindow_enable_rgb_color(window, TRUE);
  nfdwriter_write_stdout(string, len);
  nwindow_enable_rgb_color(window, FALSE);
  window->cur_x += len;
}

void nwindow_add_str(nwindow *window, const char *string) {
  nwindow_add_nstr(window, string, strlen(string));
}

void nwindow_add_ch(nwindow *window, const char c) {
  nwindow_add_nstr(window, &c, 1);
}

void nwindow_printf(nwindow *window, const char *format, ...) {
  if (!window) {
    return;
  }
  char buffer[4096];
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, ap);
  if (len > (int)sizeof(buffer)) {
    len = (int)sizeof(buffer);
  }
  va_end(ap);
  nwindow_add_nstr(window, buffer, len);
}

void nwindow_set_rgb(nwindow *window, int encoded_fg, int encoded_bg) {
  /* If the window is invalid, return early. */
  if (!window) {
    return;
  }
  /* Otherwise set the foreground. */
  nwindow_rgb_set_encoded(&window->fg, encoded_fg);
  /* Then, the background. */
  nwindow_rgb_set_encoded(&window->bg, encoded_bg);
}

void nwindow_set_rgb_code(nwindow *window, Uchar r_fg, Uchar g_fg, Uchar b_fg, Uchar r_bg, Uchar g_bg, Uchar b_bg) {
  /* Just call the encode function, and simply encode the code inline. */
  nwindow_set_rgb(window, (r_fg | (g_fg << 8) | (b_fg << 16)), (r_bg | (g_bg << 8) | (b_bg << 16)));
}

void nwindow_clrtoeol(nwindow *window) {
  if (!window) {
    return;
  }
  short was_cur_x = window->cur_x;
  window->fg.enabled = FALSE;
  window->bg.enabled = FALSE;
  nwindow_printf(window, "%*s", (window->size_x - window->cur_x), " ");
  window->fg.enabled = TRUE;
  window->bg.enabled = TRUE;
  window->cur_x = was_cur_x;
}

void nwindow_redrawl(nwindow *window, short row) {
  if (!window || row >= window->size_y || row < 0) {
    return;
  }
  nwindow_move_add_nstr(window, row, 0, window->line[row].data, window->size_x);
}

void nwindow_redrawln(nwindow *window, short from, short howmeny) {
  if (!window || from >= window->size_y || from < 0) {
    return;
  }
  short was_cur_x = window->cur_x;
  short was_cur_y = window->cur_y;
  while (from < window->size_y) {
    nwindow_redrawl(window, from++);
  }
  window->cur_x = was_cur_x;
  window->cur_y = was_cur_y;
}

void nwindow_scroll(nwindow *window, int howmush) {
  tui_scroll_region(window->beg_x, (window->beg_y + window->size_y));
  if (howmush > 0) {
    nfdwriter_printf(nfdwriter_stdout, "\233%dS", howmush);
  }
  else if (howmush < 0) {
    nfdwriter_printf(nfdwriter_stdout, "\233%dT", (howmush * (-1)));
  }
}
