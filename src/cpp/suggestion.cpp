#include "../include/prototypes.h"

char  suggest_buf[1024] = "";
char *suggest_str = NULL;
int   suggest_len = 0;
/* A linked list of the suggest completions. */
static completionstruct *list_of_completions = NULL;

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

/* ----------------------------- Gui suggestmenu ----------------------------- */

/* `INTERNAL`  Clean up the current completions, and reset the state. */
static inline void gui_suggestmenu_free_completions(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  cvec_clear(gui->suggestmenu->completions);
}

/* Init the gui suggestmenu substructure. */
void gui_suggestmenu_create(void) {
  ASSERT(gui);
  gui->suggestmenu = (GuiSuggestMenu *)xmalloc(sizeof(*gui->suggestmenu));
  gui->suggestmenu->flag.active = FALSE;
  gui->suggestmenu->completions = cvec_create_setfree(free);
  gui->suggestmenu->maxrows = 8;
  gui->suggestmenu->buf[0] = '\0';
  gui->suggestmenu->len    = 0;
  gui->suggestmenu->element = make_element_child(gui->root);
  gui->suggestmenu->element->color = GUI_BLACK_COLOR;
  gui->suggestmenu->element->flag.set<GUIELEMENT_ABOVE>();
  gui_element_set_borders(gui->suggestmenu->element, 1, vec4(vec3(0.5), 1));
  gui->suggestmenu->scrollbar = make_element_child(gui->suggestmenu->element);
  move_resize_element(gui->suggestmenu->scrollbar, 10, 10);
  gui->suggestmenu->scrollbar->flag.set<GUIELEMENT_ABOVE>();
  gui->suggestmenu->scrollbar->color = 1;
  gui->suggestmenu->scrollbar->flag.set<GUIELEMENT_REVERSE_RELATIVE_X_POS>();
  gui->suggestmenu->scrollbar->relative_pos.x = gui->suggestmenu->scrollbar->size.w;
  gui->suggestmenu->vertbuf = make_new_font_buffer();
}

/* Free the suggestmenu substructure. */
void gui_suggestmenu_free(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
  ASSERT(gui->suggestmenu->vertbuf);
  gui_suggestmenu_free_completions();
  cvec_free(gui->suggestmenu->completions);
  vertex_buffer_delete(gui->suggestmenu->vertbuf);
  free(gui->suggestmenu);
}

/* Fully clear the suggestions and reset the suggestmenu buffer. */
void gui_suggestmenu_clear(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
  cvec_clear(gui->suggestmenu->completions);
  gui->suggestmenu->buf[0] = '\0';
  gui->suggestmenu->len = 0;
}

