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
  while (line && row < (editor->rows - 1)) {
    line_cursor_metrics(row, gui->font, &top, &bot);
    if (mousepos.y > (top + editor->text->pos.y) && mousepos.y < (bot + editor->text->pos.y)) {
      break;
    }
    line = line->next;
    ++row;
  }
  return line;
}
