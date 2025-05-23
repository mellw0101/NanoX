/** @file color.c

  @author  Melwin Svensson.
  @date    23-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Whether ncurses accepts -1 to mean "default color". */
static bool defaults_allowed = FALSE;


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Initilize the color pairs for `file`. */
void prepare_palette_for(openfilestruct *const file) {
  ASSERT(file);
  short number;
  if (ISSET(USING_GUI)) {
    return;
  }
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

/* Initialize the color pairs for the currently open file. */
void prepare_palette(void) {
  if (ISSET(USING_GUI)) {
    return;
  }
  prepare_palette_for(openfile);
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
  if (ISSET(USING_GUI) || !file->syntax || !file->syntax->multiscore) {
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
  if (ISSET(USING_GUI)) {
    return;
  }
  check_the_multis_for(openfile, line);
}

/* Initialize the color pairs for nano's interface.  Ask ncurses to allow -1 to
 * mean "default color".  Initialize the color pairs for nano's interface elements. */
void set_interface_colorpairs(void) {
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
  if (ISSET(USING_GUI)) {
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

/* Try to match the given shibboleth string with, one of the regexes in the list starting at head.  Return 'TRUE' upon success. */
_UNUSED static bool found_in_list(const regexlisttype *const head, const char *const restrict shibboleth) {
  DLIST_FOR_NEXT(head, item) {
    if (regexec(item->one_rgx, shibboleth, 0, NULL, 0) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}
