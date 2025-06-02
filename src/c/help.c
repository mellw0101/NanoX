/** @file help.c

  @author  Melwin Svensson.
  @date    2-6-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The point in the help text where the shortcut descriptions begin. */
char *end_of_help_intro = NULL;

/* The point in the help text just after the title. */
const char *start_of_help_body = NULL;

/* The offset (in bytes) of the topleft of the shown help text. */
Ulong help_location;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Hard-wrap the concatenated help text, and write it into a new buffer. */
void wrap_help_text_into_buffer(void) {
  /* Avoid overtight and overwide paragraphs in the introductory text. */
  const char *ptr = start_of_help_body;
  Ulong wrapping_point = (((COLS < 40) ? 40 : (COLS > 74) ? 74 : COLS) - sidebar);
  Ulong sum = 0;
  make_new_buffer();
  /* Ensure there is a blank line at the top of the text, for esthetics. */
  if ((ISSET(MINIBAR) || !ISSET(EMPTY_LINE)) && LINES > 6) {
    openfile->current->data = realloc_strcpy(openfile->current->data, " ");
    openfile->current->next = make_new_node(openfile->current);
    openfile->current       = openfile->current->next;
  }
  /* Copy the help text into the just-created new buffer. */
  while (*ptr) {
    int   length, shim;
    char *oneline;
    if (ptr == end_of_help_intro) {
      wrapping_point = (((COLS < 40) ? 40 : COLS) - sidebar);
    }
    if (ptr < end_of_help_intro || *(ptr - 1) == '\n') {
      length  = break_line(ptr, wrapping_point, TRUE);
      oneline = xmalloc(length + 1);
      shim    = ((*(ptr + length - 1) == ' ') ? 0 : 1);
      snprintf(oneline, (length + shim), "%s", ptr);
    }
    else {
      length  = break_line(ptr, (((COLS < 40) ? 22 : (COLS - 18)) - sidebar), TRUE);
      oneline = xmalloc(length + 5);
      snprintf(oneline, (length + 5), "\t\t  %s", ptr);
    }
    free(openfile->current->data);
    openfile->current->data = oneline;
    ptr += length;
    if (*ptr != '\n') {
      --ptr;
    }
    /* Create a new line, and then one more for each extra \n. */
    do {
      openfile->current->next = make_new_node(openfile->current);
      openfile->current       = openfile->current->next;
      openfile->current->data = COPY_OF("");
    } while (*(++ptr) == '\n');
  }
  openfile->filebot = openfile->current;
  openfile->current = openfile->filetop;
  remove_magicline();
  find_and_prime_applicable_syntax();
  prepare_for_display();
  /* Move to the position in the file where we were before. */
  while (TRUE) {
    sum += strlen(openfile->current->data);
    if (sum > help_location) {
      break;
    }
    openfile->current = openfile->current->next;
  }
  openfile->edittop = openfile->current;
}
