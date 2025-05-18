/** @file nanox.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* Create a new linestruct node.  Note that we do NOT set 'prevnode->next'. */
linestruct *make_new_node(linestruct *prevnode)  {
  linestruct *newnode = xmalloc(sizeof(*newnode));
  newnode->prev       = prevnode;
  newnode->next       = NULL;
  newnode->data       = NULL;
  newnode->multidata  = NULL;
  newnode->lineno     = ((prevnode) ? (prevnode->lineno + 1) : 1);
  newnode->has_anchor = FALSE;
  // newnode->flags.clear();
  newnode->is_block_comment_start   = FALSE;
  newnode->is_block_comment_end     = FALSE;
  newnode->is_in_block_comment      = FALSE;
  newnode->is_single_block_comment  = FALSE;
  newnode->is_hidden                = FALSE;
  newnode->is_bracket_start         = FALSE;
  newnode->is_in_bracket            = FALSE;
  newnode->is_bracket_end           = FALSE;
  newnode->is_function_open_bracket = FALSE;
  newnode->is_dont_preprocess_line  = FALSE;
  newnode->is_pp_line               = FALSE;
  if (prevnode) {
    (prevnode->is_in_block_comment || prevnode->is_block_comment_start) ? (newnode->is_in_block_comment = TRUE) : ((int)0);
    (prevnode->is_in_bracket || prevnode->is_bracket_start) ? (newnode->is_in_bracket = TRUE) : ((int)0);
    // (prevnode->flags.is_set(IN_BLOCK_COMMENT) || prevnode->flags.is_set(BLOCK_COMMENT_START))
    //   ? newnode->flags.set(IN_BLOCK_COMMENT) : newnode->flags.unset(IN_BLOCK_COMMENT);
    // (prevnode->flags.is_set(IN_BRACKET) || prevnode->flags.is_set(BRACKET_START))
    //   ? newnode->flags.set(IN_BRACKET) : (void)0;
  }
  return newnode;
}

/* Splice a new node into an existing linked list of linestructs. */
void splice_node(linestruct *afterthis, linestruct *newnode) {
  newnode->next = afterthis->next;
  newnode->prev = afterthis;
  if (afterthis->next) {
    afterthis->next->prev = newnode;
  }
  afterthis->next = newnode;
  /* Update filebot when inserting a node at the end of file. */
  if (openfile && openfile->filebot == afterthis) {
    openfile->filebot = newnode;
  }
}

/* Free the data structures in the given node */
void delete_node(linestruct *line) {
  /* If the first line on the screen gets deleted, step one back. */
  if (line == openfile->edittop) {
    openfile->edittop = line->prev;
  }
  /* If the spill-over line for hard-wrapping is deleted... */
  if (line == openfile->spillage_line) {
    openfile->spillage_line = NULL;
  }
  free(line->data);
  free(line->multidata);
  free(line);
}

/* Disconnect a node from a linked list of linestructs and delete it. */
void unlink_node(linestruct *line) {
  if (line->prev) {
    line->prev->next = line->next;
  }
  if (line->next) {
    line->next->prev = line->prev;
  }
  /* Update filebot when removing a node at the end of file. */
  if (openfile && openfile->filebot == line) {
    openfile->filebot = line->prev;
  }
  delete_node(line);
}

/* Free an entire linked list of linestructs. */
void free_lines(linestruct *src) {
  if (!src) {
    return;
  }
  while (src->next) {
    src = src->next;
    delete_node(src->prev);
  }
  delete_node(src);
}

void confirm_margin_for(openfilestruct *const file, int *const out_margin) {
  ASSERT(file);
  bool keep_focus;
  int needed_margin = (digits(file->filebot->lineno) + 1);
  /* When not requested, supress line numbers. */
  if (!ISSET(LINE_NUMBERS)) {
    needed_margin = 0;
  }
  if (needed_margin != (*out_margin)) {
    keep_focus    = (((*out_margin) > 0) && focusing);
    (*out_margin) = needed_margin;
    /* Ensure a proper starting column for the first screen row. */
    ensure_firstcolumn_is_aligned_for(file);
    focusing = keep_focus;
    refresh_needed = TRUE;
  }
}

/* Ensure that the margin can accommodate the buffer's highest line number. */
void confirm_margin(void) {
  bool keep_focus;
  int needed_margin = (digits(openfile->filebot->lineno) + 1);
  /* When not requested or space is too tight, suppress line numbers. */
  if (!ISSET(LINE_NUMBERS) || needed_margin > (COLS - 4)) {
    needed_margin = 0;
  }
  if (needed_margin != margin) {
    keep_focus  = ((margin > 0) && focusing);
    margin      = needed_margin;
    editwincols = (COLS - margin - sidebar);
    /* Ensure a proper starting column for the first screen row. */
    ensure_firstcolumn_is_aligned();
    focusing = keep_focus;
    /* The margin has changed -- schedule a full refresh. */
    refresh_needed = TRUE;
  }
}
