/// \file constexpr_def.h
#pragma once
#include <Mlib/constexpr.hpp>
#include <ncursesw/ncurses.h>

/* Identifiers for color options. */
constexpr unsigned char TITLE_BAR          = 0;
constexpr unsigned char LINE_NUMBER        = 1;
constexpr unsigned char GUIDE_STRIPE       = 2;
constexpr unsigned char SCROLL_BAR         = 3;
constexpr unsigned char SELECTED_TEXT      = 4;
constexpr unsigned char SPOTLIGHTED        = 5;
constexpr unsigned char MINI_INFOBAR       = 6;
constexpr unsigned char PROMPT_BAR         = 7;
constexpr unsigned char STATUS_BAR         = 8;
constexpr unsigned char ERROR_MESSAGE      = 9;
constexpr unsigned char KEY_COMBO          = 10;
constexpr unsigned char FUNCTION_TAG       = 11;
constexpr unsigned char NUMBER_OF_ELEMENTS = 12;
/* The color options map. */
CONSTEXPR_MAP<std::string_view, unsigned char, 12> colorOptionMap = {
    {{"titlecolor", TITLE_BAR},
     {"numbercolor", LINE_NUMBER},
     {"stripecolor", GUIDE_STRIPE},
     {"scrollercolor", SCROLL_BAR},
     {"selectedcolor", SELECTED_TEXT},
     {"spotlightcolor", SPOTLIGHTED},
     {"minicolor", MINI_INFOBAR},
     {"promptcolor", PROMPT_BAR},
     {"statuscolor", STATUS_BAR},
     {"errorcolor", ERROR_MESSAGE},
     {"keycolor", KEY_COMBO},
     {"functioncolor", FUNCTION_TAG}}
};

/* Identifiers for the different configuration options. */
constexpr unsigned short OPERATINGDIR     = (1 << 0);
constexpr unsigned short FILL             = (1 << 1);
constexpr unsigned short MATCHBRACKETS    = (1 << 2);
constexpr unsigned short WHITESPACE       = (1 << 3);
constexpr unsigned short PUNCT            = (1 << 4);
constexpr unsigned short BRACKETS         = (1 << 5);
constexpr unsigned short QUOTESTR         = (1 << 6);
constexpr unsigned short SPELLER          = (1 << 7);
constexpr unsigned short BACKUPDIR        = (1 << 8);
constexpr unsigned short WORDCHARS        = (1 << 9);
constexpr unsigned short GUIDESTRIPE      = (1 << 10);
constexpr unsigned short CONF_OPT_TABSIZE = (1 << 11);
/* The configuration options map. */
CONSTEXPR_MAP<std::string_view, unsigned short, 12> configOptionMap = {
    {{"operatingdir", OPERATINGDIR},
     {"fill", FILL},
     {"matchbrackets", MATCHBRACKETS},
     {"whitespace", WHITESPACE},
     {"punct", PUNCT},
     {"brackets", BRACKETS},
     {"quotestr", QUOTESTR},
     {"speller", SPELLER},
     {"backupdir", BACKUPDIR},
     {"wordchars", WORDCHARS},
     {"guidestripe", GUIDESTRIPE},
     {"tabsize", CONF_OPT_TABSIZE}}
};

/* Identifiers for the different syntax options. */
constexpr unsigned char SYNTAX_OPT_COLOR     = (1 << 0);
constexpr unsigned char SYNTAX_OPT_ICOLOR    = (1 << 1);
constexpr unsigned char SYNTAX_OPT_COMMENT   = (1 << 2);
constexpr unsigned char SYNTAX_OPT_TABGIVES  = (1 << 3);
constexpr unsigned char SYNTAX_OPT_LINTER    = (1 << 4);
constexpr unsigned char SYNTAX_OPT_FORMATTER = (1 << 5);
/* The syntax options map. */
constexpr_map<std::string_view, unsigned char, 6> syntaxOptionMap = {
    {{"color", SYNTAX_OPT_COLOR},
     {"icolor", SYNTAX_OPT_ICOLOR},
     {"comment", SYNTAX_OPT_COMMENT},
     {"tabgives", SYNTAX_OPT_TABGIVES},
     {"linter", SYNTAX_OPT_LINTER},
     {"formatter", SYNTAX_OPT_FORMATTER}}
};

