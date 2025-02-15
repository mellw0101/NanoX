/** @file csyntax.c

  @author  Melwin Svensson.
  @date    12-2-2025.

 */
#include "../../include/c_proto.h"

#include "../../include/c/wchars.h"


_UNUSED static void macroargv(const char *const __restrict ptr, char ***const argv) {
  ASSERT(ptr);
  ASSERT(argv);
  /* The data. */
  const char *data = ptr;
  /* The length of blank chars to the next char. */
  // Ulong whitelen;
  /* End index of a word. */
  // Ulong endidx;
  ALWAYS_ASSERT(*data == '(');
  /* Move right once past the '(' char. */
  data += step_right(data, 0);
  // whitelen = indentlen(data);
  printf("%s\n", data);
}


/* Process a `c` based `SyntaxFile`. */
void process_syntaxfile_c(SyntaxFile *const sf) {
  /* Assert the 'SyntaxFile'. */
  ASSERT(sf);
  TIMER_START(timer);
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
          ALWAYS_ASSERT((endidx = wordendindex((data + whitelen), 0, TRUE)) != whitelen);
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
          /* Otherwise, get the macro expansion. */
          else {
            /* This macro could have args. */
            if (*(data + whitelen) == '(') {
              char **argv;
              macroargv((data + whitelen), &argv);
            }
            printf("%s\n", (data + whitelen));
          }
          syntaxobject_setdata(obj, macro, csyntaxmacro_free);
        }
      }
    }
  );
  TIMER_END(timer, ms);
  TIMER_PRINT(ms);
}


/* --------------------- CSyntaxMacro --------------------- */


/* Create a blank allocated `CSyntaxMacro` structure. */
CSyntaxMacro *csyntaxmacro_create(void) {
  CSyntaxMacro *macro = xmalloc(sizeof(*macro));
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
  free_nulltermchararray(macro->argv);
  free(macro->expanded);
  syntaxfilepos_free(macro->expandstart);
  syntaxfilepos_free(macro->expandend);
  free(macro);
}
