/** @file color.c

  @author  Melwin Svensson.
  @date    23-5-2025.

 */
#include "../include/c_proto.h"


/* Initilize the color pairs for `file`. */
void prepare_palette_for(openfilestruct *const file) {
  short number;
  if (ISSET(USING_GUI)) {
    return;
  }
  number = NUMBER_OF_ELEMENTS;
  /* For each unique pair number, tell ncurses to combination of colors. */
  for (colortype *ink=file->syntax->color; ink; ink=ink->next) {
    if (ink->pairnum > number) {
      init_pair(ink->pairnum, ink->fg, ink->bg);
      number = ink->pairnum;
    }
  }
  have_palette = TRUE;
}

/* Initialize the color pairs for the currently open file. */
void prepare_palette(void) {
  if (ISSET(USING_GUI)) {
    return;
  }
  prepare_palette_for(openfile);
}
