/**************************************************************************
 *   color.cpp  --  This file is part of GNU nano.                        *
 *                                                                        *
 *   Copyright (C) 2001-2011, 2013-2024 Free Software Foundation, Inc.    *
 *   Copyright (C) 2014-2017, 2020, 2021 Benno Schulenberg                *
 *                                                                        *
 *   GNU nano is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   GNU nano is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "../include/prototypes.h"

#ifdef HAVE_MAGIC_H
# include <magic.h>
#endif

/* Whether ncurses accepts -1 to mean "default color". */
// static bool defaults_allowed = FALSE;

/* Initialize the color pairs for nano's interface.  Ask ncurses to allow -1 to
 * mean "default color".  Initialize the color pairs for nano's interface elements. */
// void set_interface_colorpairs(void) {
//   /* Ask ncurses to allow -1 to mean "default color". */
//   defaults_allowed = (use_default_colors() == OK);
//   /* Initialize the color pairs for nano's interface elements. */
//   for (Ulong index = 0; index < NUMBER_OF_ELEMENTS; ++index) {
//     colortype *combo = color_combo[index];
//     if (combo) {
//       if (!defaults_allowed) {
//         if (combo->fg == THE_DEFAULT) {
//           combo->fg = COLOR_WHITE;
//         }
//         if (combo->bg == THE_DEFAULT) {
//           combo->bg = COLOR_BLACK;
//         }
//       }
//       init_pair((index + 1), combo->fg, combo->bg);
//       interface_color_pair[index] = (COLOR_PAIR(index + 1) | combo->attributes);
//       rescind_colors = FALSE;
//     }
//     else {
//       if (index == FUNCTION_TAG || index == SCROLL_BAR) {
//         interface_color_pair[index] = A_NORMAL;
//       }
//       else if (index == GUIDE_STRIPE) {
//         init_pair((index + 1), COLOR_BLUE, COLOR_BLACK);
//         interface_color_pair[index] = (COLOR_PAIR(index + 1) | A_BOLD);
//       }
//       else if (index == SPOTLIGHTED) {
//         init_pair((index + 1), COLOR_BLACK, (COLOR_YELLOW + ((COLORS > 15) ? 8 : 0)));
//         interface_color_pair[index] = COLOR_PAIR(index + 1);
//       }
//       else if (index == MINI_INFOBAR || index == PROMPT_BAR) {
//         interface_color_pair[index] = interface_color_pair[TITLE_BAR];
//       }
//       else if (index == ERROR_MESSAGE) {
//         init_pair(index + 1, COLOR_WHITE, COLOR_RED);
//         interface_color_pair[index] = (COLOR_PAIR(index + 1) | A_BOLD);
//       }
//       else if (index == LINE_NUMBER) {
//         init_pair((index + 1), XTERM_GREY_2, (defaults_allowed ? THE_DEFAULT : COLOR_BLACK));
//         interface_color_pair[index] = COLOR_PAIR(index + 1);
//       }
//       else if (index >= FG_BLUE && index <= FG_MAUVE) {
//         color_combo[index] = (colortype *)nmalloc(sizeof(*(color_combo[index])));
//         short color        = (index == FG_BLUE)    ? COLOR_BLUE
//                            : (index == FG_MAGENTA) ? COLOR_MAGENTA
//                            : (index == FG_GREEN)   ? COLOR_GREEN
//                            : (index == FG_LAGOON)  ? COLOR_LAGOON
//                            : (index == FG_YELLOW)  ? COLOR_YELLOW
//                            : (index == FG_RED)     ? COLOR_RED
//                            : (index == FG_PINK)    ? COLOR_PINK
//                            : (index == FG_TEAL)    ? COLOR_TEAL
//                            : (index == FG_MINT)    ? COLOR_MINT
//                            : (index == FG_PURPLE)  ? COLOR_PURPLE
//                            : (index == FG_MAUVE)   ? COLOR_MAUVE
//                                                    : 1;
//         init_pair((index + 1), color, (defaults_allowed ? THE_DEFAULT : COLOR_BLACK));
//         interface_color_pair[index] = COLOR_PAIR(index + 1) | A_BOLD;
//       }
//       else if (index >= FG_VS_CODE_START && index <= FG_VS_CODE_END) {
//         init_pair((index + 1), color_array[index - FG_VS_CODE_RED], (defaults_allowed ? THE_DEFAULT : COLOR_BLACK));
//         interface_color_pair[index] = COLOR_PAIR(index + 1);
//       }
//       else if (index >= BG_VS_CODE_START && index <= BG_VS_CODE_END) {
//         if (COLORS > 256) {
//           if (index == BG_VS_CODE_RED) {
//             init_extended_pair((index + 1), COLOR_BLACK, VS_CODE_RED);
//           }
//           else if (index == BG_VS_CODE_GREEN) {
//             init_extended_pair((index + 1), COLOR_BLACK, 50000);
//           }
//           else if (index == BG_VS_CODE_BLUE) {
//             init_extended_pair((index + 1), COLOR_BLACK, XTERM_DIRECT_SOFT_BLUE);
//           }
//         }
//         else if (COLORS > 15) {
//           init_pair((index + 1), xterm_color_index(0, 0, 0), BG_COLOR(index));
//         }
//         else {
//           if (index == BG_VS_CODE_RED) {
//             init_pair((index + 1), COLOR_BLACK, COLOR_RED);
//           }
//           else if (index == BG_VS_CODE_BLUE) {
//             init_pair((index + 1), COLOR_BLACK, COLOR_BLUE);
//           }
//           else if (index == BG_VS_CODE_GREEN) {
//             init_pair((index + 1), COLOR_BLACK, COLOR_GREEN);
//           }
//         }
//         interface_color_pair[index] = COLOR_PAIR(index + 1);
//       }
//       else {
//         interface_color_pair[index] = hilite_attribute;
//       }
//     }
//     if (index < FG_VS_CODE_RED && index != LINE_NUMBER) {
//       free(color_combo[index]);
//     }
//   }
//   if (rescind_colors) {
//     interface_color_pair[SPOTLIGHTED]   = A_REVERSE;
//     interface_color_pair[ERROR_MESSAGE] = A_REVERSE;
//   }
// }