/* Load the word cursor is currently on into the suggestmenu buffer, from the cursor to the beginning of the word, if any. */
void gui_suggestmenu_load_str(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
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

/* Comparison function to order all suggestions from shortest to longest string meaning the highest % of the current word has been typed. */
static int cmp(const void *a, const void *b) {
  const char *lhs = *(const char **)a;
  const char *rhs = *(const char **)b;
  long lhs_len = strlen(lhs);
  long rhs_len = strlen(rhs);
  if (lhs_len == rhs_len) {
    return strcmp(lhs, rhs);
  }
  return (lhs_len - rhs_len);
}

/* Copy the word that begins at `*text`. */
static char *gui_suggestmenu_copy_completion(char *const restrict text) {
  Ulong len=0;
  /* Find the end of the word to get the length. */
  while (is_word_char(&text[len], FALSE) || text[len] == '_') {
    len = step_right(text, len);
  }
  /* Return a copy of the word. */
  return measured_copy(text, len);
}

/* Perform the searching throue all openfiles and all lines. */
void gui_suggestmenu_find(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
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
      if (!is_word_char(&search_line->data[i + j], FALSE) && search_line->data[i + j] != '_') {
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
  cvec_qsort(gui->suggestmenu->completions, cmp);
  /* Set the view top and the selected to the first suggestion. */
  gui->suggestmenu->viewtop  = 0;
  gui->suggestmenu->selected = 0;
  TIMER_END(timer, ms);
  TIMER_PRINT(ms);
}

void gui_suggestmenu_run(void) {
  gui_suggestmenu_load_str();
  writef("%s\n", gui->suggestmenu->buf);
  gui_suggestmenu_find();
  if (cvec_len(gui->suggestmenu->completions)) {
    writef("Found completions:\n");
    for (int i=0; i<cvec_len(gui->suggestmenu->completions); ++i) {
      writef("  %s\n", (char *)cvec_get(gui->suggestmenu->completions, i));
    }
  } 
}

/* Calculate the number of visable rows and the final size of the suggestmenu element, then resize it. */
void gui_suggestmenu_resize(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
  vec2 pos, size;
  int len = cvec_len(gui->suggestmenu->completions);
  /* If there are no completions available, just return. */
  if (!len) {
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
  pos.x = cursor_pixel_x_pos(gui->font);
  row_top_bot_pixel((openfile->current->lineno - openfile->edittop->lineno), gui->font, NULL, &pos.y);
  pos.y += openeditor->text->pos.y;
  /* Calculate the size of the suggestmenu window. */
  size.w = (pixbreadth(gui->font, (char *)cvec_get(gui->suggestmenu->completions, (len - 1))) + 4);
  size.h = ((gui->suggestmenu->rows * FONT_HEIGHT(gui->font)) + 4);
  /* Add the size of the scrollbar if there is a scrollbar. */
  if (len > gui->suggestmenu->maxrows) {
    size.w += gui->suggestmenu->scrollbar->size.w;
  }
  /* Move and resize the element. */
  move_resize_element(gui->suggestmenu->element, pos, size);
}

void gui_suggestmenu_update_scrollbar(void) {
  float height;
  float ypos;
  int len = cvec_len(gui->suggestmenu->completions);
  if (len <= gui->suggestmenu->maxrows) {
    gui->suggestmenu->scrollbar->flag.set<GUIELEMENT_HIDDEN>();
    return;
  }
  else {
    gui->suggestmenu->scrollbar->flag.unset<GUIELEMENT_HIDDEN>();
  }
  calculate_scrollbar(
    (gui->suggestmenu->element->size.h - 2),
    0,
    (cvec_len(gui->suggestmenu->completions) - gui->suggestmenu->maxrows),
    gui->suggestmenu->rows,
    gui->suggestmenu->viewtop,
    &height,
    &ypos
  );
  move_resize_element(
    gui->suggestmenu->scrollbar,
    vec2(gui->suggestmenu->scrollbar->pos.x, (gui->suggestmenu->element->pos.y + ypos + 1)),
    vec2(gui->suggestmenu->scrollbar->size.w, height)
  );
}

void gui_suggestmenu_draw_text(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
  int row = 0;
  char *str;
  vec2 textpen;
  vertex_buffer_clear(gui->suggestmenu->vertbuf);
  while (row < gui->suggestmenu->rows) {
    str = (char *)cvec_get(gui->suggestmenu->completions, (gui->suggestmenu->viewtop + row));
    textpen = vec2(
      (gui->suggestmenu->element->pos.x + 2),
      (row_baseline_pixel(row, gui->font) + gui->suggestmenu->element->pos.y + 2)
    );
    vertex_buffer_add_string(gui->suggestmenu->vertbuf, str, strlen(str), NULL, gui->font, 1, &textpen);
    ++row;
  }
  upload_texture_atlas(gui->atlas);
  render_vertex_buffer(gui->font_shader, gui->suggestmenu->vertbuf);
}

void gui_suggestmenu_selected_up(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
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
  }
}

void gui_suggestmenu_selected_down(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
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
  }
}

/* Return's `TRUE` when the suggest menu had entries and one could be injected. */
bool gui_suggestmenu_accept(void) {
  ASSERT(gui);
  ASSERT(gui->suggestmenu);
  ASSERT(gui->suggestmenu->completions);
  char *str;
  if (cvec_len(gui->suggestmenu->completions)) {
    str = (char *)cvec_get(gui->suggestmenu->completions, gui->suggestmenu->selected);
    inject((str + gui->suggestmenu->len), (strlen(str) - gui->suggestmenu->len));
    cvec_clear(gui->suggestmenu->completions);
    refresh_needed = TRUE;
    return TRUE;
  }
  return FALSE;
}
