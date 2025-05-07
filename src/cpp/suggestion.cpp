#include "../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_SUGGEST_MENU               \
  ASSERT(gui);                            \
  ASSERT(gui->suggestmenu);               \
  ASSERT(gui->suggestmenu->completions);  \
  ASSERT(gui->suggestmenu->element);      \
  ASSERT(gui->suggestmenu->sb);           \
  ASSERT(gui->suggestmenu->vertbuf)


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


char  suggest_buf[1024] = "";
char *suggest_str = NULL;
int   suggest_len = 0;
/* A linked list of the suggest completions. */
static completionstruct *list_of_completions = NULL;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


void do_suggestion(void) {
  PROFILE_FUNCTION;
  /* For now only c/cpp is supported. */
  if (!openfile->type.is_set<C_CPP>()) {
    return;
  }
  suggest_str = NULL;
  if (suggestwin) {
    delwin(suggestwin);
    suggestwin = NULL;
  }
  if (!openfile->current_x || (!is_word_char(&openfile->current->data[openfile->current_x - 1], FALSE) && openfile->current->data[openfile->current_x - 1] != '_')) {
    clear_suggestion();
    return;
  }
  if (suggest_len < 0) {
    clear_suggestion();
  }
  add_char_to_suggest_buf();
  find_suggestion();
  draw_suggest_win();
}

/* Search for any match to the current suggest buf. */
void find_suggestion(void) {
  /* Reset the completion list. */
  while (list_of_completions) {
    completionstruct *dropit = list_of_completions;
    list_of_completions = list_of_completions->next;
    free(dropit->word);
    free(dropit);
  }
  /* Start with the currently open file.  At the first line. */
  openfilestruct *currentfile = openfile;
  linestruct *search_line = openfile->filetop;
  /* The position in the 'search_line' of the last found completion. */
  int search_x = 0;
  /* There is currently no suggest word. */
  if (!suggest_len) {
    search_line = NULL;
    return;
  }
  /* Search all lines in the 'currentfile'. */
  while (search_line) {
    completionstruct *some_word;
    long threshhold = (strlen(search_line->data) - suggest_len - 1);
    char *completion;
    Ulong i, j;
    /* Treverse the whole line. */
    for (i=search_x; (long)i<threshhold; ++i) {
      /* If the first byte does not match, move on. */
      if (search_line->data[i] != suggest_buf[0]) {
        continue;
      }
      /* Check the rest of the bytes. */
      for (j=1; (int)j<suggest_len; ++j) {
        if (search_line->data[i + j] != suggest_buf[j]) {
          break;
        }
      }
      /* Continue searching if all bytes did not match. */
      if ((int)j < suggest_len) {
        continue;
      }
      /* If the match is an exact copy of 'suggest_buf', skip it. */
      if (!is_word_char(&search_line->data[i + j], FALSE)) {
        continue;
      }
      /* If the match is not a seperate word, skit it. */
      if (i > 0 && is_word_char(&search_line->data[step_left(search_line->data, i)], FALSE)) {
        continue;
      }
      /* If the match is the 'suggest_buf' itself, ignore it. */
      if (search_line == openfile->current && i == (openfile->current_x - suggest_len)) {
        continue;
      }
      completion = copy_completion(search_line->data + i);
      /* Look for duplicates in the already found completions. */
      some_word = list_of_completions;
      while (some_word && strcmp(some_word->word, completion) != 0) {
        some_word = some_word->next;
      }
      /* If we already found this word, skip it. */
      if (some_word) {
        free(completion);
        continue;
      }
      /* Add the found word to the list of completions. */
      some_word = (completionstruct *)nmalloc(sizeof(*some_word));
      some_word->word = completion;
      some_word->next = list_of_completions;
      list_of_completions = some_word;
      search_x = ++i;
    }
    search_line = search_line->next;
    search_x = 0;
    if (!search_line && currentfile->next != openfile) {
      currentfile = currentfile->next;
      search_line = currentfile->filetop;
    }
  }
  if (list_of_completions) {
    NETLOG("Found completions: \n");
    completionstruct *completion = list_of_completions;
    while (completion) {
      NETLOG("  %s\n", completion->word);
      completion = completion->next;
    }
    NETLOG("\n");
  }
  else {
    NETLOG("Found no completions.\n");
  }
}

