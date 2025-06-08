/** @file gui/statusbar.c

  @author  Melwin Svensson.
  @date    19-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static Statusbar *statusbar = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static void statusbar_timed_msg_internal(message_type type, float seconds, const char *const restrict format, va_list ap) {
  ASSERT(seconds);
  ASSERT(format);
  /* If the statusbar has not been init yet just leave. */
  if (!statusbar) {
    return;
  }
  char *msg;
  va_list copy;
  /* Ignore updates when the type of this message is lower then the currently displayed message. */
  if ((type < statusbar->type) && (statusbar->type > NOTICE)) {
    return;
  }
  va_copy(copy, ap);
  msg = valstr(format, copy, NULL);
  va_end(copy);
  statusbar->type = type;
  statusbar->msg  = free_and_assign(statusbar->msg, msg);
  statusbar->time = seconds;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Create and init the statusbar. */
void statusbar_init(Element *const parent) {
  if (statusbar) {
    return;
  }
  statusbar = xmalloc(sizeof(*statusbar));
  statusbar->text_refresh_needed = TRUE;
  statusbar->msg                 = NULL;
  statusbar->time                = 0;
  statusbar->type                = VACUUM;
  statusbar->buffer              = vertbuf_create();
  statusbar->element             = element_create(0, 0, gui_width, gui_height, FALSE);
  element_set_parent(statusbar->element, parent);
  // color_copy(statusbar->element->color, &color_vs_code_red);
  statusbar->element->color = PACKED_UINT_VS_CODE_RED;
  statusbar->element->hidden                     = TRUE;
  statusbar->element->has_reverse_relative_y_pos = TRUE;
  statusbar->element->has_relative_x_pos         = TRUE;
  statusbar->element->relative_y                 = (gui_font_height(uifont) * 2);
}

/* Free the statusbar structure. */
void statusbar_free(void) {
  if (!statusbar) {
    return;
  }
  free(statusbar->msg);
  vertex_buffer_delete(statusbar->buffer);
  free(statusbar);
  statusbar = NULL;
}

/* Show a status message of `type`, for `seconds`.  Note that the type will determen if the
 * message will be shown, depending if there is a more urgent message already being shown. */
void statusline_gui_timed(message_type type, float seconds, const char *format, ...) {
  ASSERT(seconds);
  ASSERT(format);
  va_list ap;
  va_start(ap, format);
  statusbar_timed_msg_internal(type, seconds, format, ap);
  va_end(ap);
}

void statusline_gui_va(message_type type, const char *const restrict format, va_list ap) {
  ASSERT(format);
  va_list copy;
  va_copy(copy, ap);
  statusbar_timed_msg_internal(type, 2, format, copy);
  va_end(copy);
}

/* Show a status message of `type`, for the default of `2 seconds`.  Note that the type will determen
 * if the message will be shown, depending if there is a more urgent message already being shown. */
void statusline_gui(message_type type, const char *format, ...) {
  ASSERT(format);
  va_list ap;
  va_start(ap, format);
  statusbar_timed_msg_internal(type, 2, format, ap);
  va_end(ap);
}

void statusbar_gui(const char *const restrict msg) {
  statusline_gui(HUSH, "%s", msg);
}

/* Draw the status bar for the gui. */
void statusbar_draw(float fps) {
  float msg_width;
  float x;
  float y;
  if (statusbar->type != VACUUM) {
    /* Check it the message has been shown for the set time. */
    statusbar->time -= (1.0f / fps);
    if (statusbar->time < 0) {
      /* If the set time has elapsed, then reset the status element. */
      statusbar->type            = VACUUM;
      statusbar->element->hidden = TRUE;
      return;
    }
    if (refresh_needed) {
      statusbar->element->hidden = FALSE;
      msg_width = (font_breadth(uifont, statusbar->msg) + font_breadth(uifont, "  "));
      vertex_buffer_clear(statusbar->buffer);
      element_move_resize(statusbar->element, ((gui_width / 2) - (msg_width / 2)), (gui_height - (gui_font_height(uifont) * 2)), msg_width, gui_font_height(uifont));
      x = (statusbar->element->x + font_breadth(uifont, " "));
      y = (statusbar->element->y + gui_font_row_baseline(uifont, 0));
      font_vertbuf_add_mbstr(uifont, statusbar->buffer, statusbar->msg, strlen(statusbar->msg), " ", PACKED_UINT(255, 255, 255, 255), &x, &y);
    }
    element_draw(statusbar->element);
    render_vertbuf(uifont, statusbar->buffer);
  }
}
