/** @file suggestion.c

  @author  Melwin Svensson.
  @date    25-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_SM          \
  ASSERT(suggest);         \
  ASSERT(suggest->menu)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  /* Menu used to display the available suggestions. */
  Menu *menu;
  /* Buffer holding the current string used to get suggestions. */
  char *buffer;
  /* Length of the current string used to search for suggestions. */
  Ulong length;
} Suggest/* Menu */;


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


static completionstruct *list_of_completions = NULL;

static Suggest *suggest = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Suggestmenu pos ----------------------------- */

static void suggestmenu_pos(void *arg, float _UNUSED w, float _UNUSED h, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(x);
  ASSERT(y);
  Suggest *s = arg;
  /* Calculate the correct position for the suggest-menu element. */
  (*x) = (openeditor->text->x + font_wideness(menu_get_font(s->menu), GUI_OF->current->data, GUI_OF->current_x));
  (*y) = (openeditor->text->y + font_row_bottom_pix(menu_get_font(s->menu), (GUI_OF->current->lineno - GUI_OF->edittop->lineno)));
}

/* ----------------------------- Suggestmenu accept ----------------------------- */

static void suggestmenu_accept(void *arg, const char *const restrict lable, int _UNUSED index) {
  ASSERT(arg);
  ASSERT(lable);
  Suggest *s = arg;
  char *str = copy_of(lable);
  /* Ensure this injection of text becomes its own undo object, so we can go back to this point. */
  GUI_OF->last_action = OTHER;
  inject_into_buffer(GUI_CTX, (str + s->length), (strlen(str) - s->length));
  free(str);
  refresh_needed = TRUE;
}

/* ----------------------------- Suggestmenu copy completion ----------------------------- */

static char *suggestmenu_copy_completion(char *const restrict text) {
  Ulong len = 0;
  /* Find the end of the word. */
  while (is_word_char((text + len), FALSE) || text[len] == '_') {
    len = step_right(text, len);
  }
  return measured_copy(text, len);
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


void do_suggestion(void) {
  /* For now only c/cpp is supported. */
  if (!openfile->is_c_file && !openfile->is_cxx_file) {
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
      some_word = xmalloc(sizeof(*some_word));
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
  // if (list_of_completions) {
  //   // NETLOG("Found completions: \n");
  //   completionstruct *completion = list_of_completions;
  //   while (completion) {
  //     NETLOG("  %s\n", completion->word);
  //     completion = completion->next;
  //   }
  //   NETLOG("\n");
  // }
  // else {
  //   NETLOG("Found no completions.\n");
  // }
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
  Ulong pos;
  if (openfile->current_x > 0) {
    suggest_len = 0;
    // Ulong pos = get_prev_cursor_word_start_index();
    pos = wordstartindex(openfile->current->data, openfile->current_x, TRUE);
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

/* ----------------------------- Suggestmenu create ----------------------------- */

/* Create and initilize the `suggest-menu`. */
void suggestmenu_create(void) {
  suggest = xmalloc(sizeof(*suggest));
  suggest->menu = menu_create(gl_window_root(), textfont, suggest, suggestmenu_pos, suggestmenu_accept);
  menu_set_tab_accept_behavior(suggest->menu, TRUE);
  /* Insure <Left>/<Right> arrow input is ignored fully, as this menu will always only have a depth of one. */
  menu_set_arrow_depth_navigation(suggest->menu, FALSE);
  suggest->length = 0;
  suggest->buffer = NULL;
}

/* ----------------------------- Suggestmenu free ----------------------------- */

void suggestmenu_free(void) {
  ASSERT_SM;
  menu_free(suggest->menu);
  free(suggest->buffer);
  free(suggest);
  suggest = NULL;
}

/* ----------------------------- Suggestmenu clear ----------------------------- */

/* Fully clear the suggestions and reset the `suggest-menu` buffer. */
void suggestmenu_clear(void) {
  ASSERT_SM;
  *suggest->buffer = NUL;
  suggest->length = 0;
  menu_clear_entries(suggest->menu);
  menu_show(suggest->menu, FALSE);
}

/* ----------------------------- Suggestmenu load str ----------------------------- */

void suggestmenu_load_str(void) {
  ASSERT_SM;
  Ulong len;
  char *word;
  /* Always set the buffer to NULL and the length to zero. */
  suggest->buffer = free_and_assign(suggest->buffer, NULL);
  suggest->length = 0;
  /* If there is a word, assign it and its length to the suggest buffer. */
  if ((word = prev_word_get(GUI_OF->current->data, GUI_OF->current_x, &len, TRUE))) {
    suggest->buffer = word;
    suggest->length = len;
  }
}

/* ----------------------------- Suggestmenu find ----------------------------- */

/* Search for completions based on the current completion buffer. */
void suggestmenu_find(void) {
  ASSERT_SM;
  HashMap *map;
  openfilestruct *file;
  linestruct *line;
  int search_x = 0;
  long threshhold;
  char *completion;
  Ulong i;
  Ulong j;
  menu_clear_entries(suggest->menu);
  if (!suggest->length) {
    return;
  }
  map = hashmap_create_wfreefunc(free);
  /* For now we only search in the current editors files. */
  file = GUI_OF;
  line = GUI_OF->filetop;
  while (line) {
    threshhold = (strlen(line->data) - suggest->length - 1);
    /* Go through the whole line. */
    for (i=search_x; LT(i, threshhold); ++i) {
      /* If the first byte does not match, just continue. */
      if (line->data[i] != *suggest->buffer) {
        continue;
      }
      /* Otherwise, check the rest of the bytes. */
      for (j=1; LT(j, suggest->length) && line->data[i+j] == suggest->buffer[j]; ++j);
      /* Not all bytes matched. */
      if (LT(j, suggest->length)
      /* Word is identical to the current word. */
      || !(is_word_char((line->data + i + j), FALSE) || line->data[i+j] == '_')
      /* Match is not a seperate word. */
      || (i && is_word_char((line->data + step_left(line->data, i)), FALSE))
      /* Match is where the cursor itself is. */
      || (line == GUI_OF->current && i == (GUI_OF->current_x - suggest->length)))
      {
        continue;
      }
      completion = suggestmenu_copy_completion(line->data + i);
      /* Look for colisions. */
      if (hashmap_get(map, completion)) {
        free(completion);
        continue;
      }
      /* Otherwise, we add the entry to the map. */
      hashmap_insert(map, completion, completion);
      menu_push_back(suggest->menu, completion);
      search_x = ++i;
    }
    DLIST_ADV_NEXT(line);
    search_x = 0;
    if (!line && file->next != GUI_OF) {
      CLIST_ADV_NEXT(file);
      line = file->filetop;
    }
  }
  hashmap_free(map);
  menu_qsort(suggest->menu, menu_entry_qsort_strlen_cb);
}

/* ----------------------------- Suggestmenu run ----------------------------- */

void suggestmenu_run(void) {
  ASSERT_SM;
  suggestmenu_load_str();
  suggestmenu_find();
  if (menu_len(suggest->menu)) {
    menu_show(suggest->menu, TRUE);
  }
  else {
    menu_show(suggest->menu, FALSE);
  }
}

/* ----------------------------- Suggestmenu draw ----------------------------- */

void suggestmenu_draw(void) {
  ASSERT_SM;
  menu_draw(suggest->menu);
}

/* ----------------------------- Suggestmenu ----------------------------- */

/* Returns the internal menu of the suggest structure. */
Menu *suggestmenu(void) {
  return suggest->menu;
}
