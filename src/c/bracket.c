/** @file bracket.c

  @author  Melwin Svensson.
  @date    16-2-2025.

 */
#include "../include/c_proto.h"

#include "../include/c/wchars.h"

void findbracketmatch(SyntaxFileLine *const startline, Ulong startidx,  SyntaxFileLine **const endline, Ulong endidx) {
  ASSERT(startline);
  ASSERT(startidx);
  ASSERT(endline);
  ASSERT(endidx);
}

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
