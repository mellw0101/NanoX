/** @file text.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"

/* Return's the length of whilespace until first non blank char in `string`. */
Ulong indentlen(const char *const restrict string) {
  ASSERT(string);
  const char *ptr = string;
  while (*ptr && isblankc(ptr)) {
    ptr += charlen(ptr);
  }
  return (ptr - string);
}

/* Discard `undo-items` that are newer then `thisitem` in `buffer`, or all if `thisitem` is `NULL`. */
void discard_until_in_buffer(openfilestruct *const buffer, const undostruct *const thisitem) {
  undostruct  *dropit = buffer->undotop;
  groupstruct *group, *next;
  while (dropit && dropit != thisitem) {
    buffer->undotop = dropit->next;
    free(dropit->strdata);
    free(dropit->cutbuffer);
    group = dropit->grouping;
    while (group) {
      next = group->next;
      free_chararray(group->indentations, (group->bottom_line - group->top_line + 1));
      free(group);
      group = next;
    }
    free(dropit);
    dropit = buffer->undotop;
  }
  /* Adjust the pointer to the top of the undo struct. */
  buffer->current_undo = (undostruct *)thisitem;
  /* Prevent a chain of edition actions from continuing. */
  buffer->last_action = OTHER;
}

/* Discard undo items that are newer than the given one, or all if NULL. */
void discard_until(const undostruct *thisitem) {
  discard_until_in_buffer(openfile, thisitem);
}
