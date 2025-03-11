/** @file csyntax.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../../include/c_proto.h"
#include "../../include/c/wchars.h"

/* Parse the arguments of a c macro definition.  Return's `-1` on incompleat macro argument parse, otherwise, `0`. */
_UNUSED static int macroargv(SyntaxFile *const sf, CSyntaxMacro *const macro, SyntaxFileLine **const outline, const char **const outptr) {
  ASSERT(sf);
  ASSERT(macro);
  ASSERT(outline);
  ASSERT(outptr);
  /* The return status of this function. */
  int ret=0;
  /* The line we are working with. */
  SyntaxFileLine *line = *outline;
  /* The data. */
  const char *data = *outptr;
  /* A 'char *' to hold parsed args. */
  char *ptr;
  /* The length of blank chars to the next char. */
  Ulong whitelen;
  /* End index of a word. */
  Ulong endidx;
  /* The cap and size of the char array. */
  Ulong size=0, cap=10;
  /* Always assert that the string does start's with the char '('. */
  ALWAYS_ASSERT(*data == '(');
  /* Move right once past the '(' char, and skip all blank chars. */
  data += step_right(data, 0);
  whitelen = indentlen(data);
  /* If there are no params just a empty '()' just return. */  
  if (data[whitelen] == ')') {
    return -1;
  }
  /* Move the data ptr to the first char, and reset whitelen. */
  data += whitelen;
  whitelen = 0;
  /* Allocate the argv ptr. */
  macro->argv = xmalloc(sizeof(void *) * 10);
  macro->argv[0] = NULL;
  /* Iterate until we reatch the end char. */
  while (*data && *data != ')') {
    /* Check if this is a arg list ('...'). */
    if (data[whitelen] == '.') {
      /* Check that there is three '.' chars in a row. */
      if (data[whitelen + 1] == '.' && data[whitelen + 2] == '.') {
        ptr = measured_copy(S__LEN("..."));
        endidx = 3;
      }
      /* Otherwise, add the error to the syntaxfile. */
      else {
        syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Invalid variadic argument parameter");
        /* Advance the ptr to the first '.' char. */
        data += whitelen;
        /* Then to the end of all '.' char's. */
        while (data[0] == '.') {
          data += step_right(data, 0);
        }
        whitelen = indentlen(data);
        /* Only advance the data ptr when the first non blank char
         * after the invalid variadic parameter is a arg separator */
        if (data[whitelen] == ',') {
          data += (whitelen + 1);
          /* Get the length to the next non blank char. */
          whitelen = indentlen(data);
          /* Advance the data ptr to that char and set whitelen to 0. */
          data += whitelen;
          whitelen = 0;
        }
        /* When the veriatic argument was the last argument for the macro. */
        else if (data[whitelen] == ')') {
          data += whitelen;
        }
        /* Otherwise, when there is more text without a separator. */
        else {
          syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "A macro argument must have a separator");
          data += whitelen;
          whitelen = 0;
        }
        continue;
      }
    }
    /* This is a invalid state as there cannot be a empty argument. */
    else if (data[whitelen] == ',') {
      syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Expected macro argument parameter");
      /* Advance the ptr. */
      data += (whitelen + 1);
      /* And get the length of blank chars. */
      whitelen = indentlen(data);
      continue;
    }
    /* Otherwise, treat the arg as if there can only be word chars in it. */
    else {
      /* Get the index of the end of the current param. */
      ALWAYS_ASSERT_MSG(((endidx = wordendindex(data, whitelen, TRUE)) != whitelen), data);
      ptr = measured_copy(data, endidx);
    }
    ENSURE_PTR_ARRAY_SIZE(macro->argv, cap, size);
    /* Insert the param into the array. */
    macro->argv[size++] = ptr;
    ptr = NULL;
    /* Move the data ptr to the end of the param we just parsed. */
    data += endidx;
    whitelen = indentlen(data);
    /* If there is another param to parse. */
    if (data[whitelen] == ',') {
      /* Move data ptr to one after the ',' char. */
      data += (whitelen + 1);
      /* Get the length to the next non blank char. */
      whitelen = indentlen(data);
      /* If there are muliple ',' chars in a row, clear all of them. */
      while (data[whitelen] == ',') {
        syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Expected macro argument parameter");
        data += (whitelen + 1);
        whitelen = indentlen(data);
      }
      /* If the next thing after the ',' char is the end of the arguments, also add a error. */
      if (data[whitelen] == ')') {
        syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Expected macro argument parameter");
        data += whitelen;
        break;
      }
    }
    /* Otherwise, if this is the last argument. */
    else if (data[whitelen] == ')') {
      data += whitelen;
      break;
    }
    /* Macro continues on the next line. */
    else if (data[whitelen] == '\\') {
      /* If the line has more data on it after line separator, add a error and stop parsing. */
      if (data[whitelen + 1]) {
        syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Backslash and newline separated");
        /* Indicate there was as error parsing the macro args, and they are not in a good state. */
        ret = -1;
        break;
      }
      /* Oterwise, move to the next line. */
      line = line->next;
      data = line->data;
      whitelen = indentlen(data);
    }
    /* Unexpected eol. */
    else if (!data[whitelen]) {
      syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Unexpected eol");
      ret = -1;
      break;
    }
    /* Or when we have a error.  Note that we still parse the all remaining arguments, this way we can keep parsing correctly. */
    else {
      syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Invalid argument for a macro");
    }
    data += whitelen;
    whitelen = 0;
  }
  /* Resize the array to (size + 1) to save memory, as well as 'NULL-TERMINATING' the array. */
  TRIM_PTR_ARRAY(macro->argv, cap, size);
  macro->argv[size] = NULL;
