#include "../include/prototypes.h"

char **delim_str(const char *str, const char *delim, Ulong *size) {
  PROFILE_FUNCTION;
  if (!str) {
    return NULL;
  }
  Uint   bsize = 0, cap = 10;
  char **words = (char **)nmalloc(sizeof(char *) * cap);
  char  *tok   = strtok((char *)str, delim);
  while (tok != NULL) {
    if (bsize == cap) {
      cap *= 2;
      words = (char **)nrealloc(words, sizeof(char *) * cap);
    }
    words[bsize++] = copy_of(tok);
    tok            = strtok(NULL, delim);
  }
  words[bsize] = NULL;
  (size != NULL) ? *size = bsize : 0;
  return words;
}

char **split_into_words(const char *str, const Uint len, Uint *word_count) {
  const Uint max_words = ((len / 2) + 1);
  char **words = (char **)nmalloc(sizeof(char *) * max_words);
  Uint count = 0;
  const char *start = str, *end = str;
  while (end < (str + len)) {
    for (; end < (str + len) && *end == ' '; end++)
      ;
    if (end == (str + len)) {
      break;
    }
    start = end;
    for (; end < (str + len) && *end != ' '; end++)
      ;
    words[count++] = measured_copy(start, (end - start));
  }
  *word_count = count;
  return words;
}

/* Remove`s all leading 'c' char`s from 'word'.  Note that 'word' shall be passed by refrence. */
void remove_leading_char_type(char **word, const char c) {
  while (*(*word) == c) {
    *word += 1;
  }  
}

/* Return`s "\<('word')\>". */
char *rgx_word(const char *word) {
  Ulong len = (strlen(word) + STRLTRLEN("\\<()\\>"));
  char *ret = (char *)nmalloc(len + 1);
  sprintf(ret, "%s%s%s", "\\<(", word, ")\\>");
  return ret;
}

/* Assigns the number of white char`s to the prev/next word to 'nchars'.  Return`s 'true' when word is more then 2 white`s away. */
bool word_more_than_one_white_away(bool forward, Ulong *nsteps) _NO_EXCEPT {
  Ulong i = openfile->current_x, chars = 0;
  if (!forward) {
    i--;
    for (; i != (Ulong)-1 && is_blank_char(&openfile->current->data[i]); i--, chars++);
  }
  else {
    for (; openfile->current->data[i] && is_blank_char(&openfile->current->data[i]); i++, chars++);
  }
  (chars > 1) ? *nsteps = chars : 0;
  return (chars > 1);
}

bool prev_word_is_comment_start(Ulong *nsteps) {
  Ulong i = openfile->current_x - 1, steps = 0;
  for (; i != (Ulong)-1 && openfile->current->data[i] == ' '; i--, steps++);
  if (openfile->current->data[i - 1] && (openfile->current->data[i - 1] == '/' && openfile->current->data[i] == '/')) {
    *nsteps = steps + 2;
    return true;
  }
  return FALSE;
}

/* Return`s 'true' when 'ch' is found in 'word', and 'FALSE' otherwise. */
bool char_is_in_word(const char *word, const char ch, Ulong *at) {
  *at = (Ulong)-1;
  for (Ulong i = 0; word[i] != '\0' && *at == (Ulong)-1; (word[i] == ch) ? *at = i : 0, i++);
  return (*at != (Ulong)-1);
}

char *retrieve_word_from_cursor_pos(bool forward) {
  const Ulong slen = strlen(openfile->current->data);
  Ulong       i;
  for (i = openfile->current_x; i < slen; i++) {
    if (!is_word_char(openfile->current->data + i, FALSE)) {
      if (openfile->current->data[i] != '_') {
        break;
      }
    }
  }
  if (i == openfile->current_x) {
    return NULL;
  }
  return measured_copy(&openfile->current->data[openfile->current_x], i - openfile->current_x);
}

