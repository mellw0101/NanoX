/// \file constexpr_def.h
#pragma once
#include <Mlib/constexpr.hpp>
#include <ncursesw/ncurses.h>

/* Identifiers for color options. */
#define TITLE_BAR          0
#define LINE_NUMBER        1
#define GUIDE_STRIPE       2
#define SCROLL_BAR         3
#define SELECTED_TEXT      4
#define SPOTLIGHTED        5
#define MINI_INFOBAR       6
#define PROMPT_BAR         7
#define STATUS_BAR         8
#define ERROR_MESSAGE      9
#define KEY_COMBO          10
#define FUNCTION_TAG       11
#define FG_BLUE            12
#define FG_GREEN           13
#define FG_MAGENTA         14
#define FG_LAGOON          15
#define FG_YELLOW          16
#define FG_RED             17
#define FG_PINK            18
#define NUMBER_OF_ELEMENTS 19
/* The color options map. */
constexpr_map<std::string_view, unsigned char, 12> colorOptionMap = {
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
#define OPERATINGDIR     (1 << 0)
#define FILL             (1 << 1)
#define MATCHBRACKETS    (1 << 2)
#define WHITESPACE       (1 << 3)
#define PUNCT            (1 << 4)
#define BRACKETS         (1 << 5)
#define QUOTESTR         (1 << 6)
#define SPELLER          (1 << 7)
#define BACKUPDIR        (1 << 8)
#define WORDCHARS        (1 << 9)
#define GUIDESTRIPE      (1 << 10)
#define CONF_OPT_TABSIZE (1 << 11)
/* The configuration options map. */
constexpr_map<std::string_view, unsigned short, 12> configOptionMap = {
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
#define SYNTAX_OPT_COLOR     (1 << 0)
#define SYNTAX_OPT_ICOLOR    (1 << 1)
#define SYNTAX_OPT_COMMENT   (1 << 2)
#define SYNTAX_OPT_TABGIVES  (1 << 3)
#define SYNTAX_OPT_LINTER    (1 << 4)
#define SYNTAX_OPT_FORMATTER (1 << 5)
/* The syntax options map. */
constexpr_map<std::string_view, unsigned char, 6> syntaxOptionMap = {
    {{"color", SYNTAX_OPT_COLOR},
     {"icolor", SYNTAX_OPT_ICOLOR},
     {"comment", SYNTAX_OPT_COMMENT},
     {"tabgives", SYNTAX_OPT_TABGIVES},
     {"linter", SYNTAX_OPT_LINTER},
     {"formatter", SYNTAX_OPT_FORMATTER}}
};

/* Identifiers for the different flags. */

/* This flag is not used as this is part of a bitfield and 0 is not a unique value, in terms
 * of bitwise operations as the default all non set value is 0. */
#define DONTUSE                       0
#define CASE_SENSITIVE                1
#define CONSTANT_SHOW                 2
#define NO_HELP                       3
#define NO_WRAP                       4
#define AUTOINDENT                    5
#define VIEW_MODE                     6
#define USE_MOUSE                     7
#define USE_REGEXP                    8
#define SAVE_ON_EXIT                  9
#define CUT_FROM_CURSOR               10
#define BACKWARDS_SEARCH              11
#define MULTIBUFFER                   12
#define REBIND_DELETE                 13
#define RAW_SEQUENCES                 14
#define NO_CONVERT                    15
#define MAKE_BACKUP                   16
#define INSECURE_BACKUP               17
#define NO_SYNTAX                     18
#define PRESERVE                      19
#define HISTORYLOG                    20
#define RESTRICTED                    21
#define SMART_HOME                    22
#define WHITESPACE_DISPLAY            23
#define TABS_TO_SPACES                24
#define QUICK_BLANK                   25
#define WORD_BOUNDS                   26
#define NO_NEWLINES                   27
#define BOLD_TEXT                     28
#define SOFTWRAP                      29
#define POSITIONLOG                   30
#define LOCKING                       31
#define NOREAD_MODE                   32
#define MAKE_IT_UNIX                  33
#define TRIM_BLANKS                   34
#define SHOW_CURSOR                   35
#define LINE_NUMBERS                  36
#define AT_BLANKS                     37
#define AFTER_ENDS                    38
#define LET_THEM_ZAP                  39
#define BREAK_LONG_LINES              40
#define JUMPY_SCROLLING               41
#define EMPTY_LINE                    42
#define INDICATOR                     43
#define BOOKSTYLE                     44
#define COLON_PARSING                 45
#define STATEFLAGS                    46
#define USE_MAGIC                     47
#define MINIBAR                       48
#define ZERO                          49
#define MODERN_BINDINGS               50
#define EXPERIMENTAL_FAST_LIVE_SYNTAX 51
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
#define CLI_OPT_IGNORERCFILE   (1 << 0)
#define CLI_OPT_VERSION        (1 << 1)
#define CLI_OPT_HELP           (1 << 2)
#define CLI_OPT_SYNTAX         (1 << 3)
#define CLI_OPT_RCFILE         (1 << 4)
#define CLI_OPT_GUIDESTRIPE    (1 << 5)
#define CLI_OPT_WORDCHARS      (1 << 6)
#define CLI_OPT_TABSIZE        (1 << 7)
#define CLI_OPT_OPERATINGDIR   (1 << 8)
#define CLI_OPT_FILL           (1 << 9)
#define CLI_OPT_SPELLER        (1 << 10)
#define CLI_OPT_LISTSYNTAX     (1 << 11)
#define CLI_OPT_BACKUPDIR      (1 << 12)
#define CLI_OPT_BREAKLONGLINES (1 << 13)
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

#define NUMBER_OF_FLAGS 51
constexpr const char *const DEFAULT_RESPONSE_ON_NONE = "Ehm...";

/* This is a masterpiece of a map.
 * I have succesfully made the way to get the description of a flag,
 * instant and without any overhead,
 * while using less memory then a unordered map.
 * USAGE:
 * - (&epithetOfFlagMap[flag].value[0]) will return the underlying ptr to the description of the flag
 * The (flag / description) map. */
constexpr_map<unsigned int, std::string_view, NUMBER_OF_FLAGS> epithetOfFlagMap = {
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
#define MMAIN        (1 << 0)
#define MWHEREIS     (1 << 1)
#define MREPLACE     (1 << 2)
#define MREPLACEWITH (1 << 3)
#define MGOTOLINE    (1 << 4)
#define MWRITEFILE   (1 << 5)
#define MINSERTFILE  (1 << 6)
#define MEXECUTE     (1 << 7)
#define MHELP        (1 << 8)
#define MSPELL       (1 << 9)
#define MBROWSER     (1 << 10)
#define MWHEREISFILE (1 << 11)
#define MGOTODIR     (1 << 12)
#define MYESNO       (1 << 13)
#define MLINTER      (1 << 14)
#define MFINDINHELP  (1 << 15)
#define MMOST                                                                                       \
    (MMAIN | MWHEREIS | MREPLACE | MREPLACEWITH | MGOTOLINE | MWRITEFILE | MINSERTFILE | MEXECUTE | \
     MWHEREISFILE | MGOTODIR | MFINDINHELP | MSPELL | MLINTER)
#define MSOME (MMOST | MBROWSER)
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

constexpr_map<std::string_view, unsigned int, 14> toggleOptionMap = {
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

#define CS_STRUCT    (1 << 0)
#define CS_ENUM      (1 << 1)
#define CS_INT       (1 << 2)
#define CS_VOID      (1 << 3)
#define CS_LONG      (1 << 4)
#define CS_CHAR      (1 << 5)
#define CS_CLASS     (1 << 6)
#define CS_BOOL      (1 << 7)
#define CS_SIZEOF    (1 << 8)
#define CS_SHORT     (1 << 9)
#define CS_NAMESPACE (1 << 10)
#define CS_STATIC    (1 << 11)
#define CS_UNSIGNED  (1 << 12)
#define CS_CONST     (1 << 13)
#define CS_NULL      (1 << 14)
#define CS_TRUE      (1 << 15)
#define CS_FALSE     (1 << 16)
#define CS_SIZE_T    (1 << 17)
#define CS_SSIZE_T   (1 << 18)
#define CS_IF        (1 << 19)
#define CS_CASE      (1 << 20)
#define CS_ELSE      (1 << 21)
#define CS_SWITCH    (1 << 22)
#define CS_TYPEDEF   (1 << 23)
#define CS_FOR       (1 << 24)
#define CS_WHILE     (1 << 25)
#define CS_RETURN    (1 << 26)
#define CS_INCLUDE   (1 << 27)
#define CS_DEFINE    (1 << 28)
constexpr_map<std::string_view, unsigned int, 29> c_syntax_map = {
    {{"struct", CS_STRUCT},   {"enum", CS_ENUM},
     {"int", CS_INT},         {"void", CS_VOID},
     {"long", CS_LONG},       {"char", CS_CHAR},
     {"include", CS_INCLUDE}, {"define", CS_DEFINE},
     {"class", CS_CLASS},     {"bool", CS_BOOL},
     {"size_t", CS_SIZE_T},   {"ssize_t", CS_SSIZE_T},
     {"short", CS_SHORT},     {"namespace", CS_NAMESPACE},
     {"static", CS_STATIC},   {"unsigned", CS_UNSIGNED},
     {"const", CS_CONST},     {"NULL", CS_NULL},
     {"TRUE", CS_TRUE},       {"FALSE", CS_FALSE},
     {"if", CS_IF},           {"case", CS_CASE},
     {"else", CS_ELSE},       {"switch", CS_SWITCH},
     {"typedef", CS_TYPEDEF}, {"for", CS_FOR},
     {"while", CS_WHILE},     {"return", CS_RETURN},
     {"sizeof", CS_SIZEOF}}
};
