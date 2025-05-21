/** @file line.c

  @author  Melwin Svensson.
  @date    21-5-2025.

 */
#include "../include/c_proto.h"


/* Return 'TRUE' when 'line' is part of the marked region. */
bool line_in_marked_region_for(openfilestruct *const file, linestruct *const line) {
  ASSERT(file);
  ASSERT(line);
  return (file->mark
   && ((line->lineno >= file->mark->lineno && line->lineno <= file->current->lineno)
   || (line->lineno <= file->mark->lineno && line->lineno >= file->current->lineno)));
}

/* Return 'TRUE' when 'line' is part of the marked region. */
bool line_in_marked_region(linestruct *const line) {
  return line_in_marked_region_for(openfile, line);
}