//
//  Identifiers for the different flags.
//
//  TODO : ( flags ):
//       - Change the flags array into a 64 bit unsigned integer,
//       - as there is less then 64 flags, and this will save memory.
//       - Currently the flags are stored in a 32 bit unsigned integer array with size 4.
//       - ( unsigned flags[4] )
//

/* This flag is not used as this is part of a bitfield and 0 is not a unique value, in terms
 * of bitwise operations as the default all non set value is 0. */
constexpr unsigned char DONTUSE            = 0;
constexpr unsigned char CASE_SENSITIVE     = 1;
constexpr unsigned char CONSTANT_SHOW      = 2;
constexpr unsigned char NO_HELP            = 3;
constexpr unsigned char NO_WRAP            = 4;
constexpr unsigned char AUTOINDENT         = 5;
constexpr unsigned char VIEW_MODE          = 6;
constexpr unsigned char USE_MOUSE          = 7;
constexpr unsigned char USE_REGEXP         = 8;
constexpr unsigned char SAVE_ON_EXIT       = 9;
constexpr unsigned char CUT_FROM_CURSOR    = 10;
constexpr unsigned char BACKWARDS_SEARCH   = 11;
constexpr unsigned char MULTIBUFFER        = 12;
constexpr unsigned char REBIND_DELETE      = 13;
constexpr unsigned char RAW_SEQUENCES      = 14;
constexpr unsigned char NO_CONVERT         = 15;
constexpr unsigned char MAKE_BACKUP        = 16;
constexpr unsigned char INSECURE_BACKUP    = 17;
constexpr unsigned char NO_SYNTAX          = 18;
constexpr unsigned char PRESERVE           = 19;
constexpr unsigned char HISTORYLOG         = 20;
constexpr unsigned char RESTRICTED         = 21;
constexpr unsigned char SMART_HOME         = 22;
constexpr unsigned char WHITESPACE_DISPLAY = 23;
constexpr unsigned char TABS_TO_SPACES     = 24;
constexpr unsigned char QUICK_BLANK        = 25;
constexpr unsigned char WORD_BOUNDS        = 26;
constexpr unsigned char NO_NEWLINES        = 27;
constexpr unsigned char BOLD_TEXT          = 28;
constexpr unsigned char SOFTWRAP           = 29;
constexpr unsigned char POSITIONLOG        = 30;
constexpr unsigned char LOCKING            = 31;
constexpr unsigned char NOREAD_MODE        = 32;
constexpr unsigned char MAKE_IT_UNIX       = 33;
constexpr unsigned char TRIM_BLANKS        = 34;
constexpr unsigned char SHOW_CURSOR        = 35;
constexpr unsigned char LINE_NUMBERS       = 36;
constexpr unsigned char AT_BLANKS          = 37;
constexpr unsigned char AFTER_ENDS         = 38;
constexpr unsigned char LET_THEM_ZAP       = 39;
constexpr unsigned char BREAK_LONG_LINES   = 40;
constexpr unsigned char JUMPY_SCROLLING    = 41;
constexpr unsigned char EMPTY_LINE         = 42;
constexpr unsigned char INDICATOR          = 43;
constexpr unsigned char BOOKSTYLE          = 44;
constexpr unsigned char COLON_PARSING      = 45;
constexpr unsigned char STATEFLAGS         = 46;
constexpr unsigned char USE_MAGIC          = 47;
constexpr unsigned char MINIBAR            = 48;
constexpr unsigned char ZERO               = 49;
constexpr unsigned char MODERN_BINDINGS    = 50;
/* The flags map. */
constexpr_map<std::string_view, unsigned char, 94> flagOptionsMap = {
    {{"-A", SMART_HOME},
     {"--smarthome", SMART_HOME},
     {"-B", MAKE_BACKUP},
     {"--backup", MAKE_BACKUP},
     {"-C", INSECURE_BACKUP},
     {"--backupdir", INSECURE_BACKUP},
     {"-D", BOLD_TEXT},
     {"--boldtext", BOLD_TEXT},
     {"-E", TABS_TO_SPACES},
     {"--tabstospaces", TABS_TO_SPACES},
     {"-F", MULTIBUFFER},
     {"--multibuffer", MULTIBUFFER},
     {"-G", LOCKING},
     {"--locking", LOCKING},
     {"-H", HISTORYLOG},
     {"--historylog", HISTORYLOG},
     {"-J", NO_WRAP},
     {"--guidestripe", NO_WRAP},
     {"-K", RAW_SEQUENCES},
     {"--rawsequences", RAW_SEQUENCES},
     {"-L", NO_NEWLINES},
     {"--nonewlines", NO_NEWLINES},
     {"-M", TRIM_BLANKS},
     {"--trimblanks", TRIM_BLANKS},
     {"-N", NO_CONVERT},
     {"--noconvert", NO_CONVERT},
     {"-O", BOOKSTYLE},
     {"--bookstyle", BOOKSTYLE},
     {"-P", POSITIONLOG},
     {"--positionlog", POSITIONLOG},
     {"-Q", NO_SYNTAX},
     {"--quotestr", NO_SYNTAX},
     {"-R", RESTRICTED},
     {"--restricted", RESTRICTED},
     {"-S", SOFTWRAP},
     {"--softwrap", SOFTWRAP},
     {"-U", QUICK_BLANK},
     {"--quickblank", QUICK_BLANK},
     {"-W", WORD_BOUNDS},
     {"--wordbounds", WORD_BOUNDS},
     {"-Z", LET_THEM_ZAP},
     {"--zap", LET_THEM_ZAP},
     {"-a", AT_BLANKS},
     {"--atblanks", AT_BLANKS},
     {"-c", CONSTANT_SHOW},
     {"--constantshow", CONSTANT_SHOW},
     {"-d", REBIND_DELETE},
     {"--rebinddelete", REBIND_DELETE},
     {"-e", EMPTY_LINE},
     {"--emptyline", EMPTY_LINE},
     {"-g", SHOW_CURSOR},
     {"--showcursor", SHOW_CURSOR},
     {"-h", NO_HELP},
     {"--help", NO_HELP},
     {"-i", AUTOINDENT},
     {"--autoindent", AUTOINDENT},
     {"-j", JUMPY_SCROLLING},
     {"--jumpyscrolling", JUMPY_SCROLLING},
     {"-k", CUT_FROM_CURSOR},
     {"--cutfromcursor", CUT_FROM_CURSOR},
     {"-l", LINE_NUMBERS},
     {"--linenumbers", LINE_NUMBERS},
     {"-m", USE_MOUSE},
     {"--mouse", USE_MOUSE},
     {"-n", NOREAD_MODE},
     {"--noread", NOREAD_MODE},
     {"-p", PRESERVE},
     {"--preserve", PRESERVE},
     {"-q", INDICATOR},
     {"--indicator", INDICATOR},
     {"-t", SAVE_ON_EXIT},
     {"--saveonexit", SAVE_ON_EXIT},
     {"-u", MAKE_IT_UNIX},
     {"--unix", MAKE_IT_UNIX},
     {"-v", VIEW_MODE},
     {"--view", VIEW_MODE},
     {"-w", NO_WRAP},
     {"--nowrap", NO_WRAP},
     {"-x", NO_HELP},
     {"--nohelp", NO_HELP},
     {"-y", AFTER_ENDS},
     {"--afterends", AFTER_ENDS},
     {"/", MODERN_BINDINGS},
     {"--modernbindings", MODERN_BINDINGS},
     {"@", COLON_PARSING},
     {"--colonparsing", COLON_PARSING},
     {"%", STATEFLAGS},
     {"--stateflags", STATEFLAGS},
     {"_", MINIBAR},
     {"--minibar", MINIBAR},
     {"0", ZERO},
     {"--zero", ZERO},
     {"!", USE_MAGIC},
     {"--magic", USE_MAGIC}}
};

