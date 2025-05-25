#include "../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_SUGGEST_MENU               \
  ASSERT(gui);                            \
  ASSERT(gui->suggestmenu);               \
  ASSERT(gui->suggestmenu->menu)


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
  if (/* !openfile->type.is_set<C_CPP>() */ !openfile->is_c_file && !openfile->is_cxx_file) {
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

/* Copy the word that begins at `*text`. */
static char *gui_suggestmenu_copy_completion(char *const restrict text) {
  Ulong len = 0;
  /* Find the end of the word to get the length. */
  while (is_word_char(&text[len], FALSE) || text[len] == '_') {
    len = step_right(text, len);
  }
  /* Return a copy of the word. */
  return measured_copy(text, len);
}

static void gui_suggestmenu_pos_routine(void *arg, float width, float height, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(x);
  ASSERT(y);
  SuggestMenu *sm = (__TYPE(sm))arg;
  /* Calculate the correct position for the suggestmenu window. */
  (*x) = (openeditor->text->x + font_wideness(menu_get_font(sm->menu), openeditor->openfile->current->data, openeditor->openfile->current_x));
  (*y) = (openeditor->text->y + font_row_bottom_pix(menu_get_font(sm->menu), (openeditor->openfile->current->lineno - openeditor->openfile->edittop->lineno)));
}

static void gui_suggestmenu_accept_routine(void *arg, const char *const restrict lable, int index) {
  ASSERT(arg);
  ASSERT(lable);
  SuggestMenu *sm = (__TYPE(sm))arg;
  char *str = copy_of(lable);
  openfile->last_action = OTHER;
  inject((str + sm->len), (strlen(str) - sm->len));
  free(str);
  gui_suggestmenu_clear();
  refresh_needed = TRUE;
}

/* ----------------------------- Global function's ----------------------------- */

/* Init the gui suggestmenu substructure. */
void gui_suggestmenu_create(void) {
  ASSERT(gui);
  MALLOC_STRUCT(gui->suggestmenu);
  gui->suggestmenu->menu = menu_create(gui->root, gui->font, gui->suggestmenu, gui_suggestmenu_pos_routine, gui_suggestmenu_accept_routine);
  menu_set_tab_accept_behavior(gui->suggestmenu->menu, TRUE);
  menu_set_arrow_depth_navigation(gui->suggestmenu->menu, FALSE);
  gui->suggestmenu->buf[0] = '\0';
  gui->suggestmenu->len    = 0;
}

/* Free the suggestmenu substructure. */
void gui_suggestmenu_free(void) {
  ASSERT_SUGGEST_MENU;
  menu_free(gui->suggestmenu->menu);
  free(gui->suggestmenu);
}

/* Fully clear the suggestions and reset the suggestmenu buffer. */
void gui_suggestmenu_clear(void) {
  ASSERT_SUGGEST_MENU;
  gui->suggestmenu->buf[0] = '\0';
  gui->suggestmenu->len = 0;
  menu_clear_entries(gui->suggestmenu->menu);
  menu_show(gui->suggestmenu->menu, FALSE);
}

/* Load the word cursor is currently on into the suggestmenu buffer, from the cursor to the beginning of the word, if any. */
void gui_suggestmenu_load_str(void) {
  ASSERT_SUGGEST_MENU;
  Ulong pos;
  /* Ensure we clear the buffer every time. */
  gui->suggestmenu->buf[0] = '\0';
  if (openeditor->openfile->current_x > 0 && openeditor->openfile->current_x < 128) {
    gui->suggestmenu->len = 0;
    pos = get_prev_cursor_word_start_index(TRUE);
    while (pos < openeditor->openfile->current_x) {
      gui->suggestmenu->buf[gui->suggestmenu->len++] = openeditor->openfile->current->data[pos++];
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
  menu_clear_entries(gui->suggestmenu->menu);
  if (!gui->suggestmenu->len) {
    return;
  }
  hash_map = hashmap_create_wfreefunc(free);
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
      menu_push_back(gui->suggestmenu->menu, completion);
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
  menu_qsort(gui->suggestmenu->menu, menu_entry_qsort_strlen_cb);
  TIMER_END(timer, ms);
  TIMER_PRINT(ms);
}

void gui_suggestmenu_run(void) {
  ASSERT_SUGGEST_MENU;
  gui_suggestmenu_load_str();
  gui_suggestmenu_find();
  if (menu_len(gui->suggestmenu->menu)) {
    menu_show(gui->suggestmenu->menu, TRUE);
  }
  else {
    menu_show(gui->suggestmenu->menu, FALSE);
  }
}
