/** @file arg.c

  @author  Melwin Svensson.
  @date    9-7-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Used at init or when we need to parse a flag from strings. */
static HashMap *flagmap = NULL;
/* Used at init to parse cli options with or without arguments. */
static HashMap *clioptmap = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- flagmap ----------------------------- */

/* Add one entry to the hashmap.  Where str is the key and flag will be assigned to an allocated `flag_type`. */
static void flagmap_entry_add(const char *const restrict str, flag_type flag) {
  ASSERT(flagmap);
  flag_type *f = xmalloc(sizeof(*f));
  *f = flag;
  hashmap_insert(flagmap, str, f);
}

/* Create the flagmap, and add all flags to the hashmap.  TODO: Add --gui, --safe and --test to the flagmap. */
static void flagmap_create(void) {
  ASSERT(!flagmap);
  flagmap = hashmap_create_wfreefunc(free);
  flagmap_entry_add(              "-A", SMART_HOME     );
  flagmap_entry_add(     "--smarthome", SMART_HOME     );
  flagmap_entry_add(              "-B", MAKE_BACKUP    );
  flagmap_entry_add(        "--backup", MAKE_BACKUP    );
  flagmap_entry_add(              "-C", INSECURE_BACKUP);
  flagmap_entry_add(     "--backupdir", INSECURE_BACKUP);
  flagmap_entry_add(              "-D", BOLD_TEXT      );
  flagmap_entry_add(      "--boldtext", BOLD_TEXT      );
  flagmap_entry_add(              "-E", TABS_TO_SPACES );
  flagmap_entry_add(  "--tabstospaces", TABS_TO_SPACES );
  flagmap_entry_add(              "-F", MULTIBUFFER    );
  flagmap_entry_add(   "--multibuffer", MULTIBUFFER    );
  flagmap_entry_add(              "-G", LOCKING        );
  flagmap_entry_add(       "--locking", LOCKING        );
  flagmap_entry_add(              "-H", HISTORYLOG     );
  flagmap_entry_add(    "--historylog", HISTORYLOG     );
  flagmap_entry_add(              "-J", NO_WRAP        );
  flagmap_entry_add(   "--guidestripe", NO_WRAP        );
  flagmap_entry_add(              "-K", RAW_SEQUENCES  );
  flagmap_entry_add(  "--rawsequences", RAW_SEQUENCES  );
  flagmap_entry_add(              "-L", NO_NEWLINES    );
  flagmap_entry_add(    "--nonewlines", NO_NEWLINES    );
  flagmap_entry_add(              "-M", TRIM_BLANKS    );
  flagmap_entry_add(    "--trimblanks", TRIM_BLANKS    );
  flagmap_entry_add(              "-N", NO_CONVERT     );
  flagmap_entry_add(     "--noconvert", NO_CONVERT     );
  flagmap_entry_add(              "-O", BOOKSTYLE      );
  flagmap_entry_add(     "--bookstyle", BOOKSTYLE      );
  flagmap_entry_add(              "-P", POSITIONLOG    );
  flagmap_entry_add(   "--positionlog", POSITIONLOG    );
  flagmap_entry_add(              "-Q", NO_SYNTAX      );
  flagmap_entry_add(      "--quotestr", NO_SYNTAX      );
  flagmap_entry_add(              "-R", RESTRICTED     );
  flagmap_entry_add(    "--restricted", RESTRICTED     );
  flagmap_entry_add(              "-S", SOFTWRAP       );
  flagmap_entry_add(      "--softwrap", SOFTWRAP       );
  flagmap_entry_add(              "-U", QUICK_BLANK    );
  flagmap_entry_add(    "--quickblank", QUICK_BLANK    );
  flagmap_entry_add(              "-W", WORD_BOUNDS    );
  flagmap_entry_add(    "--wordbounds", WORD_BOUNDS    );
  flagmap_entry_add(              "-Z", LET_THEM_ZAP   );
  flagmap_entry_add(           "--zap", LET_THEM_ZAP   );
  flagmap_entry_add(              "-a", AT_BLANKS      );
  flagmap_entry_add(      "--atblanks", AT_BLANKS      );
  flagmap_entry_add(              "-c", CONSTANT_SHOW  );
  flagmap_entry_add(  "--constantshow", CONSTANT_SHOW  );
  flagmap_entry_add(              "-d", REBIND_DELETE  );
  flagmap_entry_add(  "--rebinddelete", REBIND_DELETE  );
  flagmap_entry_add(              "-e", EMPTY_LINE     );
  flagmap_entry_add(     "--emptyline", EMPTY_LINE     );
  flagmap_entry_add(              "-g", SHOW_CURSOR    );
  flagmap_entry_add(    "--showcursor", SHOW_CURSOR    );
  flagmap_entry_add(              "-i", AUTOINDENT     );
  flagmap_entry_add(    "--autoindent", AUTOINDENT     );
  flagmap_entry_add(              "-j", JUMPY_SCROLLING);
  flagmap_entry_add("--jumpyscrolling", JUMPY_SCROLLING);
  flagmap_entry_add(              "-k", CUT_FROM_CURSOR);
  flagmap_entry_add( "--cutfromcursor", CUT_FROM_CURSOR);
  flagmap_entry_add(              "-l", LINE_NUMBERS   );
  flagmap_entry_add(   "--linenumbers", LINE_NUMBERS   );
  flagmap_entry_add(              "-m", USE_MOUSE      );
  flagmap_entry_add(         "--mouse", USE_MOUSE      );
  flagmap_entry_add(              "-n", NOREAD_MODE    );
  flagmap_entry_add(        "--noread", NOREAD_MODE    );
  flagmap_entry_add(              "-p", PRESERVE       );
  flagmap_entry_add(      "--preserve", PRESERVE       );
  flagmap_entry_add(              "-q", INDICATOR      );
  flagmap_entry_add(     "--indicator", INDICATOR      );
  flagmap_entry_add(              "-t", SAVE_ON_EXIT   );
  flagmap_entry_add(    "--saveonexit", SAVE_ON_EXIT   );
  flagmap_entry_add(              "-u", MAKE_IT_UNIX   );
  flagmap_entry_add(          "--unix", MAKE_IT_UNIX   );
  flagmap_entry_add(              "-v", VIEW_MODE      );
  flagmap_entry_add(          "--view", VIEW_MODE      );
  flagmap_entry_add(              "-w", NO_WRAP        );
  flagmap_entry_add(        "--nowrap", NO_WRAP        );
  flagmap_entry_add(              "-x", NO_HELP        );
  flagmap_entry_add(        "--nohelp", NO_HELP        );
  flagmap_entry_add(              "-y", AFTER_ENDS     );
  flagmap_entry_add(     "--afterends", AFTER_ENDS     );
  flagmap_entry_add(              "-/", MODERN_BINDINGS);
  flagmap_entry_add("--modernbindings", MODERN_BINDINGS);
  flagmap_entry_add(              "-@", COLON_PARSING  );
  flagmap_entry_add(  "--colonparsing", COLON_PARSING  );
  flagmap_entry_add(              "-%", STATEFLAGS     );
  flagmap_entry_add(    "--stateflags", STATEFLAGS     );
  flagmap_entry_add(              "-_", MINIBAR        );
  flagmap_entry_add(       "--minibar", MINIBAR        );
  flagmap_entry_add(              "-0", ZERO           );
  flagmap_entry_add(          "--zero", ZERO           );
  flagmap_entry_add(              "-!", USE_MAGIC      );
  flagmap_entry_add(         "--magic", USE_MAGIC      );
}