/* Clear suggest buffer and len as well as setting the current suggest str to NULL. */
void clear_suggestion(void) {
  suggest_on  = FALSE;
  suggest_str = NULL;
  suggest_len = 0;
  suggest_buf[suggest_len] = '\0';
}

/* Add last typed char to the suggest buffer. */
void add_char_to_suggest_buf(void) {
  if (openfile->current_x > 0) {
    suggest_len = 0;
    Ulong pos = get_prev_cursor_word_start_index();
    while (pos < openfile->current_x) {
      suggest_buf[suggest_len++] = openfile->current->data[pos++];
    }
    suggest_buf[suggest_len] = '\0';
  }
}

/* Draw current suggestion if found to the suggest window. */
void draw_suggest_win(void) {
  if (list_of_completions) {
    if (ISSET(SUGGEST_INLINE)) {
      RENDR(SUGGEST, list_of_completions->word);
      return;
    }
    else {
      completionstruct *completion;
      Ulong width, height = 0, posx, posy, wordlen, top_margin, bot_margin, count = 0;
      bool list_up;
      top_margin = openfile->cursor_row;
      bot_margin = (editwinrows - openfile->cursor_row - 1);
      completion = list_of_completions;
      while (completion) {
        ++count;
        completion = completion->next;
      }
      /* Orient the list where it fits the best. */
      if (top_margin > bot_margin && count > bot_margin) {
        list_up = TRUE;
      }
      else {
        list_up = FALSE;
      }
      /* Calculate height. */
      if (list_up) {
        if (count > top_margin) {
          height = top_margin;
        }
        else {
          height = count;
        }
      }
      else {
        if (count > bot_margin) {
          height = bot_margin;
        }
        else {
          height = count;
        }
      }
      completion = list_of_completions;
      /* Calculate width and height of the entire window. */
      for (Uint i = 0; i < height; ++i) {
        wordlen = (breadth(completion->word) + 2);
        if (wordlen > width) {
          width = wordlen;
        }
        completion = completion->next;
      }
      posx = (xplustabs() + margin - suggest_len - 1);
      posy = (!list_up ? (openfile->cursor_row + 1) : (openfile->cursor_row - height));
      suggestwin = newwin(height, width, posy, posx);
      completion = list_of_completions;
      /* Render the list in the correct order, starting with the first completion closest to the cursor. */
      if (list_up) {
        for (Ulong i = (height - 1); completion && i >= 0; --i) {
          mvwprintw(suggestwin, i, 1, "%s", completion->word);
          completion = completion->next;
        }
      }
      else {
        for (Ulong i = 0; completion && i < height; ++i) {
          mvwprintw(suggestwin, i, 1, "%s", completion->word);
          completion = completion->next;
        }
      }
      wrefresh(suggestwin);
    }
  }
}

/* Inject a suggestion. */
void accept_suggestion(void) {
  if (suggest_str) {
    inject((suggest_str + suggest_len), (strlen(suggest_str) - suggest_len));
  }
  clear_suggestion();
}


/* ---------------------------------------------------------- Gui suggestmenu ---------------------------------------------------------- */


/* ----------------------------- Static helper function's. ----------------------------- */

/* `INTERNAL`  Clean up the current completions, and reset the state. */
static inline void gui_suggestmenu_free_completions(void) {
  ASSERT_SUGGEST_MENU;
  cvec_clear(gui->suggestmenu->completions);
}

/* Set (`TRUE`) or unset (`FALSE`) the hidden flag for the suggestmenu element and scrollbar. */
static inline void gui_suggestmenu_hide(bool hide) {
  ASSERT_SUGGEST_MENU;
  guielement_set_flag_recurse(gui->suggestmenu->element, hide, GUIELEMENT_HIDDEN);
}

