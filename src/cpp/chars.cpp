/// @file chars.cpp

#include "../include/prototypes.h"

#include <Mlib/Profile.h>

#include <cstring>
#include <cwctype>

/* Whether we've enabled UTF-8 support.
 * Initially set to 'FALSE', and then set to 'TRUE' by utf8_init(). */
static bool use_utf8 = FALSE;

/* Enable UTF-8 support.  Set the 'use_utf8' variable to 'TRUE'. */
void
utf8_init(void)
{
    use_utf8 = TRUE;
}

/* Checks if UTF-8 support has been enabled. */
bool
using_utf8(void)
{
    return use_utf8;
}

/* Return 'TRUE' when the given character is some kind of letter. */
bool
is_alpha_char(const char *const c)
{
    wchar_t wc;
    if (mbtowide(wc, c) < 0)
    {
        return FALSE;
    }
    return iswalpha(wc);
}

/* Return TRUE when the given character is some kind of letter or a digit. */
bool
is_alnum_char(const char *const c)
{
    wchar_t wc;
    if (mbtowide(wc, c) < 0)
    {
        return FALSE;
    }
    return iswalnum(wc);
}

/* Return TRUE when the given character is space or tab or other whitespace. */
bool
is_blank_char(const char *const c)
{
    wchar_t wc;
    if ((signed char)*c >= 0)
    {
        return (*c == ' ' || *c == '\t');
    }
    if (mbtowide(wc, c) < 0)
    {
        return FALSE;
    }
    return iswblank(wc);
}

/* Return 'TRUE' when the given character is a control character. */
bool
is_cntrl_char(const char *const c)
{
    if (use_utf8)
    {
        return ((c[0] & 0xE0) == 0 || c[0] == DEL_CODE || ((signed char)c[0] == -62 && (signed char)c[1] < -96));
    }
    else
    {
        return ((*c & 0x60) == 0 || *c == DEL_CODE);
    }
}

/* Return 'TRUE' when the given character is a punctuation character. */
bool
is_punct_char(const char *const c)
{
    wchar_t wc;
    if (mbtowide(wc, c) < 0)
    {
        return FALSE;
    }
    return iswpunct(wc);
}

/* Return TRUE when the given character is word-forming (it is alphanumeric or
 * specified in 'wordchars', or it is punctuation when allow_punct is TRUE). */
bool
is_word_char(const char *const c, bool allow_punct)
{
    if (*c == '\0')
    {
        return FALSE;
    }
    if (is_alnum_char(c))
    {
        return TRUE;
    }
    if (allow_punct && is_punct_char(c))
    {
        return TRUE;
    }
    if (word_chars != NULL && *word_chars != '\0')
    {
        char      symbol[MAXCHARLEN + 1];
        const int symlen = collect_char(c, symbol);
        symbol[symlen]   = '\0';
        return (constexpr_strstr(word_chars, symbol) != NULL);
    }
    else
    {
        return FALSE;
    }
}

/* Return the visible representation of control character c. */
char
control_rep(const signed char c)
{
    if (c == DEL_CODE)
    {
        return '?';
    }
    else if (c == -97)
    {
        return '=';
    }
    else if (c < 0)
    {
        return c + 224;
    }
    else
    {
        return c + 64;
    }
}

/* Return the visible representation of multibyte control character c. */
char
control_mbrep(const char *const c, bool isdata)
{
    /* An embedded newline is an encoded NUL if 'isdata' is TRUE. */
    if (*c == '\n' && (isdata || as_an_at))
    {
        return '@';
    }
    if (use_utf8)
    {
        if ((unsigned char)c[0] < 128)
        {
            return control_rep(c[0]);
        }
        else
        {
            return control_rep(c[1]);
        }
    }
    else
    {
        return control_rep(*c);
    }
}

/* Convert the given multibyte sequence c to wide character wc, and return
 * the number of bytes in the sequence, or -1 for an invalid sequence. */