/* Identifiers for the different command line options. */
constexpr unsigned int CLI_OPT_IGNORERCFILE   = (1 << 0);
constexpr unsigned int CLI_OPT_VERSION        = (1 << 1);
constexpr unsigned int CLI_OPT_HELP           = (1 << 2);
constexpr unsigned int CLI_OPT_SYNTAX         = (1 << 3);
constexpr unsigned int CLI_OPT_RCFILE         = (1 << 4);
constexpr unsigned int CLI_OPT_GUIDESTRIPE    = (1 << 5);
constexpr unsigned int CLI_OPT_WORDCHARS      = (1 << 6);
constexpr unsigned int CLI_OPT_TABSIZE        = (1 << 7);
constexpr unsigned int CLI_OPT_OPERATINGDIR   = (1 << 8);
constexpr unsigned int CLI_OPT_FILL           = (1 << 9);
constexpr unsigned int CLI_OPT_SPELLER        = (1 << 10);
constexpr unsigned int CLI_OPT_LISTSYNTAX     = (1 << 11);
constexpr unsigned int CLI_OPT_BACKUPDIR      = (1 << 12);
constexpr unsigned int CLI_OPT_BREAKLONGLINES = (1 << 13);
/* The command line options map. */
constexpr_map<std::string_view, unsigned int, 25> cliOptionMap = {
    {{"-I", CLI_OPT_IGNORERCFILE},
     {"--ignorercfiles", CLI_OPT_IGNORERCFILE},
     {"-V", CLI_OPT_VERSION},
     {"--version", CLI_OPT_VERSION},
     {"-h", CLI_OPT_HELP},
     {"--help", CLI_OPT_HELP},
     {"-Y", CLI_OPT_SYNTAX},
     {"--syntax", CLI_OPT_SYNTAX},
     {"-X", CLI_OPT_WORDCHARS},
     {"--wordchars", CLI_OPT_WORDCHARS},
     {"-f", CLI_OPT_RCFILE},
     {"--rcfile", CLI_OPT_RCFILE},
     {"-T", CLI_OPT_TABSIZE},
     {"--tabsize", CLI_OPT_TABSIZE},
     {"--rcfile", CLI_OPT_RCFILE},
     {"-o", CLI_OPT_OPERATINGDIR},
     {"--operatingdir", CLI_OPT_OPERATINGDIR},
     {"-r", CLI_OPT_FILL},
     {"--fill", CLI_OPT_FILL},
     {"-s", CLI_OPT_SPELLER},
     {"--speller", CLI_OPT_SPELLER},
     {"-z", CLI_OPT_LISTSYNTAX},
     {"--listsyntaxes", CLI_OPT_LISTSYNTAX},
     {"-b", CLI_OPT_BREAKLONGLINES},
     {"--breaklonglines", CLI_OPT_BREAKLONGLINES}}
};

