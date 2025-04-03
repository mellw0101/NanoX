/** @file gui/cursor/cursor.cpp

  @author  Melwin Svensson.
  @date    27-3-2025.

 */
#include "../../../include/prototypes.h"


linestruct *line_from_cursor_pos(guieditor *const editor) {
  ASSERT(editor);
  Uint row = 0;
  float top, bot;
  linestruct *line = editor->openfile->edittop;
  /* If the mouse y position is above the text of the editor return the topline. */
  if (mousepos.y < editor->text->pos.y) {
    return line;
  }
  /* Check if the mouse y position is within any line in the text window. */
  while (line->next && row < (editor->rows - 1)) {
    row_top_bot_pixel(row, gui->font, &top, &bot);
    /* If the mouse position falls within the position of the current line, then break. */
    if (mousepos.y > (top + editor->text->pos.y) && mousepos.y < (bot + editor->text->pos.y)) {
      break;
    }
    line = line->next;
    ++row;
  }
  return line;
}


/** TODO: Make these three into actualy intuative functions and remake them for a more clean operation.  This was done
 * in 20 min just to test and its already perfect as it just cannot preduce the problems we had with the old system. */


float *pixpositions(const char *const restrict string, float normx, Ulong *outlen, texture_font_t *const font) {
  ASSERT(string);
  Ulong len=strlen(string), i;
  float *array, start=normx, end=normx;
  const char *current, *prev=NULL;
  if (!len) {
    return NULL;
  }
  array = (float *)xmalloc((len + 1) * sizeof(float));
  for (i=0; i<len; ++i) {
    start = end;
    current = &string[i];
    if (*current == '\t') {
      current = " ";
      end += (glyph_width(current, prev, font) * tabsize);
    }
    else {
      end += glyph_width(current, prev, font);
    }
    array[i] = start;
    prev = current;
  }
  array[len++] = end;
  ASSIGN_IF_VALID(outlen, len);
  return array;
}

Ulong closest_index(float *array, Ulong len, float rawx, texture_font_t *const font) {
  ASSERT(array);
  ASSERT(len);
  Ulong index=0;
  float closest_value=(array[0] - rawx), value;
  if (closest_value < 0) {
    closest_value *= -1;
  }
  for (Ulong i=1; i<len; ++i) {
    value = (array[i] - rawx);
    if (value < 0) {
      value *= -1;
    }
    if (value < closest_value) {
      index = i;
      closest_value = value;
    }
  }
  return index;
}

Ulong index_from_pix_xpos(const char *const restrict string, float rawx, float normx, texture_font_t *const font) {
  ASSERT(string);
  ASSERT(font);
  Ulong len;
  float *array = strpx_array(string, strlen(string), normx, &len, font);
  Ulong index = strpx_array_index(array, len, rawx);
  free(array);
  return index;
}

/** TODO: Test this and see witch other ways exists. */

void set_cursor_type(GLFWwindow *window, int type) {
  static GLFWcursor *arrow=NULL, *ibeam=NULL, *crosshair=NULL, *hand=NULL;
  GLFWcursor *cursor=NULL;
  switch (type) {
    case GLFW_ARROW_CURSOR: {
      if (!arrow) {
        arrow = glfwCreateStandardCursor(type);
      }
      cursor = arrow;
      break;
    }
    case GLFW_IBEAM_CURSOR: {
      if (!ibeam) {
        ibeam = glfwCreateStandardCursor(type);
      }
      cursor = ibeam;
      break;
    }
    case GLFW_CROSSHAIR_CURSOR: {
      if (!crosshair) {
        crosshair = glfwCreateStandardCursor(type);
      }
      cursor = crosshair;
      break;
    }
    case GLFW_HAND_CURSOR: {
      if (!hand) {
        hand = glfwCreateStandardCursor(type);
      }
      cursor = hand;
      break;
    }
    default: {
      die("%s: Unknown cursor passed.\n");
    }
  }
  glfwSetCursor(window, cursor);
}