int
mbtowide(wchar_t &wc, const char *const c)
{
    if ((signed char)*c < 0 && use_utf8)
    {
        unsigned char v1 = (unsigned char)c[0];
        unsigned char v2 = (unsigned char)c[1] ^ 0x80;
        if (v2 > 0x3F || v1 < 0xC2)
        {
            return -1;
        }
        if (v1 < 0xE0)
        {
            wc = (((unsigned int)(v1 & 0x1F) << 6) | (unsigned int)v2);
            return 2;
        }
        unsigned char v3 = (unsigned char)c[2] ^ 0x80;
        if (v3 > 0x3F)
        {
            return -1;
        }
        if (v1 < 0xF0)
        {
            if ((v1 > 0xE0 || v2 >= 0x20) && (v1 != 0xED || v2 < 0x20))
            {
                wc = (((unsigned int)(v1 & 0x0F) << 12) | ((unsigned int)v2 << 6) | (unsigned int)v3);
                return 3;
            }
            else
            {
                return -1;
            }
        }
        unsigned char v4 = (unsigned char)c[3] ^ 0x80;
        if (v4 > 0x3F || v1 > 0xF4)
        {
            return -1;
        }
        if ((v1 > 0xF0 || v2 >= 0x10) && (v1 != 0xF4 || v2 < 0x10))
        {
            wc = (((unsigned int)(v1 & 0x07) << 18) | ((unsigned int)v2 << 12) | ((unsigned int)v3 << 6) |
                  (unsigned int)v4);
            return 4;
        }
        else
        {
            return -1;
        }
    }
    wc = (unsigned int)*c;
    return 1;
}

/* Return 'TRUE' when the given character occupies two cells. */
bool
is_doublewidth(const char *const ch)
{
    wchar_t wc;
    /* Only from U+1100 can code points have double width. */
    if ((unsigned char)*ch < 0xE1 || !use_utf8)
    {
        return FALSE;
    }
    if (mbtowide(wc, ch) < 0)
    {
        return FALSE;
    }
    return (wcwidth(wc) == 2);
}

/* Return 'TRUE' when the given character occupies zero cells. */
bool
is_zerowidth(const char *ch)
{
    wchar_t wc;
    /* Only from U+0300 can code points have zero width. */
    if ((unsigned char)*ch < 0xCC || !use_utf8)
    {
        return FALSE;
    }
    if (mbtowide(wc, ch) < 0)
    {
        return FALSE;
    }
#if defined(__OpenBSD__)
    /* Work around an OpenBSD bug -- see https://sv.gnu.org/bugs/?60393. */
    if (wc >= 0xF0000)
    {
        return FALSE;
    }
#endif
    return (wcwidth(wc) == 0);
}

/* Return the number of bytes in the character that starts at *pointer. */
int
char_length(const char *const &pointer)
{
    if ((unsigned char)*pointer > 0xC1 && use_utf8)
    {
        const unsigned char c1 = (unsigned char)pointer[0];
        const unsigned char c2 = (unsigned char)pointer[1];
        if ((c2 ^ 0x80) > 0x3F)
        {
            return 1;
        }
        if (c1 < 0xE0)
        {
            return 2;
        }
        if (((unsigned char)pointer[2] ^ 0x80) > 0x3F)
        {
            return 1;
        }
        if (c1 < 0xF0)
        {
            if ((c1 > 0xE0 || c2 >= 0xA0) && (c1 != 0xED || c2 < 0xA0))
            {
                return 3;
            }
            else
            {
                return 1;
            }
        }
        if (((unsigned char)pointer[3] ^ 0x80) > 0x3F)
        {
            return 1;
        }
        if (c1 > 0xF4)
        {
            return 1;
        }
        if ((c1 > 0xF0 || c2 >= 0x90) && (c1 != 0xF4 || c2 < 0x90))
        {
            return 4;
        }
    }
    return 1;
}

/* Return the number of (multibyte) characters in the given string. */
unsigned long
mbstrlen(const char *pointer)
{
    unsigned long count = 0;
    while (*pointer != '\0')
    {
        pointer += char_length(pointer);
        count++;
    }
    return count;
}

/* Return the length (in bytes) of the character at the start of the
 * given string, and return a copy of this character in *thechar. */
int
collect_char(const char *const str, char *c)
{
    const int charlen = char_length(str);
    for (int i = 0; i < charlen; i++)
    {
        c[i] = str[i];
    }
    return charlen;
}

/* Return the length ( in bytes ) of the character at the start of
 * the given string, and add this character's width to '*column'. */
int
advance_over(const char *const str, unsigned long &column)
{
    if ((char)*str < 0 && use_utf8)
    {
        /* A UTF-8 upper control code has two bytes and takes two columns. */
        if ((unsigned char)str[0] == 0xC2 && (signed char)str[1] < -96)
        {
            column += 2;
            return 2;
        }
        else
        {
            wchar_t   wc;
            const int charlen = mbtowide(wc, str);
            if (charlen < 0)
            {
                column += 1;
                return 1;
            }
            const int width = wcwidth(wc);
#if defined(__OpenBSD__)
            *column += (width < 0 || wc >= 0xF0000) ? 1 : width;
#else
            column += (width < 0) ? 1 : width;
#endif
            return charlen;
        }
    }
    if ((unsigned char)*str < 0x20)
    {
        if (*str == '\t')
        {
            column += tabsize - column % tabsize;
        }
        else
        {
            column += 2;
        }
    }
    else if (0x7E < (unsigned char)*str && (unsigned char)*str < 0xA0)
    {
        column += 2;
    }
    else
    {
        column += 1;
    }
    return 1;
}