constexpr unsigned char     NUMBER_OF_FLAGS          = 51;
constexpr const char *const DEFAULT_RESPONSE_ON_NONE = "Ehm...";
//
//  This is a masterpiece of a map.
//  I have succesfully made the way to get the description of a flag,
//  instant and without any overhead,
//  while using less memory then a unordered map.
//  .
//  USAGE:
//  - ( &epithetOfFlagMap[flag].value[0] ) will return the underlying ptr to the description of the flag
//
//  The ( flag / description ) map.
//
CONSTEXPR_MAP<u32, STRING_VIEW, NUMBER_OF_FLAGS> epithetOfFlagMap = {
    {{DONTUSE, DEFAULT_RESPONSE_ON_NONE},
     {CASE_SENSITIVE, DEFAULT_RESPONSE_ON_NONE},
     {CONSTANT_SHOW, "Constant cursor position display"},
     {NO_HELP, "Help mode"},
     {NO_WRAP, DEFAULT_RESPONSE_ON_NONE},
     {AUTOINDENT, "Auto indent"},
     {VIEW_MODE, DEFAULT_RESPONSE_ON_NONE},
     {USE_MOUSE, "Mouse support"},
     {USE_REGEXP, DEFAULT_RESPONSE_ON_NONE},
     {SAVE_ON_EXIT, DEFAULT_RESPONSE_ON_NONE},
     {CUT_FROM_CURSOR, "Cut to end"},
     {BACKWARDS_SEARCH, DEFAULT_RESPONSE_ON_NONE},
     {MULTIBUFFER, DEFAULT_RESPONSE_ON_NONE},
     {REBIND_DELETE, DEFAULT_RESPONSE_ON_NONE},
     {RAW_SEQUENCES, DEFAULT_RESPONSE_ON_NONE},
     {NO_CONVERT, DEFAULT_RESPONSE_ON_NONE},
     {MAKE_BACKUP, DEFAULT_RESPONSE_ON_NONE},
     {INSECURE_BACKUP, DEFAULT_RESPONSE_ON_NONE},
     {NO_SYNTAX, "Color syntax highlighting"},
     {PRESERVE, DEFAULT_RESPONSE_ON_NONE},
     {HISTORYLOG, DEFAULT_RESPONSE_ON_NONE},
     {RESTRICTED, DEFAULT_RESPONSE_ON_NONE},
     {SMART_HOME, "Smart home key"},
     {WHITESPACE_DISPLAY, "Whitespace display"},
     {TABS_TO_SPACES, "Conversion of typed tabs to spaces"},
     {QUICK_BLANK, DEFAULT_RESPONSE_ON_NONE},
     {WORD_BOUNDS, DEFAULT_RESPONSE_ON_NONE},
     {NO_NEWLINES, DEFAULT_RESPONSE_ON_NONE},
     {BOLD_TEXT, DEFAULT_RESPONSE_ON_NONE},
     {SOFTWRAP, "Soft wrapping of overlong lines"},
     {POSITIONLOG, DEFAULT_RESPONSE_ON_NONE},
     {LOCKING, DEFAULT_RESPONSE_ON_NONE},
     {NOREAD_MODE, DEFAULT_RESPONSE_ON_NONE},
     {MAKE_IT_UNIX, DEFAULT_RESPONSE_ON_NONE},
     {TRIM_BLANKS, DEFAULT_RESPONSE_ON_NONE},
     {SHOW_CURSOR, DEFAULT_RESPONSE_ON_NONE},
     {LINE_NUMBERS, "Line numbering"},
     {AT_BLANKS, DEFAULT_RESPONSE_ON_NONE},
     {AFTER_ENDS, DEFAULT_RESPONSE_ON_NONE},
     {LET_THEM_ZAP, DEFAULT_RESPONSE_ON_NONE},
     {BREAK_LONG_LINES, "Hard wrapping of overlong lines"},
     {JUMPY_SCROLLING, DEFAULT_RESPONSE_ON_NONE},
     {EMPTY_LINE, DEFAULT_RESPONSE_ON_NONE},
     {INDICATOR, DEFAULT_RESPONSE_ON_NONE},
     {BOOKSTYLE, DEFAULT_RESPONSE_ON_NONE},
     {COLON_PARSING, DEFAULT_RESPONSE_ON_NONE},
     {STATEFLAGS, DEFAULT_RESPONSE_ON_NONE},
     {USE_MAGIC, DEFAULT_RESPONSE_ON_NONE},
     {MINIBAR, DEFAULT_RESPONSE_ON_NONE},
     {ZERO, "Hidden interface"},
     {MODERN_BINDINGS, DEFAULT_RESPONSE_ON_NONE}}
};