/* Assign a pair number to each of the foreground/background color combinations in the given syntax, giving identical combinations the same number. */
// void set_syntax_colorpairs(syntaxtype *sntx) {
//   short number = NUMBER_OF_ELEMENTS;
//   colortype *older;
//   for (colortype *ink = sntx->color; ink; ink = ink->next) {
//     if (!defaults_allowed) {
//       if (ink->fg == THE_DEFAULT) {
//         ink->fg = COLOR_WHITE;
//       }
//       if (ink->bg == THE_DEFAULT) {
//         ink->bg = COLOR_BLACK;
//       }
//     }
//     older = sntx->color;
//     while (older != ink && (older->fg != ink->fg || older->bg != ink->bg)) {
//       older = older->next;
//     }
//     ink->pairnum = (older != ink) ? older->pairnum : ++number;
//     ink->attributes |= COLOR_PAIR(ink->pairnum);
//   }
// }

/* Initialize the color pairs for the current syntax. */
// void prepare_palette(void) {
//   short number = NUMBER_OF_ELEMENTS;
//   /* For each unique pair number, tell ncurses the combination of colors. */
//   for (colortype *ink = openfile->syntax->color; ink; ink = ink->next) {
//     if (ink->pairnum > number) {
//       init_pair(ink->pairnum, ink->fg, ink->bg);
//       number = ink->pairnum;
//     }
//   }
//   have_palette = TRUE;
// }

/* Try to match the given shibboleth string with, one of the regexes in the list starting at head.  Return 'TRUE' upon success. */
// static bool found_in_list(regexlisttype *const head, const char *shibboleth) _GL_ATTRIBUTE_NOTHROW {
//   for (regexlisttype *item = head; item; item = item->next) {
//     if (regexec(item->one_rgx, shibboleth, 0, NULL, 0) == 0) {
//       return TRUE;
//     }
//   }
//   return FALSE;
// }