/* Frees the flagmap once we are done using it. */
static void flagmap_free(void) {
  ASSERT(flagmap);
  hashmap_free(flagmap);
  flagmap = NULL;
}

/* ----------------------------- clioptmap ----------------------------- */

/* Add one entry to the `command line option map`.  Where `str` is the key and `flag` will be assigned to an allocated `cliopt_type`.  */
static void clioptmap_entry_add(const char *const restrict str, cliopt_type flag) {
  ASSERT(clioptmap);
  cliopt_type *c = xmalloc(sizeof(*c));
  *c = flag;
  hashmap_insert(clioptmap, str, c);
}

/* Create the `command line option map` */
static void clioptmap_create(void) {
  ASSERT(!clioptmap);
  /* Create the map, setting free() as the freeing function as we simply allocate one var. */
  clioptmap = hashmap_create_wfreefunc(free);
  /* Then add all entries. */
  clioptmap_entry_add(              "-I", CLIOPT_IGNORERCFILE  );
  clioptmap_entry_add( "--ignorercfiles", CLIOPT_IGNORERCFILE  );
  clioptmap_entry_add(              "-V", CLIOPT_VERSION       );
  clioptmap_entry_add(       "--version", CLIOPT_VERSION       );
  clioptmap_entry_add(              "-h", CLIOPT_HELP          );
  clioptmap_entry_add(          "--help", CLIOPT_HELP          );
  clioptmap_entry_add(              "-Y", CLIOPT_SYNTAX        );
  clioptmap_entry_add(        "--syntax", CLIOPT_SYNTAX        );
  clioptmap_entry_add(              "-X", CLIOPT_WORDCHARS     );
  clioptmap_entry_add(     "--wordchars", CLIOPT_WORDCHARS     );
  clioptmap_entry_add(              "-f", CLIOPT_RCFILE        );
  clioptmap_entry_add(        "--rcfile", CLIOPT_RCFILE        );
  clioptmap_entry_add(              "-T", CLIOPT_TABSIZE       );
  clioptmap_entry_add(       "--tabsize", CLIOPT_TABSIZE       );
  clioptmap_entry_add(              "-o", CLIOPT_OPERATINGDIR  );
  clioptmap_entry_add(  "--operatingdir", CLIOPT_OPERATINGDIR  );
  clioptmap_entry_add(              "-r", CLIOPT_FILL          );
  clioptmap_entry_add(          "--fill", CLIOPT_FILL          );
  clioptmap_entry_add(              "-s", CLIOPT_SPELLER       );
  clioptmap_entry_add(       "--speller", CLIOPT_SPELLER       );
  clioptmap_entry_add(              "-z", CLIOPT_LISTSYNTAX    );
  clioptmap_entry_add(  "--listsyntaxes", CLIOPT_LISTSYNTAX    );
  clioptmap_entry_add(              "-b", CLIOPT_BREAKLONGLINES);
  clioptmap_entry_add("--breaklonglines", CLIOPT_BREAKLONGLINES);
  clioptmap_entry_add(           "--gui", CLIOPT_GUI           );
  clioptmap_entry_add(          "--safe", CLIOPT_SAFE          );
  clioptmap_entry_add(          "--test", CLIOPT_TEST          );
}