//  Legazy code, from nano source code.
/* Identifiers for the different menus. */
constexpr unsigned short MMAIN        = (1 << 0);
constexpr unsigned short MWHEREIS     = (1 << 1);
constexpr unsigned short MREPLACE     = (1 << 2);
constexpr unsigned short MREPLACEWITH = (1 << 3);
constexpr unsigned short MGOTOLINE    = (1 << 4);
constexpr unsigned short MWRITEFILE   = (1 << 5);
constexpr unsigned short MINSERTFILE  = (1 << 6);
constexpr unsigned short MEXECUTE     = (1 << 7);
constexpr unsigned short MHELP        = (1 << 8);
constexpr unsigned short MSPELL       = (1 << 9);
constexpr unsigned short MBROWSER     = (1 << 10);
constexpr unsigned short MWHEREISFILE = (1 << 11);
constexpr unsigned short MGOTODIR     = (1 << 12);
constexpr unsigned short MYESNO       = (1 << 13);
constexpr unsigned short MLINTER      = (1 << 14);
constexpr unsigned short MFINDINHELP  = (1 << 15);
constexpr unsigned short MMOST = (MMAIN | MWHEREIS | MREPLACE | MREPLACEWITH | MGOTOLINE | MWRITEFILE | MINSERTFILE |
                                  MEXECUTE | MWHEREISFILE | MGOTODIR | MFINDINHELP | MSPELL | MLINTER);