/* Return the index in buf of the beginning of
 * the multibyte character before the one at pos. */
unsigned long
step_left(const char *const buf, const unsigned long pos)
{
    if (use_utf8)
    {
        unsigned long before, charlen = 0;
        if (pos < 4)
        {
            before = 0;
        }
        else
        {
            const char *ptr = buf + pos;
            /* Probe for a valid starter byte in the preceding four bytes. */
            if ((signed char)*--ptr > -65)
            {
                before = pos - 1;
            }
            else if ((signed char)*--ptr > -65)
            {
                before = pos - 2;
            }
            else if ((signed char)*--ptr > -65)
            {
                before = pos - 3;
            }
            else if ((signed char)*--ptr > -65)
            {
                before = pos - 4;
            }
            else
            {
                before = pos - 1;
            }
        }
        /* Move forward again until we reach the original character,
         * so we know the length of its preceding character. */
        while (before < pos)
        {
            charlen = char_length(buf + before);
            before += charlen;
        }
        return before - charlen;
    }
    else
    {
        return (pos == 0 ? 0 : pos - 1);
    }
}

/* Return the index in buf of the beginning of the multibyte character
 * after the one at pos. */
unsigned long
step_right(const char *const buf, const unsigned long pos)
{
    return pos + char_length(buf + pos);
}

/* This function is equivalent to strcasecmp() for multibyte strings. */
int
mbstrcasecmp(const char *s1, const char *s2)
{
    return mbstrncasecmp(s1, s2, HIGHEST_POSITIVE);
}

/* This function is equivalent to strncasecmp() for multibyte strings. */
int
mbstrncasecmp(const char *s1, const char *s2, unsigned long n)
{
    if (use_utf8)
    {
        wchar_t wc1, wc2;
        while (*s1 != '\0' && *s2 != '\0' && n > 0)
        {
            if (*s1 >= 0 && *s2 >= 0)
            {
                if ('A' <= (*s1 & 0x5F) && (*s1 & 0x5F) <= 'Z')
                {
                    if ('A' <= (*s2 & 0x5F) && (*s2 & 0x5F) <= 'Z')
                    {
                        if ((*s1 & 0x5F) != (*s2 & 0x5F))
                        {
                            return ((*s1 & 0x5F) - (*s2 & 0x5F));
                        }
                    }
                    else
                    {
                        return ((int)(*s1 | 0x20) - (int)*s2);
                    }
                }
                else if ('A' <= (*s2 & 0x5F) && (*s2 & 0x5F) <= 'Z')
                {
                    return ((int)*s1 - (int)(*s2 | 0x20));
                }
                else if (*s1 != *s2)
                {
                    return ((int)*s1 - (int)*s2);
                }
                s1++;
                s2++;
                n--;
                continue;
            }
            bool bad1 = (mbtowide(wc1, s1) < 0), bad2 = (mbtowide(wc2, s2) < 0);
            if (bad1 || bad2)
            {
                if (*s1 != *s2)
                {
                    return (int)((unsigned char)*s1 - (unsigned char)*s2);
                }
                if (bad1 != bad2)
                {
                    return (bad1 ? 1 : -1);
                }
            }
            else
            {
                const int difference = (int)towlower((wint_t)wc1) - (int)towlower((wint_t)wc2);
                if (difference)
                {
                    return difference;
                }
            }
            s1 += char_length(s1), s2 += char_length(s2);
            n--;
        }
        return (n > 0) ? (int)((unsigned char)*s1 - (unsigned char)*s2) : 0;
    }
    else
    {
        return strncasecmp(s1, s2, n);
    }
}

/* This function is equivalent to strcasestr() for multibyte strings. */
char *
mbstrcasestr(const char *haystack, const char *const needle)
{
    if (use_utf8)
    {
        const unsigned long needle_len = mbstrlen(needle);
        while (*haystack != '\0')
        {
            if (mbstrncasecmp(haystack, needle, needle_len) == 0)
            {
                return (char *)haystack;
            }
            haystack += char_length(haystack);
        }
        return NULL;
    }
    else
    {
        return (char *)strcasestr(haystack, needle);
    }
}

/* This function is equivalent to strstr(),
 * except in that it scans the string in reverse,
 * starting at pointer. */
char *
revstrstr(const char *const haystack, const char *const needle, const char *pointer)
{
    const unsigned long needle_len = strlen(needle), tail_len = strlen(pointer);
    if (tail_len < needle_len)
    {
        pointer -= (needle_len - tail_len);
    }
    while (pointer >= haystack)
    {
        if (strncmp(pointer, needle, needle_len) == 0)
        {
            return (char *)pointer;
        }
        pointer--;
    }
    return NULL;
}

