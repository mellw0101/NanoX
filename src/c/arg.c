/** @file arg.c

  @author  Melwin Svensson.
  @date    9-7-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* Used at init or when we need to parse a flag from strings. */
static HashMap *flagmap = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Add one entry to the hashmap.  Where str is the key and flag will be assigned to an allocated `flag_type`. */
static void flagmap_entry_add(const char *const restrict str, flag_type flag) {
  ASSERT(flagmap);
  flag_type *f = xmalloc(sizeof(*f));
  *f = flag;
  hashmap_insert(flagmap, str, f);
}

/* Create the flagmap, and add all flags to the hashmap. */
static void flagmap_create(void) {
  ASSERT(!flagmap);
  flagmap = hashmap_create_wfreefunc(free);
  flagmap_entry_add(               "-A", SMART_HOME      );
  flagmap_entry_add(      "--smarthome", SMART_HOME      );
  flagmap_entry_add(               "-B", MAKE_BACKUP     );
  flagmap_entry_add(         "--backup", MAKE_BACKUP     );
  flagmap_entry_add(               "-C", INSECURE_BACKUP );
  flagmap_entry_add(      "--backupdir", INSECURE_BACKUP );
  flagmap_entry_add(               "-D", BOLD_TEXT       );
  flagmap_entry_add(       "--boldtext", BOLD_TEXT       );
  flagmap_entry_add(               "-E", TABS_TO_SPACES  );
  flagmap_entry_add(   "--tabstospaces", TABS_TO_SPACES  );
  flagmap_entry_add(               "-F", MULTIBUFFER     );
  flagmap_entry_add(    "--multibuffer", MULTIBUFFER     );
  flagmap_entry_add(               "-G", LOCKING         );
  flagmap_entry_add(        "--locking", LOCKING         );
  flagmap_entry_add(               "-H", HISTORYLOG      );
  flagmap_entry_add(     "--historylog", HISTORYLOG      );
  flagmap_entry_add(               "-J", NO_WRAP         );
  flagmap_entry_add(    "--guidestripe", NO_WRAP         );
  flagmap_entry_add(               "-K", RAW_SEQUENCES   );
  flagmap_entry_add(   "--rawsequences", RAW_SEQUENCES   );
  flagmap_entry_add(               "-L", NO_NEWLINES     );
  flagmap_entry_add(     "--nonewlines", NO_NEWLINES     );
  flagmap_entry_add(               "-M", TRIM_BLANKS     );
  flagmap_entry_add(     "--trimblanks", TRIM_BLANKS     );
  flagmap_entry_add(               "-N", NO_CONVERT      );
  flagmap_entry_add(      "--noconvert", NO_CONVERT      );
  flagmap_entry_add(               "-O", BOOKSTYLE       );
  flagmap_entry_add(      "--bookstyle", BOOKSTYLE       );
  flagmap_entry_add(               "-P", POSITIONLOG     );
  flagmap_entry_add(    "--positionlog", POSITIONLOG     );
  flagmap_entry_add(               "-Q", NO_SYNTAX       );
  flagmap_entry_add(       "--quotestr", NO_SYNTAX       );
  flagmap_entry_add(               "-R", RESTRICTED      );
  flagmap_entry_add(     "--restricted", RESTRICTED      );
  flagmap_entry_add(               "-S", SOFTWRAP        );
  flagmap_entry_add(       "--softwrap", SOFTWRAP        );
  flagmap_entry_add(               "-U", QUICK_BLANK     );
  flagmap_entry_add(     "--quickblank", QUICK_BLANK     );
  flagmap_entry_add(               "-W", WORD_BOUNDS     );
  flagmap_entry_add(     "--wordbounds", WORD_BOUNDS     );
  flagmap_entry_add(               "-Z", LET_THEM_ZAP    );
  flagmap_entry_add(            "--zap", LET_THEM_ZAP    );
  flagmap_entry_add(               "-a", AT_BLANKS       );
  flagmap_entry_add(       "--atblanks", AT_BLANKS       );
  flagmap_entry_add(               "-c", CONSTANT_SHOW   );
  flagmap_entry_add(   "--constantshow", CONSTANT_SHOW   );
  flagmap_entry_add(               "-d", REBIND_DELETE   );
  flagmap_entry_add(   "--rebinddelete", REBIND_DELETE   );
  flagmap_entry_add(               "-e", EMPTY_LINE      );
  flagmap_entry_add(      "--emptyline", EMPTY_LINE      );
  flagmap_entry_add(               "-g", SHOW_CURSOR     );
  flagmap_entry_add(     "--showcursor", SHOW_CURSOR     );
  flagmap_entry_add(               "-h", NO_HELP         );
  flagmap_entry_add(           "--help", NO_HELP         );
  flagmap_entry_add(               "-i", AUTOINDENT      );
  flagmap_entry_add(     "--autoindent", AUTOINDENT      );
  flagmap_entry_add(               "-j", JUMPY_SCROLLING );
  flagmap_entry_add( "--jumpyscrolling", JUMPY_SCROLLING );
  flagmap_entry_add(               "-k", CUT_FROM_CURSOR );
  flagmap_entry_add(  "--cutfromcursor", CUT_FROM_CURSOR );
  flagmap_entry_add(               "-l", LINE_NUMBERS    );
  flagmap_entry_add(    "--linenumbers", LINE_NUMBERS    );
  flagmap_entry_add(               "-m", USE_MOUSE       );
  flagmap_entry_add(          "--mouse", USE_MOUSE       );
  flagmap_entry_add(               "-n", NOREAD_MODE     );
  flagmap_entry_add(         "--noread", NOREAD_MODE     );
  flagmap_entry_add(               "-p", PRESERVE        );
  flagmap_entry_add(       "--preserve", PRESERVE        );
  flagmap_entry_add(               "-q", INDICATOR       );
  flagmap_entry_add(      "--indicator", INDICATOR       );
  flagmap_entry_add(               "-t", SAVE_ON_EXIT    );
  flagmap_entry_add(     "--saveonexit", SAVE_ON_EXIT    );
  flagmap_entry_add(               "-u", MAKE_IT_UNIX    );
  flagmap_entry_add(           "--unix", MAKE_IT_UNIX    );
  flagmap_entry_add(               "-v", VIEW_MODE       );
  flagmap_entry_add(           "--view", VIEW_MODE       );
  flagmap_entry_add(               "-w", NO_WRAP         );
  flagmap_entry_add(         "--nowrap", NO_WRAP         );
  flagmap_entry_add(               "-x", NO_HELP         );
  flagmap_entry_add(         "--nohelp", NO_HELP         );
  flagmap_entry_add(               "-y", AFTER_ENDS      );
  flagmap_entry_add(      "--afterends", AFTER_ENDS      );
  flagmap_entry_add(                "/", MODERN_BINDINGS );
  flagmap_entry_add( "--modernbindings", MODERN_BINDINGS );
  flagmap_entry_add(                "@", COLON_PARSING   );
  flagmap_entry_add(   "--colonparsing", COLON_PARSING   );
  flagmap_entry_add(                "%", STATEFLAGS      );
  flagmap_entry_add(     "--stateflags", STATEFLAGS      );
  flagmap_entry_add(                "_", MINIBAR         );
  flagmap_entry_add(        "--minibar", MINIBAR         );
  flagmap_entry_add(                "0", ZERO            );
  flagmap_entry_add(           "--zero", ZERO            );
  flagmap_entry_add(                "!", USE_MAGIC       );
  flagmap_entry_add(          "--magic", USE_MAGIC       );
}

/* Frees the flagmap once we are done using it. */
static void flagmap_free(void) {
  ASSERT(flagmap);
  hashmap_free(flagmap);
  flagmap = NULL;
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/*  */
void arguments_proccess_flags(int argc, char **argv) {
  
}