/* Free the `command line option map`. */
static void clioptmap_free(void) {
  ASSERT(clioptmap);
  hashmap_free(clioptmap);
  clioptmap = NULL;
}

/* ----------------------------- Consume argument ----------------------------- */

/* Consume an argument from `argv` at `i`. */
static void consume_argument(int *const argc, char **argv, int i) {
  ASSERT(argc);
  ASSERT(argv);
  /* Consume the argument. */
  memmove((argv + i), (argv + i + 1), (((*argc) - i - 1) * _PTRSIZE));
  /* Decrament the total size. */
  --(*argc);
}

/* ----------------------------- Not a flag ----------------------------- */

/* Returns `TRUE` when `value` is not a flag.  Otherwise, this will print an error and terminate. */
static bool not_a_flag(const char *const restrict value, const char *const restrict requesting_flag) {
  ASSERT(flagmap);
  ASSERT(clioptmap);
  ASSERT(value);
  ASSERT(requesting_flag);
  if (hashmap_get(flagmap, value) || hashmap_get(clioptmap, value)) {
    fprintf(stderr, _("The argument `%s` needs a value\n"), requesting_flag);
    exit(1);
  }
  else {
    return TRUE;
  }
}

/* ----------------------------- Has next argument ----------------------------- */

/* Returns `TRUE` when the argument at index `i` is not the last
 * argument.  Otherwise, this will print a message and terminate. */
