/// \file constexpr_def.h
#pragma once
#include <Mlib/def.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

struct MapEntry
{
    std::string_view key;
    u32              value;
};

template <u64 Size>
using constexprMap = std::array<MapEntry, Size>;

struct HashMapEntry
{
    u64 hash;
    u16 value;
};

template <u64 Size>
using constexprHashMap = std::array<HashMapEntry, Size>;

constexpr u64
fnv1a_32(const s8 *s, u64 count)
{
    constexpr u64 fnv_prime    = 16777619u;
    constexpr u64 offset_basis = 2166136261u;

    u64 hash = offset_basis;
    for (u64 i = 0; i < count; ++i)
    {
        hash ^= s[i];
        hash *= fnv_prime;
    }
    return hash;
}

constexpr u64
hash_string(const char *s)
{
    return fnv1a_32(s, std::char_traits<s8>::length(s));
}

constexpr std::size_t
operator"" _hash(const char *s, std::size_t)
{
    return hash_string(s);
}

//
//  Identifiers for color options.
//
constexpr u8 TITLE_BAR          = 0;
constexpr u8 LINE_NUMBER        = 1;
constexpr u8 GUIDE_STRIPE       = 2;
constexpr u8 SCROLL_BAR         = 3;
constexpr u8 SELECTED_TEXT      = 4;
constexpr u8 SPOTLIGHTED        = 5;
constexpr u8 MINI_INFOBAR       = 6;
constexpr u8 PROMPT_BAR         = 7;
constexpr u8 STATUS_BAR         = 8;
constexpr u8 ERROR_MESSAGE      = 9;
constexpr u8 KEY_COMBO          = 10;
constexpr u8 FUNCTION_TAG       = 11;
constexpr u8 NUMBER_OF_ELEMENTS = 12;
//
//  The color options hash values.
// //
// constexpr u64 TITLE_BAR_HASH     = "titlecolor"_hash;
// constexpr u64 LINE_NUMBER_HASH   = "numbercolor"_hash;
// constexpr u64 GUIDE_STRIPE_HASH  = "stripecolor"_hash;
// constexpr u64 SCROLL_BAR_HASH    = "scrollercolor"_hash;
// constexpr u64 SELECTED_TEXT_HASH = "selectedcolor"_hash;
// constexpr u64 SPOTLIGHTED_HASH   = "spotlightcolor"_hash;
// constexpr u64 MINI_INFOBAR_HASH  = "minicolor"_hash;
// constexpr u64 PROMPT_BAR_HASH    = "promptcolor"_hash;
// constexpr u64 STATUS_BAR_HASH    = "statuscolor"_hash;
// constexpr u64 ERROR_MESSAGE_HASH = "errorcolor"_hash;
// constexpr u64 KEY_COMBO_HASH     = "keycolor"_hash;
// constexpr u64 FUNCTION_TAG_HASH  = "functioncolor"_hash;
// //
//  The color options map.
//
constexpr constexprMap<12> colorOptionsMap = {
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
// constexpr constexprHashMap<12> colorOptionHashMap = {
//     {{TITLE_BAR_HASH, TITLE_BAR},
//      {LINE_NUMBER_HASH, LINE_NUMBER},
//      {GUIDE_STRIPE_HASH, GUIDE_STRIPE},
//      {SCROLL_BAR_HASH, SCROLL_BAR},
//      {SELECTED_TEXT_HASH, SELECTED_TEXT},
//      {SPOTLIGHTED_HASH, SPOTLIGHTED},
//      {MINI_INFOBAR_HASH, MINI_INFOBAR},
//      {PROMPT_BAR_HASH, PROMPT_BAR},
//      {STATUS_BAR_HASH, STATUS_BAR},
//      {ERROR_MESSAGE_HASH, ERROR_MESSAGE},
//      {KEY_COMBO_HASH, KEY_COMBO},
//      {FUNCTION_TAG_HASH, FUNCTION_TAG}}
// };

//
//  Identifiers for the different configuration options.
//
constexpr u16 OPERATINGDIR     = (1 << 0);
constexpr u16 FILL             = (1 << 1);
constexpr u16 MATCHBRACKETS    = (1 << 2);
constexpr u16 WHITESPACE       = (1 << 3);
constexpr u16 PUNCT            = (1 << 4);
constexpr u16 BRACKETS         = (1 << 5);
constexpr u16 QUOTESTR         = (1 << 6);
constexpr u16 SPELLER          = (1 << 7);
constexpr u16 BACKUPDIR        = (1 << 8);
constexpr u16 WORDCHARS        = (1 << 9);
constexpr u16 GUIDESTRIPE      = (1 << 10);
constexpr u16 CONF_OPT_TABSIZE = (1 << 11);
//
//  The configuration options map.
//
constexpr constexprMap<12> configOptionMap = {
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

//
//  Identifiers for the different syntax options.
//
constexpr u8 SYNTAX_OPT_COLOR     = (1 << 0);
constexpr u8 SYNTAX_OPT_ICOLOR    = (1 << 1);
constexpr u8 SYNTAX_OPT_COMMENT   = (1 << 2);
constexpr u8 SYNTAX_OPT_TABGIVES  = (1 << 3);
constexpr u8 SYNTAX_OPT_LINTER    = (1 << 4);
constexpr u8 SYNTAX_OPT_FORMATTER = (1 << 5);
//
//  The syntax options map.
//
constexpr constexprMap<6> syntaxOptionMap = {
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
constexpr u8 DONTUSE            = 0;
constexpr u8 CASE_SENSITIVE     = 1;
constexpr u8 CONSTANT_SHOW      = 2;
constexpr u8 NO_HELP            = 3;
constexpr u8 NO_WRAP            = 4;
constexpr u8 AUTOINDENT         = 5;
constexpr u8 VIEW_MODE          = 6;
constexpr u8 USE_MOUSE          = 7;
constexpr u8 USE_REGEXP         = 8;
constexpr u8 SAVE_ON_EXIT       = 9;
constexpr u8 CUT_FROM_CURSOR    = 10;
constexpr u8 BACKWARDS_SEARCH   = 11;
constexpr u8 MULTIBUFFER        = 12;
constexpr u8 REBIND_DELETE      = 13;
constexpr u8 RAW_SEQUENCES      = 14;
constexpr u8 NO_CONVERT         = 15;
constexpr u8 MAKE_BACKUP        = 16;
constexpr u8 INSECURE_BACKUP    = 17;
constexpr u8 NO_SYNTAX          = 18;
constexpr u8 PRESERVE           = 19;
constexpr u8 HISTORYLOG         = 20;
constexpr u8 RESTRICTED         = 21;
constexpr u8 SMART_HOME         = 22;
constexpr u8 WHITESPACE_DISPLAY = 23;
constexpr u8 TABS_TO_SPACES     = 24;
constexpr u8 QUICK_BLANK        = 25;
constexpr u8 WORD_BOUNDS        = 26;
constexpr u8 NO_NEWLINES        = 27;
constexpr u8 BOLD_TEXT          = 28;
constexpr u8 SOFTWRAP           = 29;
constexpr u8 POSITIONLOG        = 30;
constexpr u8 LOCKING            = 31;
constexpr u8 NOREAD_MODE        = 32;
constexpr u8 MAKE_IT_UNIX       = 33;
constexpr u8 TRIM_BLANKS        = 34;
constexpr u8 SHOW_CURSOR        = 35;
constexpr u8 LINE_NUMBERS       = 36;
constexpr u8 AT_BLANKS          = 37;
constexpr u8 AFTER_ENDS         = 38;
constexpr u8 LET_THEM_ZAP       = 39;
constexpr u8 BREAK_LONG_LINES   = 40;
constexpr u8 JUMPY_SCROLLING    = 41;
constexpr u8 EMPTY_LINE         = 42;
constexpr u8 INDICATOR          = 43;
constexpr u8 BOOKSTYLE          = 44;
constexpr u8 COLON_PARSING      = 45;
constexpr u8 STATEFLAGS         = 46;
constexpr u8 USE_MAGIC          = 47;
constexpr u8 MINIBAR            = 48;
constexpr u8 ZERO               = 49;
constexpr u8 MODERN_BINDINGS    = 50;
//
//  The flags map.
//
static const constexprMap<94> flagOptionsMap = {
    {
     {"-A", SMART_HOME},
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
     {"--magic", USE_MAGIC},
     }
};

//
//  Identifiers for the different command line options.
//
constexpr u32 CLI_OPT_IGNORERCFILE   = (1 << 0);
constexpr u32 CLI_OPT_VERSION        = (1 << 1);
constexpr u32 CLI_OPT_HELP           = (1 << 2);
constexpr u32 CLI_OPT_SYNTAX         = (1 << 3);
constexpr u32 CLI_OPT_RCFILE         = (1 << 4);
constexpr u32 CLI_OPT_GUIDESTRIPE    = (1 << 5);
constexpr u32 CLI_OPT_WORDCHARS      = (1 << 6);
constexpr u32 CLI_OPT_TABSIZE        = (1 << 7);
constexpr u32 CLI_OPT_OPERATINGDIR   = (1 << 8);
constexpr u32 CLI_OPT_FILL           = (1 << 9);
constexpr u32 CLI_OPT_SPELLER        = (1 << 10);
constexpr u32 CLI_OPT_LISTSYNTAX     = (1 << 11);
constexpr u32 CLI_OPT_BACKUPDIR      = (1 << 12);
constexpr u32 CLI_OPT_BREAKLONGLINES = (1 << 13);
//
//  The command line options map.
//
constexpr constexprMap<25> cliOptionMap = {
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

//
//  Legazy code, from nano source code.
//
//
//  Identifiers for the different menus.
//
constexpr u16 MMAIN        = (1 << 0);
constexpr u16 MWHEREIS     = (1 << 1);
constexpr u16 MREPLACE     = (1 << 2);
constexpr u16 MREPLACEWITH = (1 << 3);
constexpr u16 MGOTOLINE    = (1 << 4);
constexpr u16 MWRITEFILE   = (1 << 5);
constexpr u16 MINSERTFILE  = (1 << 6);
constexpr u16 MEXECUTE     = (1 << 7);
constexpr u16 MHELP        = (1 << 8);
constexpr u16 MSPELL       = (1 << 9);
constexpr u16 MBROWSER     = (1 << 10);
constexpr u16 MWHEREISFILE = (1 << 11);
constexpr u16 MGOTODIR     = (1 << 12);
constexpr u16 MYESNO       = (1 << 13);
constexpr u16 MLINTER      = (1 << 14);
constexpr u16 MFINDINHELP  = (1 << 15);
constexpr u16 MMOST = (MMAIN | MWHEREIS | MREPLACE | MREPLACEWITH | MGOTOLINE | MWRITEFILE | MINSERTFILE | MEXECUTE |
                       MWHEREISFILE | MGOTODIR | MFINDINHELP | MSPELL | MLINTER);
constexpr u16 MSOME = MMOST | MBROWSER;
//
//  The menus map.
//
constexpr constexprMap<16> menuOptionMap = {
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
