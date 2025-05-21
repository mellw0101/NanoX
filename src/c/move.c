/** @file move.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Set `file->current` to `file->filetop`. */
void to_first_line_for(openfilestruct *const file) {
  ASSERT(file);
  file->current     = file->filetop;
  file->current_x   = 0;
  file->placewewant = 0;
  refresh_needed    = TRUE;
}

/* Move to the first line of the currently open file. */
void to_first_line(void) {
  openfile->current     = openfile->filetop;
  openfile->current_x   = 0;
  openfile->placewewant = 0;
  refresh_needed        = TRUE;
}

/* Set `file->current` to `file->filebot`.  */
void to_last_line_for(openfilestruct *const file) {
  ASSERT(file);
  file->current     = file->filebot;
  file->current_x   = (inhelp ? 0 : strlen(file->filebot->data));
  file->placewewant = xplustabs_for(file);
  /* Set the last line of the screen as the target for the cursor. */
  file->cursor_row  = (editwinrows - 1);
  refresh_needed    = TRUE;
  focusing          = FALSE;
  recook |= perturbed;
}

/* Move to the last line of the file. */
void to_last_line(void) {
  openfile->current     = openfile->filebot;
  openfile->current_x   = (inhelp ? 0 : strlen(openfile->filebot->data));
  openfile->placewewant = xplustabs();
  /* Set the last line of the screen as the target for the cursor. */
  openfile->cursor_row  = (editwinrows - 1);
  refresh_needed        = TRUE;
  focusing              = FALSE;
  recook |= perturbed;
}