char **fast_words_from_str(const char *str, Ulong slen, Ulong *nwords) {
  PROFILE_FUNCTION;
  Ulong  size = 0, cap = 10;
  char **words = (char **)nmalloc(sizeof(char *) * cap);
  (str[slen] == '\n') ? slen-- : 0;
  const char *start = str, *end = str;
  while (end < (str + slen)) {
    for (; end < (str + slen) && (*end == ' ' || *end == '\t'); end++);
    if (end == (str + slen)) {
      break;
    }
    start = end;
    for (; end < (str + slen) && *end != ' '; end++);
    const Uint word_len = end - start;
    (size == cap) ? cap *= 2, words = (char **)nrealloc(words, sizeof(char *) * cap) : 0;
    words[size++] = measured_copy(start, word_len);
  }
  words[size] = NULL;
  *nwords     = size;
  return words;
}

line_word_t *line_word_list(const char *str, Ulong slen) {
  line_word_t *head = NULL, *tail = NULL;
  (str[slen] == '\n') ? slen-- : 0;
  const char *start = str, *end = str;
  while (end < (str + slen)) {
    for (; end < (str + slen) && (*end == ' ' || *end == '\t'); end++)
      ;
    if (end == (str + slen)) {
      break;
    }
    while (!is_word_char(end, FALSE) && *end != '_') {
      end += 1;
    }
    start = end;
    for (; (end < (str + slen)) && (*end != ' ') && (*end != '\t') && (is_word_char(end, FALSE) || *end == '_'); end++)
      ;
    const Uint word_len = (end - start);
    line_word_t *word = (line_word_t *)malloc(sizeof(*word));
    word->str   = measured_copy(start, word_len);
    word->start = (start - str);
    word->len   = word_len;
    word->end   = (word->start + word_len);
    word->next  = NULL;
    if (!tail) {
      head = word;
      tail = word;
    }
    else {
      tail->next = word;
      tail = tail->next;
    }
  }
  return head;
}

Uint last_strchr(const char *str, const char ch, Uint maxlen) {
  Uint i = 0, last_seen = 0;
  for (; str[i] && (i < maxlen); i++) {
    if (str[i] == ch) {
      last_seen = i;
    }
  }
  return last_seen;
}

char *memmove_concat(const char *s1, const char *s2) {
  Ulong len1 = strlen(s1);
  Ulong len2 = strlen(s2);
  char *data = (char *)nmalloc(len1 + len2 + 1);
  memmove(data, s1, len1);
  memmove(data + len1, s2, len2);
  data[len1 + len2] = '\0';
  return data;
}

const char *substr(const char *str, Ulong end_index) {
  static char buf[PATH_MAX];
  for (Uint i = 0; i < end_index; buf[i] = str[i], i++);
  return buf;
}

/* Return the start index of the previus word, if any.  Otherwise return cursor_x. */
Ulong get_prev_word_start_index(const char *line, const Ulong cursor_x) _NO_EXCEPT {
  Ulong start_index = cursor_x;
  while (start_index > 0) {
    Ulong oneleft = step_left(line, start_index);
    if (!is_word_char(&line[oneleft], FALSE)) {
      break;
    }
    start_index = oneleft;
  }
  return start_index;
}

/* Return the start index of the previus word from 'openfile->current_x' in 'openfile->current->data', if any.  Otherwise return 'openfile->current_x'. */
Ulong get_cursor_prev_word_start_index(void) _NO_EXCEPT {
  return get_prev_word_start_index(openfile->current->data, openfile->current_x);
}

/* Return the prev word at cursor_x in line cursorline.  Otherwise return NULL when
 * there is no word * to the left.  Also asigns the length of the word to 'wordlen'. */
char *get_prev_word(const char *line, const Ulong cursor_x, Ulong *wordlen) _NO_EXCEPT {
  Ulong start_index = get_prev_word_start_index(line, cursor_x);
  if (start_index == cursor_x) {
    return NULL;
  }
  *wordlen = (cursor_x - start_index);
  return measured_copy(&line[start_index], *wordlen);
}

/* Return the word to the left of the cursor, if any.  Otherwise return NULL.  Also assign the word length to 'wordlen'. */
char *get_prev_cursor_word(Ulong *wordlen) _NO_EXCEPT {
  return get_prev_word(openfile->current->data, openfile->current_x, wordlen);
}