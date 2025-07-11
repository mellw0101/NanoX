/** @file help.c

  @author  Melwin Svensson.
  @date    2-6-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* static */ char *help_text = NULL;

/* The point in the help text where the shortcut descriptions begin. */
char *end_of_help_intro = NULL;

/* The point in the help text just after the title. */
const char *start_of_help_body = NULL;

/* The offset (in bytes) of the topleft of the shown help text. */
Ulong help_location;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Help init ----------------------------- */

/* Allocate space for the help text for the current menu,
 * and concatenate the different pieces of text into it.
 * The help text is divided into three parts:
 * - The untranslated introduction,
 * - The untranslated function key list, and
 * - The untranslated function key descriptions.
 * The function key list is built by iterating over all functions,
 * and for each function, iterating over all shortcuts.
 * The function key descriptions are built by iterating over all functions. */
/* static */ void help_init(void) {
  int tally;
  int maximum = 0;
  int counter = 0;
  /* Space needed for help_text */
  Ulong allocsize = 0;
  Ulong onoff_len;
  /* Untranslated help introduction.  We break it up into three chunks
   * in case the full string is too long for the compiler to handle...?
   * What kind of shit compiler were they thinking of like what? */
  const char *htx[3];
  const funcstruct *f;
  const keystruct  *s;
  char *ptr;
  /* First, set up the initial help text for the current function. */
  if (currmenu & (MWHEREIS | MREPLACE)) {
    htx[0] = N_("Search Command Help Text\n\n "
                "Enter the words or characters you would like to "
                "search for, and then press Enter.  If there is a "
                "match for the text you entered, the screen will be "
                "updated to the location of the nearest match for the "
                "search string.\n\n The previous search string will be "
                "shown in brackets after the search prompt.  Hitting "
                "Enter without entering any text will perform the "
                "previous search.  ");
    htx[1] = N_("If you have selected text with the mark and then "
                "search to replace, only matches in the selected text "
                "will be replaced.\n\n The following function keys are "
                "available in Search mode:\n\n");
    htx[2] = NULL;
  }
  else if (currmenu == MREPLACEWITH) {
    htx[0] = N_("=== Replacement ===\n\n "
                "Type the characters that should replace what you "
                "typed at the previous prompt, and press Enter.\n\n");
    htx[1] = N_(" The following function keys "
                "are available at this prompt:\n\n");
    htx[2] = NULL;
  }
  else if (currmenu == MGOTOLINE) {
    htx[0] = N_("Go To Line Help Text\n\n "
                "Enter the line number that you wish to go to and hit "
                "Enter.  If there are fewer lines of text than the "
                "number you entered, you will be brought to the last "
                "line of the file.\n\n The following function keys are "
                "available in Go To Line mode:\n\n");
    htx[1] = NULL;
    htx[2] = NULL;
  }
  else if (currmenu == MINSERTFILE) {
    htx[0] = N_("Insert File Help Text\n\n "
                "Type in the name of a file to be inserted into the "
                "current file buffer at the current cursor "
                "location.\n\n If you have compiled nano with multiple "
                "file buffer support, and enable multiple file buffers "
                "with the -F or --multibuffer command line flags, the "
                "Meta-F toggle, or a nanorc file, inserting a file "
                "will cause it to be loaded into a separate buffer "
                "(use Meta-< and > to switch between file buffers).  ");
    htx[1] = N_("If you need another blank buffer, do not enter "
                "any filename, or type in a nonexistent filename at "
                "the prompt and press Enter.\n\n The following "
                "function keys are available in Insert File mode:\n\n");
    htx[2] = NULL;
  }
  else if (currmenu == MWRITEFILE) {
    htx[0] = N_("Write File Help Text\n\n "
                "Type the name that you wish to save the current file "
                "as and press Enter to save the file.\n\n If you have "
                "selected text with the mark, you will be prompted to "
                "save only the selected portion to a separate file.  To "
                "reduce the chance of overwriting the current file with "
                "just a portion of it, the current filename is not the "
                "default in this mode.\n\n The following function keys "
                "are available in Write File mode:\n\n");
    htx[1] = NULL;
    htx[2] = NULL;
  }
  else if (currmenu == MBROWSER) {
    htx[0] = N_("File Browser Help Text\n\n "
                "The file browser is used to visually browse the "
                "directory structure to select a file for reading "
                "or writing.  You may use the arrow keys or Page Up/"
                "Down to browse through the files, and S or Enter to "
                "choose the selected file or enter the selected "
                "directory.  To move up one level, select the "
                "directory called \"..\" at the top of the file "
                "list.\n\n The following function keys are available "
                "in the file browser:\n\n");
    htx[1] = NULL;
    htx[2] = NULL;
  }
  else if (currmenu == MWHEREISFILE) {
    htx[0] = N_("Browser Search Command Help Text\n\n "
                "Enter the words or characters you would like to "
                "search for, and then press Enter.  If there is a "
                "match for the text you entered, the screen will be "
                "updated to the location of the nearest match for the "
                "search string.\n\n The previous search string will be "
                "shown in brackets after the search prompt.  Hitting "
                "Enter without entering any text will perform the "
                "previous search.\n\n");
    htx[1] = N_(" The following function keys "
                "are available at this prompt:\n\n");
    htx[2] = NULL;
  }
  else if (currmenu == MGOTODIR) {
    htx[0] = N_("Browser Go To Directory Help Text\n\n "
                "Enter the name of the directory you would like to "
                "browse to.\n\n If tab completion has not been "
                "disabled, you can use the Tab key to (attempt to) "
                "automatically complete the directory name.\n\n The "
                "following function keys are available in Browser Go "
                "To Directory mode:\n\n");
    htx[1] = NULL;
    htx[2] = NULL;
  }
  else if (currmenu == MSPELL) {
    htx[0] = N_("Spell Check Help Text\n\n "
                "The spell checker checks the spelling of all text in "
                "the current file.  When an unknown word is "
                "encountered, it is highlighted and a replacement can "
                "be edited.  It will then prompt to replace every "
                "instance of the given misspelled word in the current "
                "file, or, if you have selected text with the mark, in "
                "the selected text.\n\n The following function keys "
                "are available in Spell Check mode:\n\n");
    htx[1] = NULL;
    htx[2] = NULL;
  }
  else if (currmenu == MEXECUTE) {
    htx[0] = N_("Execute Command Help Text\n\n "
                "This mode allows you to insert the output of a "
                "command run by the shell into the current buffer (or "
                "into a new buffer).  If the command is preceded by '|' "
                "(the pipe symbol), the current contents of the buffer "
                "(or marked region) will be piped to the command.  ");
    htx[1] = N_("If you just need another blank buffer, do not enter any "
                "command.\n\n You can also pick one of four tools, or cut a "
                "large piece of the buffer, or put the editor to sleep.\n\n");
    htx[2] = N_(" The following function keys "
                "are available at this prompt:\n\n");
  }
  else if (currmenu == MLINTER) {
    htx[0] = N_("=== Linter ===\n\n "
                "In this mode, the status bar shows an error message or "
                "warning, and the cursor is put at the corresponding "
                "position in the file.  With PageUp and PageDown you "
                "can switch to earlier and later messages.\n\n");
    htx[1] = N_(" The following function keys are "
                "available in Linter mode:\n\n");
    htx[2] = NULL;
  }
  else {
    /* Default to the main help list. */
    htx[0] = N_("Main nano help text\n\n "
                "The nano editor is designed to emulate the "
                "functionality and ease-of-use of the UW Pico text "
                "editor.  There are four main sections of the editor.  "
                "The top line shows the program version, the current "
                "filename being edited, and whether or not the file "
                "has been modified.  Next is the main editor window "
                "showing the file being edited.  The status line is "
                "the third line from the bottom and shows important "
                "messages.  ");
    htx[1] = N_("The bottom two lines show the most commonly used "
                "shortcuts in the editor.\n\n Shortcuts are written as "
                "follows: Control-key sequences are notated with a '^' "
                "and can be entered either by using the Ctrl key or "
                "pressing the Esc key twice.  Meta-key sequences are "
                "notated with 'M-' and can be entered using either the "
                "Alt, Cmd, or Esc key, depending on your keyboard setup.  ");
    htx[2] = N_("Also, pressing Esc twice and then typing a "
                "three-digit decimal number from 000 to 255 will enter "
                "the character with the corresponding value.  The "
                "following keystrokes are available in the main editor "
                "window.  Alternative keys are shown in "
                "parentheses:\n\n");
  }
  /* Translate all parts of the help text. */
  htx[0] = _(htx[0]);
  if (htx[1]) {
    htx[1] = _(htx[1]);
  }
  if (htx[2]) {
    htx[2] = _(htx[2]);
  }
  /* Then get the length of all text combined. */
  allocsize += strlen(htx[0]);
  if (htx[1]) {
    allocsize += strlen(htx[1]);
  }
  if (htx[2]) {
    allocsize += strlen(htx[2]);
  }
  /* Calculate the length of the descriptions of the shortcuts.  Each entry has one or
   * two keystrokes, which fill 17 cells, plus translated text, plus one or two LF's. */
  DLIST_ND_FOR_NEXT(allfuncs, f) {
    if (f->menus & currmenu) {
      allocsize += (strlen(_(f->phrase)) + 21);
    }
  }
  /* If we're on the main list, we also count the toggle help text.  Each entry has "M-%c\t\t ",
   * six chars which fill 17 cells, plus two translated texts, plus a space, plus one or two LF's. */
  if (currmenu == MMAIN) {
    onoff_len = strlen(_("enable/disable"));
    DLIST_ND_FOR_NEXT(sclist, s) {
      if (s->func == do_toggle) {
        allocsize += (strlen(_(epithet_flag[s->toggle])) + onoff_len + 9);
      }
    }
  }
  /* Allocate memory for the help-text. */
  help_text = xmalloc(allocsize + 1);
  /* Now add the text we want. */
  strcpy(help_text, htx[0]);
  if (htx[1]) {
    strcat(help_text, htx[1]);
  }
  if (htx[2]) {
    strcat(help_text, htx[2]);
  }
  /* Remember this end-of-introduction, start-of-shortcuts. */
  end_of_help_intro = (help_text + strlen(help_text));
  ptr = end_of_help_intro;
  /* Now add the shortcuts and their descriptions. */
  DLIST_ND_FOR_NEXT(allfuncs, f) {
    tally = 0;
    if (!(f->menus & currmenu)) {
      continue;
    }
    /* Show the first two shortcuts (if any) for each function. */
    DLIST_ND_FOR_NEXT(sclist, s) {
      if ((s->menus & currmenu) && s->func == f->func && s->keystr[0]) {
        /* Make the first column 7 cells wide and the second 10. */
        if (++tally == 1) {
          sprintf(ptr, "%s                ", s->keystr);
          /* Unicode arrows take three bytes instead of one. */
          ptr += (strstr(s->keystr, "\xE2") ? 9 : 7);
        }
        else {
          sprintf(ptr, "(%s)       ", s->keystr);
          ptr += (strstr(s->keystr, "\xE2") ? 12 : 10);
          break;
        }
      }
    }
    if (!tally) {
      ptr += sprintf(ptr, "\t\t ");
    }
    else if (tally == 1) {
      ptr += 10;
    }
    /* The shortcutr's description. */
    ptr += sprintf(ptr, "%s\n", _(f->phrase));
    if (f->blank_after) {
      ptr += sprintf(ptr, "\n");
    }
  }
  /* And the toggles. */
  if (currmenu == MMAIN) {
    /* First see how meny toggles there are. */
    DLIST_ND_FOR_NEXT(sclist, s) {
      maximum = ((s->toggle && s->ordinal > maximum) ? s->ordinal : maximum);
    }
    /* Now show them in the original order. */
    while (counter++ < maximum) {
      DLIST_ND_FOR_NEXT(sclist, s) {
        if (s->toggle && s->ordinal == counter) {
          ptr += sprintf(ptr, "%s\t\t %s %s\n", ((s->menus & MMAIN) ? s->keystr : ""), _(epithet_flag[s->toggle]), _("enable/disable"));
          /* Add a blank like between two groups...?  Wtf? */
          if (s->toggle == NO_SYNTAX) {
            ptr += sprintf(ptr, "\n");
          }
          break;
        }
      }
    }
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Wrap help text into buffer ----------------------------- */

/* Hard-wrap the concatenated help text, and write it into a new buffer. */
void wrap_help_text_into_buffer_for(openfilestruct **const start, openfilestruct **const open, int rows, int cols) {
  ASSERT(start);
  ASSERT(open);
  int length;
  int shim;
  char *oneline;
  const char *ptr = start_of_help_body;
  /* Avoid over-tight and over-wide paragraphs in the introductory text. */
  Ulong wrapping_point = CLAMP_INLINE(cols, 40, 74);
  Ulong sum = 0;
  make_new_buffer_for(start, open);
  /* Ensure there is a blank line at the top of the text, for esthetics. */
  if ((ISSET(MINIBAR) || !ISSET(EMPTY_LINE)) && rows > 6) {
    (*open)->current->data = xstrcpy((*open)->current->data, " ");
    (*open)->current->next = make_new_node((*open)->current);
    DLIST_ADV_NEXT((*open)->current);
  }
  /* Copy the help text into the just-created new buffer. */
  while (*ptr) {
    if (ptr == end_of_help_intro) {
      wrapping_point = CLAMP_MIN_INLINE(cols, 40);
    }
    if (ptr < end_of_help_intro || *(ptr - 1) == LF) {
      length  = break_line(ptr, wrapping_point, TRUE);
      oneline = xmalloc(length + 1);
      shim    = !(*(ptr + length - 1) == SP);
      snprintf(oneline, (length + shim), "%s", ptr);
    }
    else {
      length  = break_line(ptr, (CLAMP_MIN_INLINE(cols, 40) - 18), TRUE);
      oneline = xmalloc(length + 5);
      snprintf(oneline, (length + 5), "\t\t  %s", ptr);
    }
    (*open)->current->data = free_and_assign((*open)->current->data, oneline);
    ptr += length;
    if (*ptr != LF) {
      --ptr;
    }
    /* Create a new line, and then one more for each extra LF. */
    do {
      (*open)->current->next = make_new_node((*open)->current);
      DLIST_ADV_NEXT((*open)->current);
      (*open)->current->data = COPY_OF("");
    } while (*(++ptr) == LF);
  }
  (*open)->filebot = (*open)->current;
  (*open)->current = (*open)->filetop;
  remove_magicline_for(*open);
  find_and_prime_applicable_syntax_for(*open);
  prepare_for_display_for(*open);
  /* Move to the position in the file where we were before. */
  while (1) {
    sum += strlen((*open)->current->data);
    if (sum > help_location) {
      break;
    }
    DLIST_ADV_NEXT((*open)->current);
  }
  (*open)->edittop = (*open)->current;
}

/* Hard-wrap the concatenated help text, and write it into a new buffer.  Note that this is `context-safe`. */
void wrap_help_text_into_buffer(void) {
  FULL_CTX_CALL(wrap_help_text_into_buffer_for);
}

/* Hard-wrap the concatenated help text, and write it into a new buffer. */
// void wrap_help_text_into_buffer(void) {
//   /* Avoid overtight and overwide paragraphs in the introductory text. */
//   const char *ptr = start_of_help_body;
//   Ulong wrapping_point = (((COLS < 40) ? 40 : (COLS > 74) ? 74 : COLS) - sidebar);
//   Ulong sum = 0;
//   make_new_buffer();
//   /* Ensure there is a blank line at the top of the text, for esthetics. */
//   if ((ISSET(MINIBAR) || !ISSET(EMPTY_LINE)) && LINES > 6) {
//     openfile->current->data = realloc_strcpy(openfile->current->data, " ");
//     openfile->current->next = make_new_node(openfile->current);
//     openfile->current       = openfile->current->next;
//   }
//   /* Copy the help text into the just-created new buffer. */
//   while (*ptr) {
//     int   length, shim;
//     char *oneline;
//     if (ptr == end_of_help_intro) {
//       wrapping_point = (((COLS < 40) ? 40 : COLS) - sidebar);
//     }
//     if (ptr < end_of_help_intro || *(ptr - 1) == '\n') {
//       length  = break_line(ptr, wrapping_point, TRUE);
//       oneline = xmalloc(length + 1);
//       shim    = ((*(ptr + length - 1) == ' ') ? 0 : 1);
//       snprintf(oneline, (length + shim), "%s", ptr);
//     }
//     else {
//       length  = break_line(ptr, (((COLS < 40) ? 22 : (COLS - 18)) - sidebar), TRUE);
//       oneline = xmalloc(length + 5);
//       snprintf(oneline, (length + 5), "\t\t  %s", ptr);
//     }
//     free(openfile->current->data);
//     openfile->current->data = oneline;
//     ptr += length;
//     if (*ptr != '\n') {
//       --ptr;
//     }
//     /* Create a new line, and then one more for each extra \n. */
//     do {
//       openfile->current->next = make_new_node(openfile->current);
//       openfile->current       = openfile->current->next;
//       openfile->current->data = COPY_OF("");
//     } while (*(++ptr) == '\n');
//   }
//   openfile->filebot = openfile->current;
//   openfile->current = openfile->filetop;
//   remove_magicline();
//   find_and_prime_applicable_syntax();
//   prepare_for_display();
//   /* Move to the position in the file where we were before. */
//   while (TRUE) {
//     sum += strlen(openfile->current->data);
//     if (sum > help_location) {
//       break;
//     }
//     openfile->current = openfile->current->next;
//   }
//   openfile->edittop = openfile->current;
// }
