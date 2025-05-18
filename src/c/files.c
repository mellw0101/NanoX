/** @file files.c

  @author  Melwin Svensson.
  @date    10-2-2025.

 */
#include "../include/c_proto.h"

/* Add an item to the circular list of openfile structs. */
void make_new_buffer(void) {
  openfilestruct *newnode;
  MALLOC_STRUCT(newnode);
  if (!openfile) {
    /* Make the first buffer the only element in the list. */
    CLIST_INIT(newnode);
    startfile = newnode;
  }
  else {
    /* Add the new buffer after the current one in the list. */
    CLIST_INSERT_AFTER(newnode, openfile);
    /* There is more than one buffer: show "Close" in help lines. */
    exitfunc->tag = close_tag;
    more_than_one = (!inhelp || more_than_one);
  }
  /* Make the new buffer the current one, and start initializing it */
  openfile                = newnode;
  openfile->filename      = COPY_OF("");
  openfile->filetop       = make_new_node(NULL);
  openfile->filetop->data = COPY_OF("");
  openfile->filebot       = openfile->filetop;
  openfile->current       = openfile->filetop;
  openfile->current_x     = 0;
  openfile->placewewant   = 0;
  openfile->cursor_row    = 0;
  openfile->edittop       = openfile->filetop;
  openfile->firstcolumn   = 0;
  openfile->totsize       = 0;
  openfile->modified      = FALSE;
  openfile->spillage_line = NULL;
  openfile->mark          = NULL;
  openfile->softmark      = FALSE;
  openfile->fmt           = UNSPECIFIED;
  openfile->undotop       = NULL;
  openfile->current_undo  = NULL;
  openfile->last_saved    = NULL;
  openfile->last_action   = OTHER;
  openfile->statinfo      = NULL;
  openfile->lock_filename = NULL;
  openfile->errormessage  = NULL;
  openfile->syntax        = NULL;
}

void free_one_buffer(openfilestruct *orphan, openfilestruct **open, openfilestruct **start) {
  /* If the buffer to free is the start buffer, advance the start buffer. */
  if (orphan == *start) {
    *start = (*start)->next;
  }
  CLIST_UNLINK(orphan);
  // if (/* orphan->type.is_set<C_CPP>() || orphan->type.is_set<BASH>() */ orphan->is_c_file || orphan->is_cxx_file || orphan->is_bash_file) {
  //   file_listener.stop_listener(orphan->filename);
  // }
  free(orphan->filename);
  free_lines(orphan->filetop);
  free(orphan->statinfo);
  free(orphan->lock_filename);
  /* Free the undo stack for the orphan file. */
  discard_until_in_buffer(orphan, NULL);
  free(orphan->errormessage);
  /* If the buffer to free is the open buffer, decrament it once. */
  if (orphan == *open) {
    *open = (*open)->prev;
    /* If the buffer to free was the singular and only buffer in the list, set open and start to NULL. */
    if (orphan == *open) {
      *open  = NULL;
      *start = NULL;
    }
  }
  free(orphan);
  /* When just one buffer ramains, set the legacy help bar text for the exit function. */
  if (*open && *open == (*open)->next) {
    exitfunc->tag = exit_tag;
  }
}

/* Remove the current buffer from the circular list of buffers.  When just one buffer remains open, show "Exit" in the help lines. */
void close_buffer(void) {
  openfilestruct *orphan = openfile;
  if (orphan == startfile) {
    startfile = startfile->next;
  }
  CLIST_UNLINK(orphan);
  // if (/* orphan->type.is_set<C_CPP>() || orphan->type.is_set<BASH>() */ orphan->is_c_file || orphan->is_cxx_file || orphan->is_bash_file) {
  //   file_listener.stop_listener(orphan->filename);
  // }
  free(orphan->filename);
  free_lines(orphan->filetop);
  free(orphan->statinfo);
  free(orphan->lock_filename);
  /* Free the undo stack. */
  discard_until(NULL);
  free(orphan->errormessage);
  openfile = orphan->prev;
  if (openfile == orphan) {
    openfile = NULL;
  }
  free(orphan);
  /* When just one buffer remains open, show "Exit" in the help lines. */
  if (openfile && openfile == openfile->next) {
    exitfunc->tag = exit_tag;
  }
}
