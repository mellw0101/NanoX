/** @file csyntax.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../../include/c_proto.h"
#include "../../include/c/wchars.h"


/* --------------------- CSyntaxMacro --------------------- */

/* Create a blank allocated `CSyntaxMacro` structure. */
CSyntaxMacro *csyntaxmacro_create(void) {
  CSyntaxMacro *macro = xmalloc(sizeof(*macro));
  macro->args         = cvec_create_setfree(free);
  macro->expanded     = NULL;
  macro->expandstart  = syntaxfilepos_create(0, 0);
  macro->expandend    = syntaxfilepos_create(0, 0);
  macro->empty        = FALSE;
  return macro;
}

/* Free's a `CSyntaxMacro` structure. */
void csyntaxmacro_free(void *ptr) {
  CSyntaxMacro *macro = ptr;
  ASSERT(macro);
  cvec_free(macro->args);
  free(macro->expanded);
  syntaxfilepos_free(macro->expandstart);
  syntaxfilepos_free(macro->expandend);
  free(macro);
}

/* Parse the exantion of a macro. */
static void csyntaxmacro_expantion(SyntaxFile *const sf, CSyntaxMacro *const macro, SyntaxFileLine **const outline, const char **const outptr) {
  ASSERT(sf);
  ASSERT(macro);
  ASSERT(outline);
  ASSERT(outptr);
  /* Copy of the current line we are working with. */
  SyntaxFileLine *line = *outline;
  /* Copy of the data we are working with. */
  const char *start = *outptr;
  const char *end = *outptr;
  /* The string we will construct. */
  char *expanded = COPY_OF("");
  /* The length of blank chars. */
  Ulong white_len;
  /* Set the start position of this macro exantion inside the `SyntaxFile` structure. */
  syntaxfilepos_set(macro->expandstart, line->lineno, (start - line->data));
  /* Iter until we reach eol. */
  while (*end) {
    /* Advance until we find backslash or eol. */
    while (*end && *end != '\\') {
      end += step_right(end, 0);
    }
    /* Append the parsed line to expanded. */
    expanded = xstrncat(expanded, start, (end - start));
    /* If we found a backslash. */
    if (*end == '\\') {
      /* Advance to the next line. */
      line = line->next;
      /* Get the indent length of the new line data. */
      white_len = indentlen(line->data);
      /* And when its greater then zero, place the start ptr and one blank char before the data. */
      start = (line->data + (white_len ? (white_len - 1) : 0));
      end = start;
    }
  }
  /* Assign the expanded macro to the macro strucutre. */
  macro->expanded = expanded;
  /* Set the end position of this macro expantion inside the `SyntaxFile` structure. */
  syntaxfilepos_set(macro->expandend, line->lineno, (end - line->data));
  /* Assign the line to *outline. */
  *outline = line;
  /* Also assign the data ptr to *outptr. */
  *outptr = end;
}

