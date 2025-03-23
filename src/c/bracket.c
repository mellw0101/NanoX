/** @file bracket.c

  @author  Melwin Svensson.
  @date    16-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"


void findblockcommentmatch(SyntaxFileLine *const startline, Ulong startidx, SyntaxFileLine **const endline, Ulong *endidx) {
  ASSERT(startline);
  ASSERT(startidx);
  ASSERT(endline);
  ASSERT(endidx);
  SyntaxFileLine *line = startline;
  Ulong idx = startidx;
  const char *data = line->data;
  ALWAYS_ASSERT(data[idx] == '/' && data[idx + 1] == '*');
  idx += 2;
  while (TRUE) {
    if (data[idx] == '*' && data[idx + 1] == '/') {
      idx += 2;
      break;
    }
    else if (!data[idx]) {
      if (!line->next) {
        break;
      }
      line = line->next;
      data = line->data;
    }
    else {
      idx = step_right(data, idx);
    }
  }
  *endline = line;
  *endidx = idx;
}

void skip_blkcomment(SyntaxFileLine **const outline, const char **const outdata) {
  ASSERT(outline);
  ASSERT(outdata);
  /* The current line we are at. */
  SyntaxFileLine *line = *outline;
  /* A ptr to the data of the current line. */
  const char *data = *outdata;
  ALWAYS_ASSERT_MSG((*data == '/' && *(data + 1) == '*'), "This function should only be called when at the start of a block comment");
  data += 2;
  while (TRUE) {
    /* The end of the block comment. */
    if (*data == '*' && *(data + 1) == '/') {
      data += step_nright(data, 0, 2);
      break;
    }
    /* `EOL` has been reached. */
    else if (!*data) {
      /* If we are currently at the last line, just break the loop. */
      if (!line->next) {
        break;
      }
      line = line->next;
      data = line->data;
    }
    /* Otherwise, just move to the next char. */
    else {
      data += step_right(data, 0);
    }
  }
  /* Assign the current line and its data ptr back to the callers inputs. */
  *outline = line;
  *outdata = data;
}

/* Only works for '{' starting bracket gooing forward.  For now. */
void findbracketmatch(SyntaxFileLine **const outline, const char **const outdata) {
  ASSERT(outline);
  ASSERT(outdata);
  int lvl = 0;
  SyntaxFileLine *line = *outline;
  const char *data = *outdata;
  /* Assert that we start at a bracket. */
  ALWAYS_ASSERT(*data == '{');
  data += step_right(data, 0);
  while (TRUE) {
    if (*data == '/' && *(data + 1) == '*') {
      skip_blkcomment(&line, &data);
    }
    else if (!*data || (*data == '/' && *(data + 1) == '/')) {
      if (!line->next) {
        break;
      }
      line = line->next;
      data = line->data;
    }
    else {
      if (*data == '{') {
        ++lvl;
      }
      else if (*data == '}') {
        if (!lvl) { 
          break;
        }
        --lvl;
      }
      data += step_right(data, 0);
    }
  }
  *outline = line;
  *outdata = data;
}

/* Move to the next non blank char, even if its ten lines down. */
void findnextchar(SyntaxFileLine **const outline, const char **const outdata) {
  ASSERT(outline);
  ASSERT(outdata);
  SyntaxFileLine *line = *outline;
  const char *data = *outdata;
  while (TRUE) {
    if (isblankc(data)) {
      data += step_right(data, 0);
    }
    else if (*data == '/' && *(data + 1) == '*') {
      skip_blkcomment(&line, &data);
    }
    else if (!*data || (*data == '/' && *(data + 1) == '/')) {
      if (!line->next) {
        break;
      }
      line = line->next;
      data = line->data;
    }
    else {
      break;
    }
  }
  *outline = line;
  *outdata = data;
}