/* Find a syntax that applies to the current buffer, based upon filename or buffer content, and load and prime this syntax when needed. */
// void find_and_prime_applicable_syntax(void) {
//   syntaxtype *sntx = NULL;
//   /* If the rcfiles were not read, or contained no syntaxes, get out. */
//   if (!syntaxes) {
//     return;
//   }
//   /* If we specified a syntax-override string, use it. */
//   if (syntaxstr) {
//     /* An override of "none" is like having no syntax at all. */
//     if (strcmp(syntaxstr, "none") == 0) {
//       return;
//     }
//     for (sntx = syntaxes; sntx; sntx = sntx->next) {
//       if (strcmp(sntx->name, syntaxstr) == 0) {
//         break;
//       }
//     }
//     if (!sntx && !inhelp) {
//       statusline(ALERT, _("Unknown syntax name: %s"), syntaxstr);
//     }
//   }
//   /* If no syntax-override string was specified, or it didn't match,
//    * try finding a syntax based on the filename (extension). */
//   if (!sntx && !inhelp) {
//     char *fullname = get_full_path(openfile->filename);
//     if (!fullname) {
//       fullname = realloc_strcpy(fullname, openfile->filename);
//     }
//     for (sntx = syntaxes; sntx; sntx = sntx->next) {
//       if (found_in_list(sntx->extensions, fullname)) {
//         break;
//       }
//     }
//     free(fullname);
//   }
//   /* If the filename didn't match anything, try the first line. */
//   if (!sntx && !inhelp) {
//     for (sntx = syntaxes; sntx; sntx = sntx->next) {
//       if (found_in_list(sntx->headers, openfile->filetop->data)) {
//         break;
//       }
//     }
//   }
// #ifdef HAVE_LIBMAGIC
//   /* If we still don't have an answer, try using magic (when requested). */
//   if (!sntx && !inhelp && ISSET(USE_MAGIC)) {
//     struct stat fileinfo;
//     magic_t     cookie      = NULL;
//     const char *magicstring = NULL;
//     if (stat(openfile->filename, &fileinfo) == 0) {
//       /* Open the magic database and get a diagnosis of the file. */
//       cookie = magic_open(MAGIC_SYMLINK |
// #  ifdef DEBUG
//                           MAGIC_DEBUG | MAGIC_CHECK |
// #  endif
//                           MAGIC_ERROR);
//       if (!cookie || magic_load(cookie, NULL) < 0) {
//         statusline(ALERT, _("magic_load() failed: %s"), strerror(errno));
//       }
//       else {
//         magicstring = magic_file(cookie, openfile->filename);
//         if (!magicstring) {
//           statusline(ALERT, _("magic_file(%s) failed: %s"), openfile->filename, magic_error(cookie));
//         }
//       }
//     }
//     /* Now try and find a syntax that matches the magic string. */
//     if (magicstring) {
//       for (sntx = syntaxes; sntx; sntx = sntx->next) {
//         if (found_in_list(sntx->magics, magicstring)) {
//           break;
//         }
//       }
//     }
//     if (!stat(openfile->filename, &fileinfo)) {
//       magic_close(cookie);
//     }
//   }
// #endif
//   /* If nothing at all matched, see if there is a default syntax. */
//   if (!sntx && !inhelp) {
//     for (sntx = syntaxes; sntx; sntx = sntx->next) {
//       if (strcmp(sntx->name, "default") == 0) {
//         break;
//       }
//     }
//   }
//   /* When the syntax isn't loaded yet, parse it and initialize its colors. */
//   if (sntx && sntx->filename) {
//     parse_one_include(sntx->filename, sntx);
//     set_syntax_colorpairs(sntx);
//   }
//   openfile->syntax = sntx;
// }

/* Determine whether the matches of multiline regexes are still the same, and if not, schedule a screen refresh, so things will be repainted. */
// void check_the_multis(linestruct *line) _NOTHROW {
//   const colortype *ink;
//   regmatch_t startmatch;
//   regmatch_t endmatch;
//   bool  astart;
//   bool  anend;
//   char *afterstart;
//   /* If there is no syntax or no multiline regex, there is nothing to do. */
//   if (!openfile->syntax || !openfile->syntax->multiscore || ISSET(USING_GUI)) {
//     return;
//   }
//   if (!line->multidata) {
//     refresh_needed = TRUE;
//     return;
//   }
//   for (ink = openfile->syntax->color; ink; ink = ink->next) {
//     /* If it's not a multiline regex, skip. */
//     if (!ink->end) {
//       continue;
//     }
//     astart     = (regexec(ink->start, line->data, 1, &startmatch, 0) == 0);
//     afterstart = (line->data + (astart ? startmatch.rm_eo : 0));
//     anend      = (regexec(ink->end, afterstart, 1, &endmatch, 0) == 0);
//     /* Check whether the multidata still matches the current situation. */
//     if (line->multidata[ink->id] == NOTHING) {
//       if (!astart) {
//         continue;
//       }
//     }
//     else if (line->multidata[ink->id] == WHOLELINE) {
//       /* Ensure that a detected start match is not actually an end match. */
//       if (!anend && (!astart || regexec(ink->end, line->data, 1, &endmatch, 0) != 0)) {
//         continue;
//       }
//     }
//     else if (line->multidata[ink->id] == JUSTONTHIS) {
//       if (astart && anend && regexec(ink->start, line->data + startmatch.rm_eo + endmatch.rm_eo, 1, &startmatch, 0) != 0) {
//         continue;
//       }
//     }
//     else if (line->multidata[ink->id] == STARTSHERE) {
//       if (astart && !anend) {
//         continue;
//       }
//     }
//     else if (line->multidata[ink->id] == ENDSHERE) {
//       if (!astart && anend) {
//         continue;
//       }
//     }
//     /* There is a mismatch, so something changed: repaint. */
//     refresh_needed = TRUE;
//     perturbed      = TRUE;
//     return;
//   }
// }