static bool has_next_argument(int i, int argc, const char *const restrict requesting_flag) {
  ASSERT(requesting_flag);
  if ((i + 1) >= argc) {
    fprintf(stderr, _("The argument `%s` needs a value\n"), requesting_flag);
    exit(1);
  }
  else {
    return TRUE;
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Proccess cli arguments ----------------------------- */

/* Process all command line arguments, and consume and correct check all parsed options. */
void proccess_cli_arguments(int *const argc, char **argv) {
  ASSERT(argc);
  ASSERT(argv);
  /* A ptr containing the flag value of a successfully parsed flag opt. */
  flag_type *fptr;
  /* A ptr containing the flag value of a successfully parsed flag opt. */
  cliopt_type *cptr;
  if (*argc > 1) {
    flagmap_create();
    clioptmap_create();
    for (int i=1; i<(*argc); ++i) {
      fptr = hashmap_get(flagmap, argv[i]);
      /* If the passed argument has some flag value. */
      if (fptr) {
        /* Set tha value its pointing to. */
        SET(*fptr);
        consume_argument(argc, argv, i--);
      }
      /* Otherwise, check for cli options with or without arguments. */
      else {
        cptr = hashmap_get(clioptmap, argv[i]);
        if (cptr) {
          switch (*cptr) {
            case CLIOPT_IGNORERCFILE: {
              ignore_rcfiles = TRUE;
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_VERSION: {
              version();
            }
            case CLIOPT_HELP: {
              usage();
            }
            case CLIOPT_SYNTAX: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                syntaxstr = xstrcpy(syntaxstr, argv[i + 1]);
                consume_argument(argc, argv, i);
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_WORDCHARS: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                word_chars = xstrcpy(word_chars, argv[i + 1]);
                consume_argument(argc, argv, i);
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_RCFILE: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                custom_nanorc = xstrcpy(custom_nanorc, argv[i + 1]);
                consume_argument(argc, argv, i);
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_BREAKLONGLINES: {
              hardwrap = 1;
              break;
            }
            case CLIOPT_SPELLER: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                alt_speller = xstrcpy(alt_speller, argv[i + 1]);
                consume_argument(argc, argv, i);
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_OPERATINGDIR: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                operating_dir = xstrcpy(operating_dir, argv[i + 1]);
                consume_argument(argc, argv, i);
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_TABSIZE: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                if (!parse_num(argv[i + 1], &tabsize) || tabsize <= 0) {
                  fprintf(stderr, _("Requested tab size \"%s\" is invalid\n"), argv[i + 1]);
                  exit(1);
                }
                else {
                  consume_argument(argc, argv, i);
                }
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_FILL: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                if (!parse_num(argv[i + 1], &fill) || fill <= 0) {
                  fprintf(stderr, _("Requested fill size \"%s\" is invalid\n"), argv[i + 1]);
                  exit(1);
                }
                else {
                  fill_used = TRUE;
                  consume_argument(argc, argv, i);
                }
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_LISTSYNTAX: {
              /* No-op for now. */
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_BACKUPDIR: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                backup_dir = xstrcpy(backup_dir, argv[i + 1]);
                consume_argument(argc, argv, i);
              }
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_GUIDESTRIPE: {
              if (has_next_argument(i, *argc, argv[i]) && not_a_flag(argv[i + 1], argv[i])) {
                if (!parse_num(argv[i + 1], &stripe_column) || stripe_column <= 0) {
                  fprintf(stderr, _("Guide column \"%s\" is invalid\n"), argv[i + 1]);
                  exit(1);
                }
                else {
                  consume_argument(argc, argv, i);
                }
              }
              break;
            }
            case CLIOPT_GUI: {
              SET(USING_GUI);
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_SAFE: {
              UNSET(EXPERIMENTAL_FAST_LIVE_SYNTAX);
              consume_argument(argc, argv, i--);
              break;
            }
            case CLIOPT_TEST: {
              SET(NO_NCURSES);
              consume_argument(argc, argv, i--);
              break;
            }
          }
        }
      }
    }
    flagmap_free();
    clioptmap_free();
  }
}
