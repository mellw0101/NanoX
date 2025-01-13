#include "../include/prototypes.h"

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

/* Assigns the number of white char`s to the prev/next word to 'nchars'.  Return`s 'true' when word is more then 2 white`s away. */
bool word_more_than_one_white_away(const char *string, Ulong index, bool forward, Ulong *nsteps) _NOTHROW {
  Ulong i = index, chars = 0;
  if (!forward) {
    --i;
    for (; i != (Ulong)-1 && is_blank_char(&string[i]); --i, ++chars);
  }
  else {
    for (; string[i] && is_blank_char(&string[i]); ++i, ++chars);
  }
  (chars > 1) ? *nsteps = chars : 0;
  return (chars > 1);
}

/* Assigns the number of white char`s to the prev/next word to 'nchars'.  Return`s 'true' when word at the current cursor position is more then 2 white`s away. */
bool cursor_word_more_than_one_white_away(bool forward, Ulong *nsteps) _NOTHROW {
  return word_more_than_one_white_away(openfile->current->data, openfile->current_x, forward, nsteps);
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

line_word_t *line_word_list(const char *str, Ulong slen) {
  PROFILE_FUNCTION;
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

/* Get the words in a line.  New and improved version of line_word_list(), that also is mush safer. */
line_word_t *get_line_words(const char *string, Ulong slen) {
  PROFILE_FUNCTION;
  line_word_t *head = NULL; /* The head of the linked list of words. */
  line_word_t *tail = NULL; /* The tail of the linked list of words. */
  Ulong index    = 0; /* The start of the current word. */
  Ulong word_end = 0; /* The end of the current word. */
  while (index < slen) {
    /* Move to the start of the next word. */
    while (is_blank_char((string + index)) || (!is_word_char((string + index), FALSE) && *(string + index) != '_')) {
      index = step_right(string, index);
    }
    /* If we are at the end of the line, or have overshot the end, exit. */
    if (index >= slen) {
      break;
    }
    word_end = get_current_word_end_index(string, index, TRUE);
    line_word_t *node = (line_word_t *)nmalloc(sizeof(*node));
    node->len   = (word_end - index);
    node->start = index;
    node->end   = word_end;
    node->str   = measured_copy((string + index), node->len);
    node->next  = NULL;
    if (!tail) {
      head = node;
      tail = node;
    }
    else {
      tail->next = node;
      tail = tail->next;
    }
    index = word_end;
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
Ulong get_prev_word_start_index(const char *line, Ulong cursor_x, bool allow_underscore) _NOTHROW {
  Ulong start_index = cursor_x;
  while (start_index > 0) {
    Ulong oneleft = step_left(line, start_index);
    if (!is_word_char(&line[oneleft], FALSE) && (!allow_underscore || line[oneleft] != '_')) {
      break;
    }
    start_index = oneleft;
  }
  return start_index;
}

/* Return the start index of the previus word from 'openfile->current_x' in 'openfile->current->data', if any.  Otherwise return 'openfile->current_x'. */
Ulong get_prev_cursor_word_start_index(bool allow_underscore) _NOTHROW {
  return get_prev_word_start_index(openfile->current->data, openfile->current_x, allow_underscore);
}

/* Return the prev word at cursor_x in line cursorline.  Otherwise return NULL when
 * there is no word * to the left.  Also asigns the length of the word to 'wordlen'. */
char *get_prev_word(const char *line, const Ulong cursor_x, Ulong *wordlen) _NOTHROW {
  Ulong start_index = get_prev_word_start_index(line, cursor_x);
  if (start_index == cursor_x) {
    return NULL;
  }
  *wordlen = (cursor_x - start_index);
  return measured_copy(&line[start_index], *wordlen);
}

/* Return the word to the left of the cursor, if any.  Otherwise return NULL.  Also assign the word length to 'wordlen'. */
char *get_prev_cursor_word(Ulong *wordlen) _NOTHROW {
  return get_prev_word(openfile->current->data, openfile->current_x, wordlen);
}

/* Return the end index of the current word, if any.  Otherwise, return from_index if already at end index or if at whitespace. */
Ulong get_current_word_end_index(const char *line, Ulong from_index, bool allow_underscore) _NOTHROW {
  Ulong index = from_index;
  while (line[index]) {
    if (!is_word_char(&line[index], FALSE) && (!allow_underscore || line[index] != '_')) {
      break;
    }
    index = step_right(line, index);
  }
  return index;
}

/* Return the end index of the current word at 'openfile->current_x' in 'openfile->current->data',
 * if any.  Otherwise, return from_index if already at end index or if at whitespace. */
Ulong get_current_cursor_word_end_index(bool allow_underscore) _NOTHROW {
  return get_current_word_end_index(openfile->current->data, openfile->current_x, allow_underscore);
}