/* Parse the argument's of a `c macro`. */
static void csyntaxmacro_params(SyntaxFile *const sf, CSyntaxMacro *const macro, SyntaxFileLine **const outline, const char **const outdata) {
  /* Ensure all parameters are valid. */
  ASSERT(sf);
  ASSERT(macro);
  ASSERT(outline);
  ASSERT(outdata);
  /* The current line we are working on. */
  SyntaxFileLine *line = *outline;
  /* The data ptr where we are in 'line->data'. */
  const char *data = *outdata;
  /* The index of something we ran. */
  Ulong idx;
  /* If we accept a comma if its the next thing parsed.  This starts as
   * TRUE meaning we do not accept another ',' until text is parsed. */
  bool wascomma = TRUE;
  /* Ensure this is the start of arguments. */
  ASSERT(*data == '(');
  /* Move the ptr past the '(' char. */
  data += step_right(data, 0);
  /* Parse the arguments. */
  while (TRUE) {
    /* When we are at a blank char, advance the ptr. */
    if (isblankc(data)) {
      data += indentlen(data);
    }
    /* Something other then blank or word chars. */
    else if (isconeof(*data, "\\,)\0")) {
      if (*data == ',') {
        /* If the last thing we had was a comma. */
        if (wascomma) {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected argument name");
        }
        else {
          wascomma = TRUE;
        }
        data += step_right(data, 0);
      }
      else if (*data == ')') {
        /* When the last thing was a comma, report an error. */
        if (wascomma && cvec_len(macro->args) != 0) {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected argument name");
        }
        break;
      }
      else if (*data == '\\') {
        idx = step_right(data, 0);
        /* When eol is not directly after the backslash.  Add a error. */
        if (*(data + idx)) {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected newline directly after backslash");
        }
        /* Continue like normal even when there is wierd backslash usage. */
        if (line->next) {
          line = line->next;
          data = line->data;
        }
        /* There is no line after this one, so just leave. */
        else {
          break;
        }
      }
      else if (!*data) {
        syntaxfile_adderror(sf, line->lineno, (data - line->data), "Unexpected eol");
        break;
      }
    }
    /* Comment. */
    else if (data[0] == '/' && data[1] == '*') {
      findblockcommentmatch(line, (data - line->data), &line, &idx);
      data = (line->data + idx);
    }
    /* Otherwise when we are at a word forming char. */
    else if (iswordc(data, FALSE, "_.")) {
      if (*data == '.') {
        /* This is a valid variatic parameter argument. */
        if (strncmp(data, S__LEN("...")) == 0 && (isblankornulc(data + STRLEN("...")) || isconeof(*(data + STRLEN("...")), "\\),"))) {
          cvec_push(macro->args, COPY_OF("..."));
          idx = 3;
        }
        /* Add error. */
        else {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected a comma");
          idx = 0;
          while (*data == '.') {
            data += step_right(data, 0);
            ++idx;
          }
        }
      }
      else {
        ASSERT((idx = wordendindex(data, 0, TRUE)) != 0);
        cvec_push(macro->args, measured_copy(data, idx));
      }
      data += idx;
      /* If this text was after anoter argument without a comma between them. */
      if (!wascomma) {
        syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected a comma");
      }
      wascomma = FALSE;
    }
    /* As a last safety macanicam, if none of these are true, we just report a error and return. */
    else {
      syntaxfile_adderror(sf, line->lineno, (data - line->data), "Failed to parse macro arguments, unexpected error.");
      break;
    }
  }
  /* Assign the current line and data ptr to the parameters passed by caller. */
  *outline = line;
  *outdata = data;
}

/* Parse a `c define` macro. */
static void csyntaxmacro_parse(SyntaxFile *const sf, SyntaxFileLine **const outline, const char **const outdata) {
  ASSERT(sf);
  ASSERT(outline);
  ASSERT(outdata);
  /* Current line we are working with. */
  SyntaxFileLine *line = *outline;
  /* Ptr to the data of the current line. */
  const char *data = *outdata;
  /* Holds allocated data when needed. */
  char *ptr;
  Ulong endidx;
  SyntaxObject *obj;
  CSyntaxMacro *macro;
  /* 'EOL' here means this macro has no name, so add a error. */
  if (!*data) {
    syntaxfile_adderror(sf, line->lineno, (data - line->data), "Macro must have a name");
  }
  else {
    endidx = wordendindex(data, 0, TRUE);
    /* The macro's name is invalid. */
    if (!endidx) {
      syntaxfile_adderror(sf, line->lineno, (data - line->data), "Macro name is invalid");
    }
    else {
      /* Create the syntaxfile object. */
      obj = syntaxobject_create();
      syntaxobject_setfile(obj, sf->path);
      syntaxobject_setcolor(obj, SYNTAX_COLOR_BLUE);
      syntaxobject_settype(obj, SYNTAX_OBJECT_TYPE_C_MACRO);
      syntaxobject_setpos(obj, line->lineno, (data - line->data));
      /* Allocate the name, so we can add it to the object map. */
      ptr = measured_copy(data, endidx);
      syntaxfile_addobject(sf, ptr, obj);
      free(ptr);
      /* Adv data ptr. */
      data += endidx;
      macro = csyntaxmacro_create();
      /* If there is a '(' char directly after the macro name, this macro could have arguments. */
      if (*data == '(') {
        /* Parse the arguments. */
        csyntaxmacro_params(sf, macro, &line, &data);
        /* If the data ptr does not point to a ')' char, something went wrong when parsing the arguments. */
        if (*data != ')') {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Failed to parse macro arguments");
        }
        else {
          data += STRLEN(")");
        }
      }
      data += indentlen(data);
      csyntaxmacro_expantion(sf, macro, &line, &data);
      syntaxobject_setdata(obj, macro, csyntaxmacro_free);
    }
  }
  *outline = line;
  *outdata = data;
}

/* ----------------------------- CSyntaxPp ----------------------------- */

