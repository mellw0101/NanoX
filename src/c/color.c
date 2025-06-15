/** @file color.c

  @author  Melwin Svensson.
  @date    23-5-2025.

 */
#include "../include/c_proto.h"
#ifdef HAVE_MAGIC_H
# include <magic.h>
#endif


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#ifdef HAVE_MAGIC_H
# ifdef DEBUG
#   define MAGIC_FLAGS  (MAGIC_SYMLINK | MAGIC_DEBUG | MAGIC_CHECK | MAGIC_ERROR)
# else
#   define MAGIC_FLAGS  (MAGIC_SYMLINK | MAGIC_ERROR)
# endif
#endif


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether ncurses accepts -1 to mean "default color". */
static bool defaults_allowed = FALSE;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Try to match the given shibboleth string with, one of the regexes in the list starting at head.  Return 'TRUE' upon success. */
static bool found_in_list(const regexlisttype *const head, const char *const restrict shibboleth) {
  DLIST_FOR_NEXT(head, item) {
    if (regexec(item->one_rgx, shibboleth, 0, NULL, 0) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Initilize the color pairs for `file`. */
void prepare_palette_for(openfilestruct *const file) {
  ASSERT(file);
  short number;
  if (IN_CURSES_CTX) {
    number = NUMBER_OF_ELEMENTS;
    /* For each unique pair number, tell ncurses to combination of colors. */
    for (colortype *ink=file->syntax->color; ink; ink=ink->next) {
      if (ink->pairnum > number) {
        init_pair(ink->pairnum, ink->fg, ink->bg);
        number = ink->pairnum;
      }
    }
    have_palette = TRUE;
  }
}

/* Initialize the color pairs for the currently open file. */
void prepare_palette(void) {
  prepare_palette_for(CTX_OF);
}

/* Determine whether the matches of multiline regexes are still the same, and if not, schedule a screen refresh, so things will be repainted. */
void check_the_multis_for(openfilestruct *const file, linestruct *const line) {
  ASSERT(file);
  ASSERT(line);
  const colortype *ink;
  regmatch_t startmatch;
  regmatch_t endmatch;
  bool  astart;
  bool  anend;
  char *afterstart;
  /* If there is no syntax or no multiline regex, there is nothing to do. */
  if (!IN_CURSES_CTX || !file->syntax || !file->syntax->multiscore) {
    return;
  }
  if (!line->multidata) {
    refresh_needed = TRUE;
    return;
  }
  for (ink=file->syntax->color; ink; ink=ink->next) {
    /* If it's not a multiline regex, skip. */
    if (!ink->end) {
      continue;
    }
    astart     = (regexec(ink->start, line->data, 1, &startmatch, 0) == 0);
    afterstart = (line->data + (astart ? startmatch.rm_eo : 0));
    anend      = (regexec(ink->end, afterstart, 1, &endmatch, 0) == 0);
    /* Check whether the multidata still matches the current situation. */
    if (line->multidata[ink->id] == NOTHING) {
      if (!astart) {
        continue;
      }
    }
    else if (line->multidata[ink->id] == WHOLELINE) {
      /* Ensure that a detected start match is not actually an end match. */
      if (!anend && (!astart || regexec(ink->end, line->data, 1, &endmatch, 0) != 0)) {
        continue;
      }
    }
    else if (line->multidata[ink->id] == JUSTONTHIS) {
      if (astart && anend && regexec(ink->start, line->data + startmatch.rm_eo + endmatch.rm_eo, 1, &startmatch, 0) != 0) {
        continue;
      }
    }
    else if (line->multidata[ink->id] == STARTSHERE) {
      if (astart && !anend) {
        continue;
      }
    }
    else if (line->multidata[ink->id] == ENDSHERE) {
      if (!astart && anend) {
        continue;
      }
    }
    /* There is a mismatch, so something changed: repaint. */
    refresh_needed = TRUE;
    perturbed      = TRUE;
    return;
  }
}

/* Determine whether the matches of multiline regexes are still the same, and if not, schedule a screen refresh, so things will be repainted. */
void check_the_multis(linestruct *const line) {
  check_the_multis_for(CTX_OF, line);
}

/* Initialize the color pairs for nano's interface.  Ask ncurses to allow -1 to
 * mean "default color".  Initialize the color pairs for nano's interface elements. */
void set_interface_colorpairs(void) {
  if (!IN_CURSES_CTX) {
    return;
  }
  /* Ask ncurses to allow -1 to mean "default color". */
  defaults_allowed = (use_default_colors() == OK);
  /* Initialize the color pairs for nano's interface elements. */
  for (Ulong index=0; index<NUMBER_OF_ELEMENTS; ++index) {
    colortype *combo = color_combo[index];
    if (combo) {
      if (!defaults_allowed) {
        if (combo->fg == THE_DEFAULT) {
          combo->fg = COLOR_WHITE;
        }
        if (combo->bg == THE_DEFAULT) {
          combo->bg = COLOR_BLACK;
        }
      }
      init_pair((index + 1), combo->fg, combo->bg);
      interface_color_pair[index] = (COLOR_PAIR(index + 1) | combo->attributes);
      rescind_colors = FALSE;
    }
    else {
      if (index == FUNCTION_TAG || index == SCROLL_BAR) {
        interface_color_pair[index] = A_NORMAL;
      }
      else if (index == GUIDE_STRIPE) {
        init_pair((index + 1), COLOR_BLUE, COLOR_BLACK);
        interface_color_pair[index] = (COLOR_PAIR(index + 1) | A_BOLD);
      }
      else if (index == SPOTLIGHTED) {
        init_pair((index + 1), COLOR_BLACK, (COLOR_YELLOW + ((COLORS > 15) ? 8 : 0)));
        interface_color_pair[index] = COLOR_PAIR(index + 1);
      }
      else if (index == MINI_INFOBAR || index == PROMPT_BAR) {
        interface_color_pair[index] = interface_color_pair[TITLE_BAR];
      }
      else if (index == ERROR_MESSAGE) {
        init_pair(index + 1, COLOR_WHITE, COLOR_RED);
        interface_color_pair[index] = (COLOR_PAIR(index + 1) | A_BOLD);
      }
      else if (index == LINE_NUMBER) {
        init_pair((index + 1), XTERM_GREY_2, (defaults_allowed ? THE_DEFAULT : COLOR_BLACK));
        interface_color_pair[index] = COLOR_PAIR(index + 1);
      }
      else if (index >= FG_BLUE && index <= FG_MAUVE) {
        color_combo[index] = xmalloc(sizeof(*(color_combo[index])));
        short color        = (index == FG_BLUE)    ? COLOR_BLUE
                           : (index == FG_MAGENTA) ? COLOR_MAGENTA
                           : (index == FG_GREEN)   ? COLOR_GREEN
                           : (index == FG_LAGOON)  ? COLOR_LAGOON
                           : (index == FG_YELLOW)  ? COLOR_YELLOW
                           : (index == FG_RED)     ? COLOR_RED
                           : (index == FG_PINK)    ? COLOR_PINK
                           : (index == FG_TEAL)    ? COLOR_TEAL
                           : (index == FG_MINT)    ? COLOR_MINT
                           : (index == FG_PURPLE)  ? COLOR_PURPLE
                           : (index == FG_MAUVE)   ? COLOR_MAUVE
                                                   : 1;
        init_pair((index + 1), color, (defaults_allowed ? THE_DEFAULT : COLOR_BLACK));
        interface_color_pair[index] = COLOR_PAIR(index + 1) | A_BOLD;
      }
      else if (index >= FG_VS_CODE_START && index <= FG_VS_CODE_END) {
        init_pair((index + 1), color_array[index - FG_VS_CODE_RED], (defaults_allowed ? THE_DEFAULT : COLOR_BLACK));
        interface_color_pair[index] = COLOR_PAIR(index + 1);
      }
      else if (index >= BG_VS_CODE_START && index <= BG_VS_CODE_END) {
        if (COLORS > 256) {
          if (index == BG_VS_CODE_RED) {
            init_extended_pair((index + 1), COLOR_BLACK, VS_CODE_RED);
          }
          else if (index == BG_VS_CODE_GREEN) {
            init_extended_pair((index + 1), COLOR_BLACK, 50000);
          }
          else if (index == BG_VS_CODE_BLUE) {
            init_extended_pair((index + 1), COLOR_BLACK, XTERM_DIRECT_SOFT_BLUE);
          }
        }
        else if (COLORS > 15) {
          init_pair((index + 1), xterm_color_index(0, 0, 0), BG_COLOR(index));
        }
        else {
          if (index == BG_VS_CODE_RED) {
            init_pair((index + 1), COLOR_BLACK, COLOR_RED);
          }
          else if (index == BG_VS_CODE_BLUE) {
            init_pair((index + 1), COLOR_BLACK, COLOR_BLUE);
          }
          else if (index == BG_VS_CODE_GREEN) {
            init_pair((index + 1), COLOR_BLACK, COLOR_GREEN);
          }
        }
        interface_color_pair[index] = COLOR_PAIR(index + 1);
      }
      else {
        interface_color_pair[index] = hilite_attribute;
      }
    }
    if (index < FG_VS_CODE_RED && index != LINE_NUMBER) {
      free(color_combo[index]);
    }
  }
  if (rescind_colors) {
    interface_color_pair[SPOTLIGHTED]   = A_REVERSE;
    interface_color_pair[ERROR_MESSAGE] = A_REVERSE;
  }
}

/* Assign a pair number to each of the foreground/background color combinations in the given syntax, giving identical combinations the same number. */
void set_syntax_colorpairs(syntaxtype *const sntx) {
  short number;
  colortype *older;
  if (!IN_CURSES_CTX) {
    return;
  }
  number = NUMBER_OF_ELEMENTS;
  DLIST_FOR_NEXT(sntx->color, ink) {
    if (!defaults_allowed) {
      if (ink->fg == THE_DEFAULT) {
        ink->fg = COLOR_WHITE;
      }
      if (ink->bg == THE_DEFAULT) {
        ink->bg = COLOR_BLACK;
      }
    }
    older = sntx->color;
    while (older != ink && (older->fg != ink->fg || older->bg != ink->bg)) {
      older = older->next;
    }
    ink->pairnum = (older != ink) ? older->pairnum : ++number;
    ink->attributes |= COLOR_PAIR(ink->pairnum);
  }
}

/* Find a syntax that applies to `file`, based upon filename or buffer content, and load and prime this syntax when needed. */
void find_and_prime_applicable_syntax_for(openfilestruct *const file) {
  ASSERT(file);
  syntaxtype *sntx = NULL;
  char *fullname;
# ifdef HAVE_LIBMAGIC
  struct stat  fileinfo;
  magic_t      cookie      = NULL;
  const char * magicstring = NULL;
# endif
  /* If the rcfiles were not read, or contained no syntaxes, get out. */
  if (!IN_CURSES_CTX || !syntaxes) {
    return;
  }
  /* If we specified a syntax-override string, use it. */
  if (syntaxstr) {
    /* An override of "none" is like having no syntax at all. */
    if (strcmp(syntaxstr, "none") == 0) {
      return;
    }
    DLIST_ND_FOR_NEXT(syntaxes, sntx) {
      if (strcmp(sntx->name, syntaxstr) == 0) {
        break;
      }
    }
    if (!sntx && !inhelp) {
      statusline_curses(ALERT, _("Unknown syntax name: %s"), syntaxstr);
    }
  }
  /* If no syntax-override string was specified, or it didn't match,
   * try finding a syntax based on the filename (extension). */
  if (!sntx && !inhelp) {
    fullname = get_full_path(file->filename);
    if (!fullname) {
      fullname = realloc_strcpy(fullname, file->filename);
    }
    DLIST_ND_FOR_NEXT(syntaxes, sntx) {
      if (found_in_list(sntx->extensions, fullname)) {
        break;
      }
    }
    free(fullname);
  }
  /* If the filename didn't match anything, try the first line. */
  if (!sntx && !inhelp) {
    for (sntx = syntaxes; sntx; sntx = sntx->next) {
      if (found_in_list(sntx->headers, file->filetop->data)) {
        break;
      }
    }
  }
// # define HAVE_LIBMAGIC
# ifdef HAVE_LIBMAGIC
  /* If we still don't have an answer, try using magic (when requested). */
  if (!sntx && !inhelp && ISSET(USE_MAGIC)) {
    if (stat(file->filename, &fileinfo) == 0) {
      /* Open the magic database and get a diagnosis of the file. */
      cookie = magic_open(MAGIC_FLAGS);
      if (!cookie || magic_load(cookie, NULL) < 0) {
        statusline(ALERT, _("magic_load() failed: %s"), strerror(errno));
      }
      else {
        magicstring = magic_file(cookie, file->filename);
        if (!magicstring) {
          statusline_curses(ALERT, _("magic_file(%s) failed: %s"), file->filename, magic_error(cookie));
        }
      }
    }
    /* Now try and find a syntax that matches the magic string. */
    if (magicstring) {
      DLIST_ND_FOR_NEXT(syntaxes, sntx) {
        if (found_in_list(sntx->magics, magicstring)) {
          break;
        }
      }
    }
    if (!stat(file->filename, &fileinfo)) {
      magic_close(cookie);
    }
  }
# endif
  /* If nothing at all matched, see if there is a default syntax. */
  if (!sntx && !inhelp) {
    DLIST_ND_FOR_NEXT(syntaxes, sntx) {
      if (strcmp(sntx->name, "default") == 0) {
        break;
      }
    }
  }
  /* When the syntax isn't loaded yet, parse it and initialize its colors. */
  if (sntx && sntx->filename) {
    parse_one_include(sntx->filename, sntx);
    set_syntax_colorpairs(sntx);
  }
  file->syntax = sntx;
}

/* Find a syntax that applies to the current buffer, based upon filename or buffer content, and load and prime this syntax when needed. */
void find_and_prime_applicable_syntax(void) {
  find_and_prime_applicable_syntax_for(CTX_OF);
}

/* Precalculate the multi-line start and end regex info so we can speed up rendering (with any hope at all...). */
void precalc_multicolorinfo_for(openfilestruct *const file) {
  ASSERT(file);
  const colortype *ink;
  regmatch_t startmatch, endmatch;
  linestruct *line, *tailline;
  if (!IN_CURSES_CTX || ISSET(NO_SYNTAX) || !file->syntax || !file->syntax->multiscore) {
    return;
  }
  /* For each line, allocate cache space for the multiline-regex info. */
  for (line = file->filetop; line; line = line->next) {
    if (!line->multidata) {
      line->multidata = xmalloc(file->syntax->multiscore * sizeof(short));
    }
  }
  for (ink=file->syntax->color; ink; ink=ink->next) {
    /* If this is not a multi-line regex, skip it. */
    if (!ink->end) {
      continue;
    }
    for (line=file->filetop; line; line=line->next) {
      int index = 0;
      /* Assume nothing applies until proven otherwise below. */
      line->multidata[ink->id] = NOTHING;
      /* When the line contains a start match, look for an end, and if found, mark all the lines that are affected. */
      while (regexec(ink->start, (line->data + index), 1, &startmatch, ((index == 0) ? 0 : REG_NOTBOL)) == 0) {
        /* Begin looking for an end match after the start match. */
        index += startmatch.rm_eo;
        /* If there is an end match on this same line, mark the line, but continue looking for other starts after it. */
        if (regexec(ink->end, (line->data + index), 1, &endmatch, ((index == 0) ? 0 : REG_NOTBOL)) == 0) {
          line->multidata[ink->id] = JUSTONTHIS;
          index += endmatch.rm_eo;
          /* If the total match has zero length, force an advance. */
          if ((startmatch.rm_eo - startmatch.rm_so + endmatch.rm_eo) == 0) {
            /* When at end-of-line, there is no other start. */
            if (!line->data[index]) {
              break;
            }
            index = step_right(line->data, index);
          }
          continue;
        }
        /* Look for an end match on later lines. */
        tailline = line->next;
        while (tailline && regexec(ink->end, tailline->data, 1, &endmatch, 0) != 0) {
          tailline = tailline->next;
        }
        line->multidata[ink->id] = STARTSHERE;
        /* Note that this also advances the line in the main loop. */
        for (line = line->next; line != tailline; line = line->next) {
          line->multidata[ink->id] = WHOLELINE;
        }
        if (!tailline) {
          line = file->filebot;
          break;
        }
        tailline->multidata[ink->id] = ENDSHERE;
        /* Look for a possible new start after the end match. */
        index = endmatch.rm_eo;
      }
    }
  }
}

/* Precalculate the multi-line start and end regex info so we can speed up rendering (with any hope at all...). */
void precalc_multicolorinfo(void) {
  precalc_multicolorinfo_for(CTX_OF);
}