constexpr unsigned short MSOME = MMOST | MBROWSER;
/* The menus map. */
constexpr_map<std::string_view, unsigned short, 16> menuOptionMap = {
    {{"main", MMAIN},
     {"search", MWHEREIS},
     {"replace", MREPLACE},
     {"replacewith", MREPLACEWITH},
     {"yesno", MYESNO},
     {"gotoline", MGOTOLINE},
     {"writeout", MWRITEFILE},
     {"insert", MINSERTFILE},
     {"execute", MEXECUTE},
     {"help", MHELP},
     {"spell", MSPELL},
     {"linter", MLINTER},
     {"browser", MBROWSER},
     {"whereisfile", MWHEREISFILE},
     {"gotodir", MGOTODIR},
     {"all", MMOST | MBROWSER | MHELP | MYESNO}}
};

constexpr_map<STRING_VIEW, u32, 14> toggleOptionMap = {
    {{"nohelp", NO_HELP},
     {"zero", ZERO},
     {"constantshow", CONSTANT_SHOW},
     {"replacewith", MREPLACEWITH},
     {"softwrap", SOFTWRAP},
     {"linenumbers", LINE_NUMBERS},
     {"whitespacedisplay", WHITESPACE_DISPLAY},
     {"nosyntax", NO_SYNTAX},
     {"smarthome", SMART_HOME},
     {"autoindent", AUTOINDENT},
     {"cutfromcursor", CUT_FROM_CURSOR},
     {"breaklonglines", BREAK_LONG_LINES},
     {"tabstospaces", TABS_TO_SPACES},
     {"mouse", USE_MOUSE}}
};

#define CS_STRUCT  (1 << 0)
#define CS_ENUM    (1 << 1)
#define CS_INT     (1 << 2)
#define CS_VOID    (1 << 3)
#define CS_LONG    (1 << 4)
#define CS_CHAR    (1 << 5)
#define CS_INCLUDE (1 << 6)
#define CS_DEFINE  (1 << 7)
#define CS_CLASS   (1 << 8)
constexpr_map<std::string_view, unsigned short, 9> c_syntax_map = {
    {{"struct", CS_STRUCT},
     {"enum", CS_ENUM},
     {"int", CS_INT},
     {"void", CS_VOID},
     {"long", CS_LONG},
     {"char", CS_CHAR},
     {"#include", CS_INCLUDE},
     {"#define", CS_DEFINE},
     {"class", CS_CLASS}}
};

constexpr const char *ERROR_MSG_OUT_OF_MEMORY         = "NanoX is out of memory!\n";
constexpr const char *ERROR_MSG_TO_MENY_DOT_SAVEFILES = "\nToo many .save files\n";
constexpr const char *TAB_STR                         = "\t";
