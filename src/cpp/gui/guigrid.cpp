/** @file guigrid.cpp

  @author  Melwin Svensson.
  @date    2-1-2025.

 */
#include "../../include/prototypes.h"

// /* Create a x section of the guigrid. */
// guigridsection *make_new_gridsection(void) {
//   guigridsection *section = (guigridsection *)nmalloc(sizeof(*section));
//   section->len = 0;
//   section->cap = 10;
//   section->array = (guieditor **)nmalloc(sizeof(guieditor *) * section->cap);
//   return section;
// }

// /* Resize the internal array of `section` to `newsize`. */
// void gridsection_resize(guigridsection *section, Ulong newsize) {
//   section->array = (guieditor **)nrealloc(section->array, (sizeof(guieditor *) * newsize));
//   section->cap   = newsize;
// }

// void gridsection_add_editor(guigridsection *section, guieditor *editor) {
//   if (section->len == section->cap) {
//     gridsection_resize(section, (section->cap * 2));
//   }
//   section->array[section->len++] = editor;
// }

// guigrid *make_new_grid(void) {
//   guigrid *grid = (guigrid *)nmalloc(sizeof(*grid));
//   grid->len = 0;
//   grid->cap = 10;
//   grid->sections = (guigridsection **)nmalloc(sizeof(guigridsection *) * grid->cap);
//   return grid;
// }