/* Copy the word that begins at `*text`. */
static char *gui_suggestmenu_copy_completion(char *const restrict text) {
  Ulong len = 0;
  /* Find the end of the word to get the length. */
  while (iswordc(&text[len], FALSE, "_")) {
    len = step_right(text, len);
  }
  /* Return a copy of the word. */
  return measured_copy(text, len);
}

static void gui_suggestmenu_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  GuiSuggestMenu *sm = (__TYPE(sm))arg;
  ASSIGN_IF_VALID(total_length, (sm->element->size.h - 2));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, cvec_len(sm->completions) - sm->rows);
  ASSIGN_IF_VALID(visible, sm->rows);
  ASSIGN_IF_VALID(current, sm->viewtop);
  ASSIGN_IF_VALID(top_offset, 1);
  ASSIGN_IF_VALID(right_offset, 1);
}

static void gui_suggestmenu_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  GuiSuggestMenu *sm = (__TYPE(sm))arg;
  sm->viewtop = index;
}

/* ----------------------------- Global function's ----------------------------- */

/* Init the gui suggestmenu substructure. */
void gui_suggestmenu_create(void) {
  ASSERT(gui);
  MALLOC_STRUCT(gui->suggestmenu);
  gui->suggestmenu->text_refresh_needed = TRUE;
  gui->suggestmenu->pos_refresh_needed  = TRUE;
  gui->suggestmenu->completions = cvec_create_setfree(free);
  gui->suggestmenu->maxrows = 8;
  gui->suggestmenu->buf[0] = '\0';
  gui->suggestmenu->len    = 0;
  gui->suggestmenu->element = guielement_create(gui->root);
  gui->suggestmenu->element->color = GUI_BLACK_COLOR;
  gui->suggestmenu->element->flag.set<GUIELEMENT_ABOVE>();
  guielement_set_borders(gui->suggestmenu->element, 1, vec4(vec3(0.5f), 1.0f));
  gui->suggestmenu->vertbuf = make_new_font_buffer();
  gui->suggestmenu->sb      = guiscrollbar_create(gui->suggestmenu->element, gui->suggestmenu, gui_suggestmenu_scrollbar_update_routine, gui_suggestmenu_scrollbar_moving_routine);
}

/* Free the suggestmenu substructure. */
void gui_suggestmenu_free(void) {
  ASSERT_SUGGEST_MENU;
  gui_suggestmenu_free_completions();
  cvec_free(gui->suggestmenu->completions);
  vertex_buffer_delete(gui->suggestmenu->vertbuf);
  free(gui->suggestmenu->sb);
  free(gui->suggestmenu);
}

/* Fully clear the suggestions and reset the suggestmenu buffer. */
void gui_suggestmenu_clear(void) {
  ASSERT_SUGGEST_MENU;
  cvec_clear(gui->suggestmenu->completions);
  gui->suggestmenu->buf[0] = '\0';
  gui->suggestmenu->len = 0;
  gui_suggestmenu_hide(TRUE);
}

/* Load the word cursor is currently on into the suggestmenu buffer, from the cursor to the beginning of the word, if any. */
void gui_suggestmenu_load_str(void) {
  ASSERT_SUGGEST_MENU;
  Ulong pos;
  /* Ensure we clear the buffer every time. */
  gui->suggestmenu->buf[0] = '\0';
  if (openfile->current_x > 0 && openfile->current_x < 128) {
    gui->suggestmenu->len = 0;
    pos = get_prev_cursor_word_start_index(TRUE);
    while (pos < openfile->current_x) {
      gui->suggestmenu->buf[gui->suggestmenu->len++] = openfile->current->data[pos++];
    }
    gui->suggestmenu->buf[gui->suggestmenu->len] = '\0';
  }
}

