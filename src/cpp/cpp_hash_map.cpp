/* C++ code produced by gperf version 3.1 */
/* Command-line: gperf --language=C++ cpp_hash_map  */
/* Computed positions: -k'1,3' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) && ('%' == 37) && ('&' == 38) && ('\'' == 39) &&       \
      ('(' == 40) && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) && ('-' == 45) && ('.' == 46) &&        \
      ('/' == 47) && ('0' == 48) && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) && ('5' == 53) &&        \
      ('6' == 54) && ('7' == 55) && ('8' == 56) && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) &&        \
      ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) && ('B' == 66) && ('C' == 67) && ('D' == 68) &&        \
      ('E' == 69) && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) && ('J' == 74) && ('K' == 75) &&        \
      ('L' == 76) && ('M' == 77) && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) && ('R' == 82) &&        \
      ('S' == 83) && ('T' == 84) && ('U' == 85) && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) &&        \
      ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) && ('^' == 94) && ('_' == 95) && ('a' == 97) &&       \
      ('b' == 98) && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) && ('g' == 103) && ('h' == 104) &&   \
      ('i' == 105) && ('j' == 106) && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) && ('o' == 111) && \
      ('p' == 112) && ('q' == 113) && ('r' == 114) && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) && \
      ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) && ('{' == 123) && ('|' == 124) && ('}' == 125) && \
      ('~' == 126))
/* The character set is not based on ISO-646.  */
#    error \
        "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "cpp_hash_map"

#include <iostream>
#include <string>

struct keyword
{
    const char *name;
    int         value;
};

#define TOTAL_KEYWORDS  34
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE  4
#define MAX_HASH_VALUE  59
/* maximum key range = 56, duplicates = 0 */

class HashMap
{
private:
    static inline unsigned int hash(const char *str, size_t len);

public:
    static const char *find_keyword(const char *str, size_t len);
};

inline unsigned int
HashMap::hash(const char *str, size_t len)
{
    static unsigned char asso_values[] = {
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 0,  60, 60, 60, 60, 60, 30, 60,
        25, 60, 60, 60, 60, 60, 0,  0,  60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 25, 40, 15, 60, 10, 0,  60,
        60, 0,  60, 60, 5,  40, 5,  5,  0,  60, 10, 10, 5,  10, 35, 60, 60, 60, 0,  60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60};
    return len + asso_values[static_cast<unsigned char>(str[2])] + asso_values[static_cast<unsigned char>(str[0])];
}

const char *
HashMap::find_keyword(const char *str, size_t len)
{
    static const char *wordlist[] = {
        "",      "",       "",        "",         "TRUE",      "",      "",       "private", "int",      "this",
        "false", "inline", "typedef", "typename", "long",      "union", "sizeof", "nullptr", "explicit", "true",
        "short", "extern", "",        "noexcept", "enum",      "const", "struct", "",        "unsigned", "constexpr",
        "",      "",       "",        "",         "auto",      "FALSE", "",       "",        "",         "void",
        "",      "static", "",        "",         "char",      "class", "public", "",        "volatile", "bool",
        "",      "",       "",        "template", "namespace", "",      "",       "",        "",         "NULL"};
    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
        unsigned int key = hash(str, len);
        if (key <= MAX_HASH_VALUE)
        {
            const char *s = wordlist[key];
            if (*str == *s && !strcmp(str + 1, s + 1))
            {
                return s;
            }
        }
    }
    return 0;
}
#line 49 "cpp_hash_map"