/* This function is equivalent to strcasestr(), except in that it scans
 * the string in reverse, starting at pointer. */
char *
revstrcasestr(const char *const haystack, const char *const needle, const char *pointer)
{
    const unsigned long needle_len = strlen(needle), tail_len = strlen(pointer);
    if (tail_len < needle_len)
    {
        pointer -= (needle_len - tail_len);
    }
    while (pointer >= haystack)
    {
        if (strncasecmp(pointer, needle, needle_len) == 0)
        {
            return (char *)pointer;
        }
        pointer--;
    }
    return NULL;
}

/* This function is equivalent to strcasestr() for multibyte strings,
 * except in that it scans the string in reverse, starting at pointer. */
char *
mbrevstrcasestr(const char *const haystack, const char *const needle, const char *pointer)
{
    if (use_utf8)
    {
        const unsigned long needle_len = mbstrlen(needle), tail_len = mbstrlen(pointer);
        if (tail_len < needle_len)
        {
            pointer -= (needle_len - tail_len);
        }
        if (pointer < haystack)
        {
            return NULL;
        }
        while (TRUE)
        {
            if (!mbstrncasecmp(pointer, needle, needle_len))
            {
                return (char *)pointer;
            }
            if (pointer == haystack)
            {
                return NULL;
            }
            pointer = haystack + step_left(haystack, pointer - haystack);
        }
    }
    else
    {
        return revstrcasestr(haystack, needle, pointer);
    }
}

/* This function is equivalent to strchr() for multibyte strings.
 * It is used to find the first occurrence of a character in a string.
 * The character to find is given as a multibyte string.
 * The function is used in justify.c to find the first space in a line. */
char *
mbstrchr(const char *string, const char *const chr)
{
    if (use_utf8)
    {
        bool    bad_s = FALSE, bad_c = FALSE;
        wchar_t ws, wc;
        if (mbtowide(wc, chr) < 0)
        {
            wc    = (unsigned char)*chr;
            bad_c = TRUE;
        }
        while (*string != '\0')
        {
            const int symlen = mbtowide(ws, string);
            if (symlen < 0)
            {
                ws    = (unsigned char)*string;
                bad_s = TRUE;
            }
            if (ws == wc && bad_s == bad_c)
            {
                break;
            }
            string += symlen;
        }
        if (*string == '\0')
        {
            return NULL;
        }
        return (char *)string;
    }
    else
    {
        return strchr((char *)string, *chr);
    }
}

/* Locate, in the given string, the first occurrence of any of
 * the characters in accept, searching forward. */
char *
mbstrpbrk(const char *str, const char *accept)
{
    while (*str != '\0')
    {
        if (mbstrchr(accept, str) != NULL)
        {
            return (char *)str;
        }
        str += char_length(str);
    }
    return NULL;
}

/* Locate, in the string that starts at head, the first occurrence of any of
 * the characters in accept, starting from pointer and searching backwards. */
char *
mbrevstrpbrk(const char *const head, const char *const accept, const char *pointer)
{
    if (*pointer == '\0')
    {
        if (pointer == head)
        {
            return NULL;
        }
        pointer = head + step_left(head, pointer - head);
    }
    while (TRUE)
    {
        if (mbstrchr(accept, pointer) != NULL)
        {
            return (char *)pointer;
        }
        /* If we've reached the head of the string, we found nothing. */
        if (pointer == head)
        {
            return NULL;
        }
        pointer = head + step_left(head, pointer - head);
    }
}

/* Return 'TRUE' if the given string contains at least one blank character. */
bool
has_blank_char(const char *str)
{
    while (*str != '\0' && !is_blank_char(str))
    {
        str += char_length(str);
    }
    return *str;
}

/* Return 'TRUE' when the given string is empty or consists of only blanks. */
bool
white_string(const char *str)
{
    while (*str != '\0' && (is_blank_char(str) || *str == '\r'))
    {
        str += char_length(str);
    }
    return !*str;
}

/* This is the original code from 'nano', and I must say, this is fucking horible.
 * Honestly why the fuck even do it like this, it is the most inefficient way to do it.
 *
 * Remove leading whitespace from the given string.
void
strip_leading_blanks_from(char *str)
{
    while (str && (*str == ' ' || *str == '\t'))
    {
        memmove(str, str + 1, strlen(str));
    }
}
*/

/* Remove leading whitespace from a given string */
void
strip_leading_blanks_from(char *str)
{
    char *start = str;
    for (; start && (*start == '\t' || *start == ' '); start++)
        ;
    (start != str) ? memmove(str, start, strlen(start) + 1) : 0;
}