/* Perform the searching throue all openfiles and all lines. */
void gui_suggestmenu_find(void) {
  ASSERT_SUGGEST_MENU;
  TIMER_START(timer);
  /* We use our hash map for fast lookup. */
  HashMap *hash_map;
  openfilestruct *current_file;
  linestruct *search_line;
  int search_x;
  long threshhold;
  char *completion;
  Ulong i, j;
  gui_suggestmenu_free_completions();
  if (!gui->suggestmenu->len) {
    return;
  }
  hash_map = hashmap_create();
  current_file = openfile;
  search_line  = current_file->filetop;
  search_x     = 0;
  while (search_line) {
    threshhold = (strlen(search_line->data) - gui->suggestmenu->len - 1);
    /* Go thrue whole line. */
    for (i=search_x; (long)i<threshhold; ++i) {
      /* If the first byte does not match, move on. */
      if (search_line->data[i] != gui->suggestmenu->buf[0]) {
        continue;
      }
      /* When it does match, check the rest of the bytes. */
      for (j=1; (int)j<gui->suggestmenu->len; ++j) {
        if (search_line->data[i + j] != gui->suggestmenu->buf[j]) {
          break;
        }
      }
      /* Continue if all bytes did not match. */
      if ((int)j < gui->suggestmenu->len) {
        continue;
      }
      /* Or the match is an exact copy of `gui->suggestmenu->buf`. */
      if (!iswordc(&search_line->data[i + j], FALSE, "_")) {
        continue;
      }
      /* Or the match is not a seperate word. */
      if (i > 0 && is_word_char(&search_line->data[step_left(search_line->data, i)], FALSE)) {
        continue;
      }
      /* Or the match is the `gui->suggestmenu->buf` itself. */
      if (search_line == openfile->current && i == (openfile->current_x - gui->suggestmenu->len)) {
        continue;
      }
      completion = gui_suggestmenu_copy_completion(search_line->data + i);
      /* Look for duplicates in the already found completions. */
      if (hashmap_get(hash_map, completion)) {
        free(completion);
        continue;
      }
      /* Add to the hashmap, using the ptr to the word as it will live longer then this hashmap. */
      hashmap_insert(hash_map, completion, (void *)completion);
      cvec_push(gui->suggestmenu->completions, completion);
      search_x = ++i;
    }
    search_line = search_line->next;
    search_x = 0;
    if (!search_line && current_file->next != openfile) {
      current_file = current_file->next;
      search_line = current_file->filetop;
    }
  }
  hashmap_free(hash_map);
  cvec_qsort(gui->suggestmenu->completions, qsort_strlen);
  /* Set the view top and the selected to the first suggestion. */
  gui->suggestmenu->viewtop  = 0;
  gui->suggestmenu->selected = 0;
  TIMER_END(timer, ms);
  TIMER_PRINT(ms);
}

void gui_suggestmenu_run(void) {
  ASSERT_SUGGEST_MENU;
  gui_suggestmenu_load_str();
  // writef("%s\n", gui->suggestmenu->buf);
  gui_suggestmenu_find();
  if (cvec_len(gui->suggestmenu->completions)) {
    gui_suggestmenu_hide(FALSE);
    guiscrollbar_refresh_needed(gui->suggestmenu->sb);
    gui->suggestmenu->text_refresh_needed = TRUE;
    gui->suggestmenu->pos_refresh_needed = TRUE;
    // writef("Found completions:\n");
    // for (int i=0; i<cvec_len(gui->suggestmenu->completions); ++i) {
    //   writef("  %s\n", (char *)cvec_get(gui->suggestmenu->completions, i));
    // }
  } 
}

