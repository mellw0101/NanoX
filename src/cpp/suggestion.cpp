#include "../include/prototypes.h"

char  suggest_buf[1024] = "";
char *suggest_str = NULL;
int   suggest_len = 0;

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
  if (openfile->type.is_set<C_CPP>()) {
    if (strncmp("char", suggest_buf, suggest_len) == 0) {
      suggest_str = (char *)"char";
    }
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
    const char *c = &openfile->current->data[openfile->current_x - 1];
    if (is_word_char(c - 1, FALSE)) {
      suggest_len     = 0;
      const Ulong pos = word_index(TRUE);
      for (int i = pos; i < (openfile->current_x - 1); suggest_len++, i++) {
        suggest_buf[suggest_len] = openfile->current->data[i];
      }
      suggest_buf[suggest_len] = '\0';
    }
    suggest_buf[suggest_len++] = *c;
    suggest_buf[suggest_len]   = '\0';
  }
}

/* Draw current suggestion if found to the suggest window. */
void draw_suggest_win(void) {
  if (!suggest_str) {
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
  }
}

/* Inject a suggestion. */
void accept_suggestion(void) {
  if (suggest_str) {
    inject((suggest_str + suggest_len), (strlen(suggest_str) - suggest_len));
  }
  clear_suggestion();
}