/* Parse a c preprocessor line. */
static void csyntaxpp_parse(SyntaxFile *const sf, SyntaxFileLine **const outline, const char **const outdata) {
  ASSERT(sf);
  ASSERT(outline);
  ASSERT(outdata);
  SyntaxFileLine *line = *outline;
  const char *data = *outdata;
  /* Define directive. */
  if (strncmp(data, S__LEN("define")) == 0 && isblankornulc(data + STRLEN("define"))) {
    /* Advance the data ptr to the first non blank char after `define`. */
    data += (indentlen(data + STRLEN("define")) + STRLEN("define"));
    /* Parse the macro. */
    csyntaxmacro_parse(sf, &line, &data);
  }
  *outline = line;
  *outdata = data;
}

/* ----------------------------- CSyntaxStruct ----------------------------- */

/* Create a new blank allocated `CSyntaxStruct` structure. */
static CSyntaxStruct *csyntaxstruct_create(void) {
  CSyntaxStruct *st = xmalloc(sizeof(*st));
  st->bodystpos     = syntaxfilepos_create(0, 0);
  st->bodyendpos    = syntaxfilepos_create(0, 0);
  st->forward_decl  = FALSE;
  return st;
}

/* Free a allocated `CSyntaxStruct` structure. */
static void csyntaxstruct_free(void *ptr) {
  CSyntaxStruct *st = ptr;
  ASSERT(st);
  syntaxfilepos_free(st->bodystpos);
  syntaxfilepos_free(st->bodyendpos);
  free(st);
}

/* Add a c struct forward declaration object to the syntaxfile hashmap. */
static void csyntaxstruct_add_forward_decl(SyntaxFile *const sf, int lineno, int colno,  const char *const restrict name) {
  ASSERT(sf);
  ASSERT(name);
  ASSERT(lineno > 0);
  CSyntaxStruct *st = csyntaxstruct_create();
  SyntaxObject *obj = syntaxobject_create();
  syntaxobject_setfile(obj, sf->path);
  syntaxobject_setcolor(obj, SYNTAX_COLOR_GREEN);
  syntaxobject_settype(obj, SYNTAX_OBJECT_TYPE_C_STRUCT);
  syntaxobject_setpos(obj, lineno, colno);
  st->forward_decl = TRUE;
  syntaxobject_setdata(obj, st, csyntaxstruct_free);
  syntaxfile_addobject(sf, name, obj);
}

/* Add a c struct declaration objecty to the syntaxfile hashmap.  TODO: Parse the data of the struct not just move the line and data ptr's to the end. */
static void csyntaxstruct_add_decl(SyntaxFile *const sf, SyntaxFileLine **const outline, const char **const outdata, int row, int col, const char *const restrict name) {
  ASSERT(sf);
  ASSERT(outline);
  ASSERT(outdata);
  ASSERT(name);
  CSyntaxStruct *st = csyntaxstruct_create();
  SyntaxObject *obj = syntaxobject_create();
  SyntaxFileLine *line = *outline;
  const char *data = *outdata;
  ALWAYS_ASSERT(*data == '{');
  syntaxobject_setfile(obj, sf->path);
  syntaxobject_setcolor(obj, SYNTAX_COLOR_GREEN);
  syntaxobject_settype(obj, SYNTAX_OBJECT_TYPE_C_STRUCT);
  syntaxobject_setpos(obj, row, col);
  st->forward_decl = FALSE;
  syntaxfilepos_set(st->bodystpos, line->lineno, (data - line->data));
  findbracketmatch(&line, &data);
  syntaxfilepos_set(st->bodyendpos, line->lineno, (data - line->data));
  syntaxobject_setdata(obj, st, csyntaxstruct_free);
  syntaxfile_addobject(sf, name, obj);
  /* Set the current line and data ptr before we return. */
  *outline = line;
  *outdata = data;
}