#if (MACROARGV_DEBUG_PRINT == 1)
  for (char **arg = macro->argv; *arg; ++arg) {
    printf("Arg[%lu]: %s\n", (arg - macro->argv), *arg);
  }
#endif
  /* Assign the line we have worked with to the output line. */
  *outline = line;
  /* Also assign the data ptr to the output ptr. */
  *outptr = data;
  /* Return the status. */
  return ret;
}

/* Parse the exantion of a macro. */
static void macroexpantion(SyntaxFile *const sf, CSyntaxMacro *const macro, SyntaxFileLine **const outline, const char **const outptr) {
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
  Ulong whitelen;
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
      whitelen = indentlen(line->data);
      /* And when its greater then zero, place the start ptr and one blank char before the data. */
      start = (line->data + (whitelen ? (whitelen - 1) : 0));
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

static void csyntax_macroargv(SyntaxFile *const sf, CSyntaxMacro *const macro, SyntaxFileLine **outline, const char **const outdata) {
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
        if (wascomma) {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected argument name");
        }
        break;
      }
      else if (*data == '\\') {
        idx = step_right(data, 0);
        /* When eol is not directly after the backslash.  Add a error. */
        if (*(data + idx) != '\0') {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Expected newline directly after backslash");
        }
        /* Continue like normal even when there is wierd backslash usage. */
        if (line->next) {
          line = line->next;
          data = line->data;
        }
        /* There is no line after this one, so just leave. */
        else {
          return;
        }
      }
      else if (*data == '\0') {
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
      // printf("Arg: %s\n", (char *)cvec_back(macro->args));
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

/* Process a `c` based `SyntaxFile`. */
void process_syntaxfile_c(SyntaxFile *const sf) {
  /* Assert the 'SyntaxFile'. */
  ASSERT(sf);
  /* Copy of the data for each line. */
  const char *data;
  /* Ptr to hold a string when needed. */
  char *ptr;
  /* The indentlen of the line. */
  Ulong whitelen;
  /* Used to hold the end index of a word or other things when needed. */
  Ulong endidx;
  /* A ptr to a 'SyntaxObject' structure, for things we want to add to the hashmap. */
  SyntaxObject *obj;
  /* Ptr for macros, used when parsing defines. */
  CSyntaxMacro *macro;
  /* Iterate thrue all lines in the syntax file, starting from the top. */
  ITER_SFL_TOP(sf, line,
    data = line->data;
    whitelen = indentlen(data);
    /* This is most likely always a preprocessor line. */
    if (data[whitelen] == '#') {
      /* Move the ptr to after the '#' char */
      data += (whitelen + 1);
      /* Get the length to next char. */
      whitelen = indentlen(data);
      /* If the string end at the end of the white after the '#' char or directly after, add a error to the syntax-file. */
      if (!data[whitelen]) {
        syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "# needs to be folowed by a preprocessor directive");
      }
      /* Check if this is a 'define' and if so, process it. */
      else if (strncmp((data + whitelen), S__LEN("define")) == 0 && isblankornulc(data + whitelen + STRLEN("define"))) {
        /* Move the ptr to after 'define'. */
        data += (whitelen + STRLEN("define"));
        /* Get the length to the next char. */
        whitelen = indentlen(data);
        /* If the end of the line is after 'define', there is no name so add the error. */
        if (!data[whitelen]) {
          syntaxfile_adderror(sf, line->lineno, ((data + whitelen) - line->data), "Macro must have a name");
        }
        /* Otherwise get the macro data. */
        else {
          /* Get the index of the end of the define name. */
          endidx = wordendindex((data + whitelen), 0, TRUE);
          /* Get the name. */
          ptr = measured_copy((data + whitelen), endidx);
          /* Create a syntax-object for the define. */
          obj = syntaxobject_create();
          /* Set the data of the object. */
          syntaxobject_setpos(obj, line->lineno, ((data + whitelen) - line->data));
          syntaxobject_settype(obj, SYNTAX_OBJECT_TYPE_C_MACRO);
          syntaxobject_setcolor(obj, SYNTAX_COLOR_BLUE);
          /* Add the 'SyntaxObject' to the 'sf'. */
          syntaxfile_addobject(sf, ptr, obj);
          /* Free and reset the ptr we used for the name of the macro. */
          free(ptr);
          ptr = NULL;
          /* Move the ptr to the end of the macro name. */
          data += (whitelen + endidx);
          /* And get the index of the first non blank char. */
          whitelen = indentlen(data);
          /* Create the macro. */
          macro = csyntaxmacro_create();
          /* If the macro is empty, meaning it does not exand into anything, set the empty flag. */
          if (!data[whitelen]) {
            macro->empty = TRUE;
          }
          /* This macro has args. */
          else if (data[0] == '(') {
            csyntax_macroargv(sf, macro, &line, &data);
          }
          /* Advance the ptr to the first non blank char. */
          whitelen = indentlen(data); 
          data += whitelen;
          whitelen = 0;
          /* Parse the full expansion of this macro. */
          macroexpantion(sf, macro, &line, &data);
          syntaxobject_setdata(obj, macro, csyntaxmacro_free);
        }
      }
    }
  );
}

void syntaxfile_parse_csyntax(SyntaxFile *const sf) {
  ASSERT(sf);
  const char *data;
  char *ptr;
  Ulong endidx;
  SyntaxObject *obj;
  CSyntaxMacro *macro;
  /* Iter all lines in the syntax file. */
  ITER_SFL_TOP(sf, line,
    /* Start by assigning the data ptr to the first non blank char in the line. */
    data = &line->data[indentlen(line->data)];
    /* If this line is a preprocessor line. */
    if (*data == '#') {
      data += (indentlen(data + STRLEN("#")) + STRLEN("#"));
      /* 'EOL' here means an error. */
      if (!*data) {
        syntaxfile_adderror(sf, line->lineno, (data - line->data), "'#' Needs a preprocessor directive");
      }
      /* Define directive. */
      else if (strncmp(data, S__LEN("define")) == 0 && isblankornulc(data + STRLEN("define"))) {
        data += (indentlen(data + STRLEN("define")) + STRLEN("define"));
        /* 'EOL' here means this macro has no name, so add a error. */
        if (!*data) {
          syntaxfile_adderror(sf, line->lineno, (data - line->data), "Macro must have a name");
        }
        else {
          endidx = wordendindex(data, 0, TRUE);
          /* The macro name is invalid. */
          if (!endidx) {
            syntaxfile_adderror(sf, line->lineno, (data - line->data), "Macro name is invalid");
          }
          else {
            /* Create the syntaxfile object. */
            obj = syntaxobject_create();
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
            if (*data == '(') {
              csyntax_macroargv(sf, macro, &line, &data);
              if (*data != ')') {
                syntaxfile_adderror(sf, line->lineno, (data - line->data), "Failed to parse macro arguments");
              }
              else {
                data += STRLEN(")");
              }
            }
            data += indentlen(data);
            macroexpantion(sf, macro, &line, &data);
            syntaxobject_setdata(obj, macro, csyntaxmacro_free);
          }
        }
      }
    }
  );
}

/* --------------------- CSyntaxMacro --------------------- */

/* Create a blank allocated `CSyntaxMacro` structure. */
CSyntaxMacro *csyntaxmacro_create(void) {
  CSyntaxMacro *macro = xmalloc(sizeof(*macro));
  macro->args        = cvec_create_setfree(free);
  macro->argv        = NULL;
  macro->expanded    = NULL;
  macro->expandstart = syntaxfilepos_create(0, 0);
  macro->expandend   = syntaxfilepos_create(0, 0);
  macro->empty       = FALSE;
  return macro;
}

void csyntaxmacro_free(void *ptr) {
  CSyntaxMacro *macro = ptr;
  ASSERT(macro);
  cvec_free(macro->args);
  free_nulltermchararray(macro->argv);
  free(macro->expanded);
  syntaxfilepos_free(macro->expandstart);
  syntaxfilepos_free(macro->expandend);
  free(macro);
}
