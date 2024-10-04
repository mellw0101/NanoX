#include "../include/prototypes.h"

void highlight_current_bracket(void) {
}

bool do_find_a_bracket(linestruct *start_line, Ulong start_index, bool reverse, const char *bracket_pair,
                       linestruct **found_line, Ulong *found_index) {
  linestruct *line = start_line;
  const char *pointer, *found;
  if (reverse) {
    /* First step away from the current bracket. */
    if (!start_index) {
      line = line->prev;
      if (!line) {
        return false;
      }
      pointer = line->data + strlen(line->data);
    }
    else {
      pointer = line->data + step_left(line->data, start_index);
    }
    /* Now seek for any of the two brackets we are interested in. */
    while (!(found = mbrevstrpbrk(line->data, bracket_pair, pointer))) {
      line = line->prev;
      if (!line) {
        return false;
      }
      pointer = (line->data + strlen(line->data));
    }
  }
  else {
    pointer = (line->data + step_right(line->data, start_index));
    while (!(found = mbstrpbrk(pointer, bracket_pair))) {
      line = line->next;
      if (!line) {
        return false;
      }
      pointer = line->data;
    }
  }
  /* Set the found line and index. */
  *found_line  = line;
  *found_index = (found - line->data);
  return true;
}

bool find_matching_bracket(linestruct *start_line, Ulong start_index, linestruct **to_line, Ulong *to_index) {
  linestruct *line = start_line;
  const char *ch, *wanted_ch;
  int         ch_len, wanted_ch_len;
  char        bracket_pair_buf[MAXCHARLEN * 2 + 1];
  Ulong       halfway   = 0;
  Ulong       charcount = mbstrlen(matchbrackets) / 2;
  Ulong       balance   = 1;
  bool        reverse;
  /* Find the bracket at the starting position */
  ch = mbstrchr(matchbrackets, line->data + start_index);
  /* Not a bracket */
  if (!ch) {
    return false;
  }
  /* Find the halfway point in matchbrackets, where the closing ones start */
  for (Ulong i = 0; i < charcount; ++i) {
    halfway += char_length(matchbrackets + halfway);
  }
  /* Determine if we are searching forwards or backwards */
  reverse = (ch >= (matchbrackets + halfway));
  /* Find the complementing bracket */
  wanted_ch = ch;
  while (charcount-- > 0) {
    if (reverse) {
      wanted_ch = matchbrackets + step_left(matchbrackets, wanted_ch - matchbrackets);
    }
    else {
      wanted_ch += char_length(wanted_ch);
    }
  }
  /* Get the lengths of the current bracket and the wanted bracket */
  ch_len        = char_length(ch);
  wanted_ch_len = char_length(wanted_ch);
  /* Create a pair string with both brackets for comparison */
  strncpy(bracket_pair_buf, ch, ch_len);
  strncpy(bracket_pair_buf + ch_len, wanted_ch, wanted_ch_len);
  bracket_pair_buf[ch_len + wanted_ch_len] = '\0';
  /* Start searching for the matching bracket */
  while (do_find_a_bracket(line, start_index, reverse, bracket_pair_buf, &line, &start_index)) {
    /* Adjust balance based on whether we found the same or the complementary bracket */
    balance += (strncmp(line->data + start_index, ch, ch_len) == 0) ? 1 : -1;
    /* If balance reaches zero, the matching bracket is found */
    if (balance == 0) {
      *to_line  = line;
      *to_index = start_index;
      return true;
    }
  }
  /* No matching bracket found */
  return false;
}

bool find_end_bracket(linestruct *from, Ulong index, linestruct **end, Ulong *end_index) {
  PROFILE_FUNCTION;
  if (!from || index >= strlen(from->data)) {
    return false;
  }
  const char st_ch = from->data[index];
  if (st_ch != '{' && st_ch != '[' && st_ch != '(') {
    return false;
  }
  const char  end_ch  = (st_ch == '[') ? ']' : (st_ch == '(') ? ')' : '}';
  int         lvl     = 0;
  const char *b_start = nullptr;
  const char *b_end   = nullptr;
  FOR_EACH_LINE_NEXT(line, from) {
    /* Skip empty lines. */
    if (!line->data[0]) {
      continue;
    }
    /* If current line is start line, start search from index. */
    (from == line) ? b_start = &line->data[index] : b_start = line->data;
    /* Now find all start brackets on current line. */
    do {
      b_start = strchr(b_start, st_ch);
      if (b_start) {
        if (b_start == line->data ||
            !(line->data[(b_start - line->data) - 1] == '\'' && line->data[(b_start - line->data) + 1] == '\'')) {
          lvl += 1;
        }
        b_start += 1;
      }
    } while (b_start);
    /* If current line is start line, start search from index. */
    (from == line) ? b_end = &line->data[index] : b_end = line->data;
    /* Now find all end brackets on current line. */
    do {
      b_end = strchr(b_end, end_ch);
      if (b_end) {
        if (b_end == line->data ||
            !(line->data[(b_end - line->data) - 1] == '\'' && line->data[(b_end - line->data) + 1] == '\'')) {
          lvl -= 1;
          if (!lvl) {
            *end       = line;
            *end_index = (b_end - line->data);
            return true;
          }
        }
        b_end += 1;
      }
    } while (b_end);
  }
  return false;
}

char *fetch_bracket_body(linestruct *from, Ulong index) {
  PROFILE_FUNCTION;
  string      ret = "";
  linestruct *end_line;
  Ulong       end_index;
  if (find_end_bracket(from, index, &end_line, &end_index)) {
    /* If the bracket body is contained to one line. */
    if (from == end_line) {
      return measured_copy(&from->data[index + 1], (&from->data[end_index] - &from->data[index + 1]));
    }
    FOR_EACH_LINE_NEXT(line, from) {
      if (line == from) {
        ret += &line->data[index];
      }
      else {
        ret += line->data;
      }
      ret += '\n';
      if (line == end_line) {
        break;
      }
    }
    char *data = measured_copy(ret.c_str(), ret.length());
    return data;
  }
  return nullptr;
}