/* Calculate the number of visable rows and the final size of the suggestmenu element, then resize it. */
void gui_suggestmenu_resize(void) {
  ASSERT_SUGGEST_MENU;
  vec2 pos, size;
  int len = cvec_len(gui->suggestmenu->completions);
  /* If there are no completions available or we dont need to recalculate the position, just return. */
  if (!len || !gui->suggestmenu->pos_refresh_needed) {
    return;
  }
  /* Set the number of visable rows. */
  if (len > gui->suggestmenu->maxrows) {
    gui->suggestmenu->rows = gui->suggestmenu->maxrows;
  }
  else {
    gui->suggestmenu->rows = len;
  }
  /* Calculate the correct position for the suggestmenu window. */
  pos.x = cursor_pixel_x_pos(gui_font_get_font(gui->font));
  gui_font_row_top_bot(gui->font, (openfile->current->lineno - openfile->edittop->lineno), NULL, &pos.y);
  pos.y += openeditor->text->pos.y;
  /* Calculate the size of the suggestmenu window. */
  size.w = (pixbreadth(gui_font_get_font(gui->font), (char *)cvec_get(gui->suggestmenu->completions, (len - 1))) + 4);
  size.h = ((gui->suggestmenu->rows * gui_font_height(gui->font) /* FONT_HEIGHT(gui_font_get_font(gui->font)) */) + 4);
  /* Add the size of the scrollbar if there is a scrollbar. */
  if (len > gui->suggestmenu->maxrows) {
    size.w += guiscrollbar_width(gui->suggestmenu->sb);
  }
  /* Move and resize the element. */
  guielement_move_resize(gui->suggestmenu->element, pos, size);
  gui->suggestmenu->pos_refresh_needed = FALSE;
}

/* When the selected entry of the suggestmenu inside the viewport of the suggestmenu, highlight it. */
void gui_suggestmenu_draw_selected(void) {
  ASSERT_SUGGEST_MENU;
  int selected_row = (gui->suggestmenu->selected - gui->suggestmenu->viewtop);
  vec2 pos, size;
  /* Draw the selected entry, if its on screen. */
  if (selected_row >= 0 && selected_row < gui->suggestmenu->rows) {
    pos.x  = (gui->suggestmenu->element->pos.x + 1);
    size.w = (gui->suggestmenu->element->size.w - 2);
    gui_font_row_top_bot(gui->font, selected_row, &pos.y, &size.h);
    size.h -= (pos.y - ((selected_row == gui->suggestmenu->rows - 1) ? 1 : 0));
    pos.y  += (gui->suggestmenu->element->pos.y + 1);
    draw_rect(pos, size, vec4(vec3(1.0f), 0.4f));
  }
}

/* Draw the correct text in the suggestmenu based on the current viewtop of the suggestmenu. */
void gui_suggestmenu_draw_text(void) {
  ASSERT_SUGGEST_MENU;
  static int was_viewtop = gui->suggestmenu->viewtop;
  int row = 0;
  char *str;
  vec2 textpen;
  /* Only clear and reconstruct the vertex buffer when asked or when the viewtop has changed. */
  if (gui->suggestmenu->text_refresh_needed || was_viewtop != gui->suggestmenu->viewtop) {
    vertex_buffer_clear(gui->suggestmenu->vertbuf);
    while (row < gui->suggestmenu->rows) {
      str = (char *)cvec_get(gui->suggestmenu->completions, (gui->suggestmenu->viewtop + row));
      textpen = vec2(
        (gui->suggestmenu->element->pos.x + 2),
        (gui_font_row_baseline(gui->font, row) + gui->suggestmenu->element->pos.y + 2)
      );
      vertex_buffer_add_string(gui->suggestmenu->vertbuf, str, strlen(str), NULL, gui_font_get_font(gui->font), 1, &textpen);
      ++row;
    }
    was_viewtop = gui->suggestmenu->viewtop;
    gui->suggestmenu->text_refresh_needed = FALSE;
  }
  upload_texture_atlas(gui_font_get_atlas(gui->font));
  render_vertex_buffer(gui->font_shader, gui->suggestmenu->vertbuf);
}

/* Move the currently selected suggestmenu entry up once or when at the very top move it to the bottom.  This also ensures that viewtop moves as well when moving from the top of it. */
void gui_suggestmenu_selected_up(void) {
  ASSERT_SUGGEST_MENU;
  int len = cvec_len(gui->suggestmenu->completions);
  if (len) {
    /* If we are at the first entry. */
    if (gui->suggestmenu->selected == 0) {
      gui->suggestmenu->selected = (len - 1);
      if (len > gui->suggestmenu->maxrows) {
        gui->suggestmenu->viewtop = (len - gui->suggestmenu->maxrows);
      }
    }
    else {
      if (gui->suggestmenu->viewtop == gui->suggestmenu->selected) {
        --gui->suggestmenu->viewtop;
      }
      --gui->suggestmenu->selected;
    }
    guiscrollbar_refresh_needed(gui->suggestmenu->sb);
  }
}