/* Precalculate the multi-line start and end regex info so we can speed up rendering (with any hope at all...). */
// void precalc_multicolorinfo(void) _GL_ATTRIBUTE_NOTHROW {
//   PROFILE_FUNCTION;
//   const colortype *ink;
//   regmatch_t startmatch, endmatch;
//   linestruct *line, *tailline;
//   if (!openfile->syntax || !openfile->syntax->multiscore || ISSET(NO_SYNTAX)) {
//     return;
//   }
//   /* For each line, allocate cache space for the multiline-regex info. */
//   for (line = openfile->filetop; line; line = line->next) {
//     if (!line->multidata) {
//       line->multidata = (short *)nmalloc(openfile->syntax->multiscore * sizeof(short));
//     }
//   }
//   for (ink = openfile->syntax->color; ink; ink = ink->next) {
//     /* If this is not a multi-line regex, skip it. */
//     if (!ink->end) {
//       continue;
//     }
//     for (line = openfile->filetop; line; line = line->next) {
//       int index = 0;
//       /* Assume nothing applies until proven otherwise below. */
//       line->multidata[ink->id] = NOTHING;
//       /* When the line contains a start match, look for an end, and if found, mark all the lines that are affected. */
//       while (regexec(ink->start, (line->data + index), 1, &startmatch, ((index == 0) ? 0 : REG_NOTBOL)) == 0) {
//         /* Begin looking for an end match after the start match. */
//         index += startmatch.rm_eo;
//         /* If there is an end match on this same line, mark the line, but continue looking for other starts after it. */
//         if (regexec(ink->end, (line->data + index), 1, &endmatch, ((index == 0) ? 0 : REG_NOTBOL)) == 0) {
//           line->multidata[ink->id] = JUSTONTHIS;
//           index += endmatch.rm_eo;
//           /* If the total match has zero length, force an advance. */
//           if ((startmatch.rm_eo - startmatch.rm_so + endmatch.rm_eo) == 0) {
//             /* When at end-of-line, there is no other start. */
//             if (!line->data[index]) {
//               break;
//             }
//             index = step_right(line->data, index);
//           }
//           continue;
//         }
//         /* Look for an end match on later lines. */
//         tailline = line->next;
//         while (tailline && regexec(ink->end, tailline->data, 1, &endmatch, 0) != 0) {
//           tailline = tailline->next;
//         }
//         line->multidata[ink->id] = STARTSHERE;
//         /* Note that this also advances the line in the main loop. */
//         for (line = line->next; line != tailline; line = line->next) {
//           line->multidata[ink->id] = WHOLELINE;
//         }
//         if (!tailline) {
//           line = openfile->filebot;
//           break;
//         }
//         tailline->multidata[ink->id] = ENDSHERE;
//         /* Look for a possible new start after the end match. */
//         index = endmatch.rm_eo;
//       }
//     }
//   }
// }

bool str_equal_to_rgx(const char *str, const regex_t *rgx) {
  return (regexec(rgx, str, 0, NULL, 0) == 0);
}

short rgb_to_ncurses(Uchar value) {
  return short(value * 1000 / 255);
}

/* Encodes `fg` and `bg` with a rgb value for a given idx.  To use the rgb value encoded into `color_int` do:
 *   `r` (color_int & 0xff)
 *   `g` ((color_int >> 8) & 0xff)
 *   `b` ((color_int >> 16) & 0xff)
 * Note that both `fg` and `bg` will return `-1` when `idx` represents nothing. */
void attr_idx_int_code(int idx, int *fg, int *bg) {
  if (!fg || !bg || idx < 0 || idx >= NUMBER_OF_ELEMENTS) {
    *fg = -1;
    *bg = -1;
    return;
  }
  *fg = encoded_idx_color[idx][0];
  *bg = encoded_idx_color[idx][1];
}
