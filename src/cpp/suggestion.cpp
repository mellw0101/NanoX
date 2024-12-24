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
    for (i = search_x; (long)i < threshhold; ++i) {
      /* If the first byte does not match, move on. */
      if (search_line->data[i] != suggest_buf[0]) {
        continue;
      }
      /* Check the rest of the bytes. */
      for (j = 1; j < suggest_len; ++j) {
        if (search_line->data[i + j] != suggest_buf[j]) {
          break;
        }
      }
      /* Continue searching if all bytes did not match. */
      if (j < suggest_len) {
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

  /* if (openfile->type.is_set<C_CPP>()) {
    NETLOG("suggest_buf: %s\n", suggest_buf);
    if (strncmp("char", suggest_buf, suggest_len) == 0) {
      suggest_str = (char *)"char";
    }
  } */
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
    Ulong pos = get_cursor_prev_word_start_index();
    while (pos < openfile->current_x) {
      suggest_buf[suggest_len++] = openfile->current->data[pos++];
    }
    suggest_buf[suggest_len] = '\0';
  }
}

/* Draw current suggestion if found to the suggest window. */
void draw_suggest_win(void) {
  /* if (!suggest_str) {
    return;
  }
  Ulong col_len = (strlen(suggest_str) + 2);
  Ulong row_len = 1;
  Ulong row_pos = ((openfile->cursor_row > editwinrows - 2) ? (openfile->cursor_row - row_len) : (openfile->cursor_row + 1));
  Ulong col_pos = (xplustabs() + margin - suggest_len - 1);
  suggestwin = newwin(row_len, col_len, row_pos, col_pos);
  mvwprintw(suggestwin, 0, 1, "%s", suggest_str);
  wrefresh(suggestwin);
  if (ISSET(SUGGEST_INLINE)) {
    RENDR(SUGGEST, suggest_str);
  } */
  bool list_up = FALSE;
  if (list_of_completions) {
    completionstruct *completion;
    Ulong width  = 0;
    Ulong height = 0;
    completion = list_of_completions;
    while (completion) {
      Ulong wordlen = (breadth(completion->word) + 2);
      if (wordlen > width) {
        width = wordlen;
      }
      if (++height > (editwinrows - (openfile->cursor_row + 2)) /* && height > (openfile->cursor_row - 1) */) {
        list_up = FALSE;
        break;
      }
      completion = completion->next;
    }
    Ulong posx = (xplustabs() + margin - suggest_len - 1);
    Ulong posy = (!list_up ? (openfile->cursor_row + 1) : 0) /* ((openfile->cursor_row > (editwinrows - (height + 1))) ? (openfile->cursor_row - height) : (openfile->cursor_row + 1)) */;
    suggestwin = newwin(height, width, posy, posx);
    completion = list_of_completions;
    for (Ulong i = 0; completion && i < height; ++i) {
      mvwprintw(suggestwin, i, 1, "%s", completion->word);
      completion = completion->next;
    }
    wrefresh(suggestwin);
  }
}

/* Inject a suggestion. */
void accept_suggestion(void) {
  if (suggest_str) {
    inject((suggest_str + suggest_len), (strlen(suggest_str) - suggest_len));
  }
  clear_suggestion();
}