void gui_suggestmenu_selected_down(void) {
  ASSERT_SUGGEST_MENU;
  int len = cvec_len(gui->suggestmenu->completions);
  if (len) {
    /* If we are at the last entry. */
    if (gui->suggestmenu->selected == (len - 1)) {
      gui->suggestmenu->selected = 0;
      gui->suggestmenu->viewtop  = 0;
    }
    else {
      if (gui->suggestmenu->selected == (gui->suggestmenu->viewtop + gui->suggestmenu->maxrows - 1)) {
        ++gui->suggestmenu->viewtop;
      }
      ++gui->suggestmenu->selected;
    }
    guiscrollbar_refresh_needed(gui->suggestmenu->sb);
  }
}

/* Return's `TRUE` when the suggest menu had entries and one could be injected. */
bool gui_suggestmenu_accept(void) {
  ASSERT_SUGGEST_MENU;
  char *str;
  if (cvec_len(gui->suggestmenu->completions)) {
    openfile->last_action = OTHER;
    str = (char *)cvec_get(gui->suggestmenu->completions, gui->suggestmenu->selected);
    inject((str + gui->suggestmenu->len), (strlen(str) - gui->suggestmenu->len));
    gui_suggestmenu_clear();
    refresh_needed = TRUE;
    return TRUE;
  }
  return FALSE;
}

void gui_suggestmenu_hover_action(float y_pos) {
  ASSERT_SUGGEST_MENU;
  ASSERT(gui->font);
  long row;
  float top;
  float bot;
  if (cvec_len(gui->suggestmenu->completions)) {
    /* Top of the completions menu. */
    top = (gui->suggestmenu->element->pos.y);
    /* Bottom of the completions menu. */
    bot = (gui->suggestmenu->element->pos.y + gui->suggestmenu->element->size.h);
    /* If `y_pos` relates to a valid row in the suggestmenu completion menu, then adjust the selected to that row. */
    if (gui_font_row_from_pos(gui->font, top, bot, y_pos, &row)) {
      gui->suggestmenu->selected = lclamp((gui->suggestmenu->viewtop + row), gui->suggestmenu->viewtop, (gui->suggestmenu->viewtop + gui->suggestmenu->rows));
    }
  }
}

void gui_suggestmenu_scroll_action(bool direction, float y_pos) {
  ASSERT_SUGGEST_MENU;
  int len = cvec_len(gui->suggestmenu->completions);
  /* Only do anything if there are more entries then rows in the suggestmenu. */
  if (len > gui->suggestmenu->maxrows) {
    /* Only scroll when not already at the top or bottom. */
    if ((direction == BACKWARD && gui->suggestmenu->viewtop > 0) || (direction == FORWARD && gui->suggestmenu->viewtop < (len - gui->suggestmenu->maxrows))) {
      gui->suggestmenu->viewtop += (!direction ? -1 : 1);
      gui->suggestmenu->text_refresh_needed = TRUE;
      /* Ensure that the currently selected entry gets correctly set based on where the mouse is. */
      gui_suggestmenu_hover_action(y_pos);
      guiscrollbar_refresh_needed(gui->suggestmenu->sb);
    }
  }
}

void gui_suggestmenu_click_action(float y_pos) {
  ASSERT_SUGGEST_MENU;
  ASSERT(gui->font);
  float top;
  float bot;
  /* Only perform any action when there are completions available. */
  if (cvec_len(gui->suggestmenu->completions)) {
    top = gui->suggestmenu->element->pos.y;
    bot = (gui->suggestmenu->element->pos.y + gui->suggestmenu->element->size.h);
    if (y_pos >= top && y_pos <= bot) {
      gui_suggestmenu_hover_action(y_pos);
      gui_suggestmenu_accept();
    }
  }
}