/* Parse a c struct. */
static void csyntaxstruct_parse(SyntaxFile *const sf, SyntaxFileLine **const outline, const char **const outdata, bool *recheck) {
  ASSERT(sf);
  ASSERT(outline);
  ASSERT(outdata);
  int row, col, endidx;
  SyntaxFileLine *line = *outline;
  const char *data = *outdata;
  char *ptr;
  /* Move the ptr to the first non blank char after struct. */
  data += (indentlen(data + STRLEN("struct")) + STRLEN("struct"));
  findnextchar(&line, &data);
  /* `EOL` here means either this is a multiline decl, or its invalid. */
  if (!*data) {
    syntaxfile_adderror(sf, line->lineno, (data - line->data), "`struct` type must have a name");
  }
  /* Anonomus struct.  Note that in some c standard's this must be named, always... */
  else if (*data == '{') {
  }
  else {
    endidx = wordendindex(data, 0, TRUE);
    /* Struct has an invalid name. */
    if (!endidx) {
      syntaxfile_adderror(sf, line->lineno, (data - line->data), "Struct has an invalid name");
    }
    else {
      row = line->lineno;
      col = (data - line->data);
      /* Allocate and ptint the structure name, for now. */
      ptr = measured_copy(data, endidx);
      data += endidx;
      findnextchar(&line, &data);
      /* The strucure has a body. */
      if (*data == '{') {
        writef("%s:[%d:%d]: Found struct decl: %s {}\n", sf->path, row, col, ptr);
        csyntaxstruct_add_decl(sf, &line, &data, row, col, ptr);
        /* If we are not at the end bracket of the struct body, add a error. */
        if (*data != '}') {
          syntaxfile_adderror(sf, row, col, "Struct does not have closing bracket to its body");
        }
        /* Otherwise, check if the struct has the correct ending. */
        else {
          data += 1;
          findnextchar(&line, &data);
          /* This struct ends correctly with a ';' char after the closing bracket. */
          if (*data == ';') {
            data += 1;
            findnextchar(&line, &data);
            ASSIGN_IF_VALID(recheck, TRUE);
          }
          /* Otherwise, check if this struct has a varable declared at its end. */
          else {
            endidx = wordendindex(data, 0, TRUE);
            /* If the name is invalid. */
            if (!endidx) {
              syntaxfile_adderror(sf, line->lineno, (data - line->data), "Variable declaration has an invalid name");
            }
            else {
              /* TODO: Parse the variable declared here. */
              data += endidx;
              findnextchar(&line, &data);
              if (*data != ';') {
                syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected ';' after struct");
              }
              else {
                data += 1;
                findnextchar(&line, &data);
                ASSIGN_IF_VALID(recheck, TRUE);
              }
            }
          }
        }
      }
      /* This structure declaration is a forward declaration. */
      else if (*data == ';') {
        writef("%s:[%d:%d]: Found forward struct decl: %s;\n", sf->path, row, col, ptr);
        csyntaxstruct_add_forward_decl(sf, row, col, ptr);
        data += 1;
        findnextchar(&line, &data);
        ASSIGN_IF_VALID(recheck, TRUE);
      }
      /* TODO: If none of these are true, this could be a variable declaration. */
      free(ptr);
    }
  }
  *outline = line;
  *outdata = data;
}

/* ----------------------------- Main parsing function ----------------------------- */

/* The main parsing function for c files. */
void syntaxfile_parse_csyntax(SyntaxFile *const sf) {
  ASSERT(sf);
  /* Pointer to the data of the current line. */
  const char *data;
  bool recheck = FALSE;
  /* Iter all lines in the syntax file. */
  ITER_SFL_TOP(sf, line,
    /* If the line is empty, just continue. */
    if (!*line->data) {
      continue;
    }
    /* Start by assigning the data ptr to the first non blank char in the line. */
    data = &line->data[indentlen(line->data)];
recheck:
    /* If this line contains only blank chars, continue. */
    if (!*data) {
      continue;
    }
    else if (*data == '/' && *(data + 1) == '*') {
      skip_blkcomment(&line, &data);
      findnextchar(&line, &data);
      goto recheck;
    }
    /* If this line is a preprocessor line. */
    else if (*data == '#') {
      data += (indentlen(data + STRLEN("#")) + STRLEN("#"));
      /* 'EOL' here means an error. */
      if (!*data) {
        syntaxfile_adderror(sf, line->lineno, (data - line->data), "'#' Needs a preprocessor directive");
      }
      else {
        csyntaxpp_parse(sf, &line, &data);
      }
    }
    /* If the line is a pure struct declaration.  So no typedef or such. */
    else if (strncmp(data, S__LEN("struct")) == 0 && isblankornulc(data + STRLEN("struct"))) {
      csyntaxstruct_parse(sf, &line, &data, &recheck);
      if (recheck) {
        goto recheck;
      }
    }
    else if (strncmp(data, S__LEN("typedef")) == 0 && isblankornulc(data + STRLEN("typedef"))) {
      data += STRLEN("typedef");
      findnextchar(&line, &data);
      /* Typedef struct. */
      if (strncmp(data, S__LEN("struct")) == 0 && isblankornulc(data + STRLEN("struct"))) {
        writef("%s:[%lu:%lu]: Found typedef struct: %s\n", sf->path, line->lineno, (data - line->data), data);
      }
      else if (strncmp(data, S__LEN("enum")) == 0 && isblankornulc(data + STRLEN("enum"))) {
        writef("%s:[%lu:%lu]: Found typedef enum: %s\n", sf->path, line->lineno, (data - line->data), data);
      }
    }
  );
}
