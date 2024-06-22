/// @file chars.cpp
#include "../include/prototypes.h"

// #include <cctype>
#include <cstring>
#include <cwchar>
#include <cwctype>

/// @name @c use_utf8
/// @brief
/// - Whether we've enabled UTF-8 support.
/// - Initially set to @p false,
/// - and then set to @p true by @see @c utf8_init().
static bool use_utf8 = false;

/// @name @c utf8_init
/// @brief
/// - Enable UTF-8 support.
/// @details
/// - Modify the global variable @c use_utf8 to @p true
void
utf8_init()
{
    use_utf8 = true;
}

// Is UTF-8 support enabled?
bool
using_utf8()
{
    return use_utf8;
}

// Return TRUE when the given character
// is some kind of letter.
bool
is_alpha_char(const s8 *const &c)
{
    wchar_t wc;
    if (mbtowide(&wc, c) < 0)
    {
        return false;
    }
    return iswalpha(wc);
    /// The following line is commented out because
    /// it is not used in the code.
    /// it was used if not using UTF-8
    /// Original CODE:
    /// @c return @c isalpha((u8)*c);
}

// Return TRUE when the given character
// is some kind of letter or a digit.
bool
is_alnum_char(const s8 *const &c)
{
    wchar_t wc;
    if (mbtowide(&wc, c) < 0)
    {
        return false;
    }
    return iswalnum(wc);
    /// The following line is commented out because
    /// it is not used in the code.
    /// it was used if not using UTF-8
    /// Original CODE:
    /// @c return @c isalnum((u8)*c);
}

// Return TRUE when the given character is space or tab or other whitespace.
bool
is_blank_char(const char *c)
{
#ifdef ENABLE_UTF8
    wchar_t wc;

    if ((signed char)*c >= 0)
    {
        return (*c == ' ' || *c == '\t');
    }

    if (mbtowide(&wc, c) < 0)
    {
        return FALSE;
    }

    return iswblank(wc);
#else
    return isblank((unsigned char)*c);
#endif
}

/* Return TRUE when the given character is a control character. */
bool
is_cntrl_char(const char *c)
{
#ifdef ENABLE_UTF8
    if (use_utf8)
    {
        return ((c[0] & 0xE0) == 0 || c[0] == DEL_CODE || ((signed char)c[0] == -62 && (signed char)c[1] < -96));
    }
    else
#endif
        return ((*c & 0x60) == 0 || *c == DEL_CODE);
}

/* Return TRUE when the given character is a punctuation character. */
bool
is_punct_char(const char *c)
{
#ifdef ENABLE_UTF8
    wchar_t wc;
    if (mbtowide(&wc, c) < 0)
    {
        return FALSE;
    }
    return iswpunct(wc);
#else
    return ispunct((unsigned char)*c);
#endif
}

// Return TRUE when the given character is word-forming (it is alphanumeric or
// specified in 'wordchars', or it is punctuation when allow_punct is TRUE).
bool
is_word_char(const char *c, bool allow_punct)
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
        char symbol[MAXCHARLEN + 1];
        int  symlen = collect_char(c, symbol);

        symbol[symlen] = '\0';
        return (strstr(word_chars, symbol) != NULL);
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
control_mbrep(const char *c, bool isdata)
{
    /* An embedded newline is an encoded NUL if it is data. */
    if (*c == '\n' && (isdata || as_an_at))
    {
        return '@';
    }

#ifdef ENABLE_UTF8
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
#endif
        return control_rep(*c);
}

#ifdef ENABLE_UTF8
/* Convert the given multibyte sequence c to wide character wc, and return
 * the number of bytes in the sequence, or -1 for an invalid sequence. */
int
mbtowide(wchar_t *wc, const char *c)
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
            *wc = (((unsigned int)(v1 & 0x1F) << 6) | (unsigned int)v2);
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
                *wc = (((unsigned int)(v1 & 0x0F) << 12) | ((unsigned int)v2 << 6) | (unsigned int)v3);
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
            *wc = (((unsigned int)(v1 & 0x07) << 18) | ((unsigned int)v2 << 12) | ((unsigned int)v3 << 6) |
                   (unsigned int)v4);
            return 4;
        }
        else
        {
            return -1;
        }
    }

    *wc = (unsigned int)*c;
    return 1;
}

/* Return TRUE when the given character occupies two cells. */
bool
is_doublewidth(const char *ch)
{
    wchar_t wc;

    /* Only from U+1100 can code points have double width. */
    if ((unsigned char)*ch < 0xE1 || !use_utf8)
    {
        return FALSE;
    }

    if (mbtowide(&wc, ch) < 0)
    {
        return FALSE;
    }

    return (wcwidth(wc) == 2);
}

/* Return TRUE when the given character occupies zero cells. */
bool
is_zerowidth(const char *ch)
{
    wchar_t wc;

    /* Only from U+0300 can code points have zero width. */
    if ((unsigned char)*ch < 0xCC || !use_utf8)
    {
        return FALSE;
    }

    if (mbtowide(&wc, ch) < 0)
    {
        return FALSE;
    }

#    if defined(__OpenBSD__)
    /* Work around an OpenBSD bug -- see https://sv.gnu.org/bugs/?60393. */
    if (wc >= 0xF0000)
    {
        return FALSE;
    }
#    endif

    return (wcwidth(wc) == 0);
}
#endif /* ENABLE_UTF8 */

// Return the number of bytes in the character that starts at *pointer.
s32
char_length(const s8 *const &pointer)
{
    if ((u8)*pointer > 0xC1 && use_utf8)
    {
        const u8 c1 = static_cast<u8>(pointer[0]);
        const u8 c2 = static_cast<u8>(pointer[1]);

        if ((c2 ^ 0x80) > 0x3F)
        {
            return 1;
        }

        if (c1 < 0xE0)
        {
            return 2;
        }

        if ((static_cast<u8>(pointer[2]) ^ 0x80) > 0x3F)
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

        if ((static_cast<u8>(pointer[3]) ^ 0x80) > 0x3F)
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
size_t
mbstrlen(const char *pointer)
{
    size_t count = 0;
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
collect_char(const char *string, char *thechar)
{
    int charlen = char_length(string);

    for (int i = 0; i < charlen; i++)
    {
        thechar[i] = string[i];
    }

    return charlen;
}

//
//  Return the length (in bytes) of the character at the start of
//  the given string, and add this character's width to *column. */
//
s32
advance_over(const s8 *string, u64 &column)
{
    if (static_cast<s8>(*string) < 0 && use_utf8)
    {
        //
        //  A UTF-8 upper control code has two bytes and takes two columns.
        //
        if (static_cast<u8>(string[0]) == 0xC2 && static_cast<signed char>(string[1]) < -96)
        {
            column += 2;
            return 2;
        }
        else
        {
            wchar_t wc;

            s32 charlen = mbtowide(&wc, string);
            if (charlen < 0)
            {
                column += 1;
                return 1;
            }
            s32 width = wcwidth(wc);

#if defined(__OpenBSD__)
            *column += (width < 0 || wc >= 0xF0000) ? 1 : width;
#else
            column += (width < 0) ? 1 : width;
#endif
            return charlen;
        }
    }

    if (static_cast<u8>(*string) < 0x20)
    {
        if (*string == '\t')
        {
            column += tabsize - column % tabsize;
        }
        else
        {
            column += 2;
        }
    }
    else if (0x7E < static_cast<u8>(*string) && static_cast<u8>(*string) < 0xA0)
    {
        column += 2;
    }
    else
    {
        column += 1;
    }

    return 1;
}

/* Return the index in buf of the beginning of the multibyte character
 * before the one at pos. */
size_t
step_left(const char *buf, size_t pos)
{
#ifdef ENABLE_UTF8
    if (use_utf8)
    {
        size_t before, charlen = 0;

        if (pos < 4)
        {
            before = 0;
        }
        else
        {
            const char *ptr = buf + pos;

            /* Probe for a valid starter byte in the preceding four bytes. */
            if ((signed char)*(--ptr) > -65)
            {
                before = pos - 1;
            }
            else if ((signed char)*(--ptr) > -65)
            {
                before = pos - 2;
            }
            else if ((signed char)*(--ptr) > -65)
            {
                before = pos - 3;
            }
            else if ((signed char)*(--ptr) > -65)
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
#endif
        return (pos == 0 ? 0 : pos - 1);
}

/* Return the index in buf of the beginning of the multibyte character
 * after the one at pos. */
size_t
step_right(const char *buf, size_t pos)
{
    return pos + char_length(buf + pos);
}

/* This function is equivalent to strcasecmp() for multibyte strings. */
int
mbstrcasecmp(const char *s1, const char *s2)
{
    return mbstrncasecmp(s1, s2, HIGHEST_POSITIVE);
}

// This function is equivalent to strncasecmp() for multibyte strings.
s32
mbstrncasecmp(const s8 *s1, const s8 *s2, u64 n)
{
    if (use_utf8)
    {
        wchar_t wc1, wc2;

        while (*s1 != '\0' && *s2 != '\0' && n > 0)
        {
            if ((s8)*s1 >= 0 && (s8)*s2 >= 0)
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
                        return (static_cast<s32>(*s1 | 0x20) - static_cast<s32>(*s2));
                    }
                }
                else if ('A' <= (*s2 & 0x5F) && (*s2 & 0x5F) <= 'Z')
                {
                    return (static_cast<s32>(*s1) - static_cast<s32>(*s2 | 0x20));
                }
                else if (*s1 != *s2)
                {
                    return (static_cast<s32>(*s1) - static_cast<s32>(*s2));
                }

                s1++;
                s2++;
                n--;
                continue;
            }

            bool bad1 = (mbtowide(&wc1, s1) < 0);
            bool bad2 = (mbtowide(&wc2, s2) < 0);

            if (bad1 || bad2)
            {
                if (*s1 != *s2)
                {
                    return static_cast<u8>(*s1) - static_cast<u8>(*s2);
                }

                if (bad1 != bad2)
                {
                    return (bad1 ? 1 : -1);
                }
            }
            else
            {
                s32 difference = static_cast<s32>(towlower(wc1)) - static_cast<s32>(towlower(wc2));

                if (difference)
                {
                    return difference;
                }
            }

            s1 += char_length(s1);
            s2 += char_length(s2);
            n--;
        }

        return (n > 0) ? (static_cast<u8>(*s1) - static_cast<u8>(*s2)) : 0;
    }
    else
    {
        return strncasecmp(s1, s2, n);
    }
}

/* This function is equivalent to strcasestr() for multibyte strings. */
char *
mbstrcasestr(const char *haystack, const char *needle)
{
#ifdef ENABLE_UTF8
    if (use_utf8)
    {
        size_t needle_len = mbstrlen(needle);

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
#endif
        return (char *)strcasestr(haystack, needle);
}

/* This function is equivalent to strstr(), except in that it scans the
 * string in reverse, starting at pointer. */
char *
revstrstr(const char *haystack, const char *needle, const char *pointer)
{
    size_t needle_len = strlen(needle);
    size_t tail_len   = strlen(pointer);

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
revstrcasestr(const char *haystack, const char *needle, const char *pointer)
{
    size_t needle_len = strlen(needle);
    size_t tail_len   = strlen(pointer);

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

//
/// @name @c mbrevstrcasestr
///
/// @brief
///  -  This function is equivalent to strcasestr() for multibyte strings,
///  -  except in that it scans the string in reverse, starting at pointer.
///
/// @param haystack
///  - The string in which to search.
///
/// @param needle
///  - The string to find.
///
/// @param pointer
///  - The position in the string to start searching from.
///
/// @returns
///  - ( char* ) - A pointer to the first occurrence of the string in the string
///  - ( NULL )  - if the string is not found.
//
s8 *
mbrevstrcasestr(const s8 *haystack, const s8 *needle, const s8 *pointer)
{
    if (use_utf8)
    {
        u64 needle_len = mbstrlen(needle);
        u64 tail_len   = mbstrlen(pointer);

        if (tail_len < needle_len)
        {
            pointer -= (needle_len - tail_len);
        }

        if (pointer < haystack)
        {
            return nullptr;
        }

        while (true)
        {
            if (!mbstrncasecmp(pointer, needle, needle_len))
            {
                return const_cast<s8 *>(pointer);
            }

            if (pointer == haystack)
            {
                return nullptr;
            }

            pointer = haystack + step_left(haystack, pointer - haystack);
        }
    }
    else
    {
        return revstrcasestr(haystack, needle, pointer);
    }
}

//
/// @name @c mbstrchr
///
/// @brief
/// - This function is equivalent to strchr() for multibyte strings.
/// - It is used to find the first occurrence of a character in a string.
/// - The character to find is given as a multibyte string.
/// - The function is used in justify.c to find the first space in a line.
///
/// @param string
/// - The string in which to search.
///
/// @param chr
/// - The character to find.
///
/// @returns
/// - ( char* ) - A pointer to the first occurrence of the character in the string
/// - ( NULL )  - if the character is not found.
//
s8 *
mbstrchr(const s8 *string, const s8 *chr)
{
    if (use_utf8)
    {
        bool    bad_s = false, bad_c = false;
        wchar_t ws, wc;

        if (mbtowide(&wc, chr) < 0)
        {
            wc    = static_cast<u8>(*chr);
            bad_c = true;
        }

        while (*string != '\0')
        {
            s32 symlen = mbtowide(&ws, string);

            if (symlen < 0)
            {
                ws    = static_cast<u8>(*string);
                bad_s = true;
            }

            if (ws == wc && bad_s == bad_c)
            {
                break;
            }

            string += symlen;
        }

        if (*string == '\0')
        {
            return nullptr;
        }

        return const_cast<s8 *>(string);
    }
    else
    {
        return strchr(const_cast<s8 *>(string), *chr);
    }
}

//
/// @name @c mbstrpbrk
///
/// @brief
/// - Locate, in the given string, the first occurrence of any of
/// - the characters in accept, searching forward.
///
/// @param string
/// - The string in which to search.
///
/// @param accept
/// - The set of characters to find.
///
/// @returns
/// - ( char* ) - A pointer to the first occurrence of any of the characters in the string
/// - ( NULL )  - if none of the characters are found.
//
s8 *
mbstrpbrk(const s8 *string, const s8 *accept)
{
    while (*string != '\0')
    {
        if (mbstrchr(accept, string) != NULL)
        {
            return (char *)string;
        }
        string += char_length(string);
    }
    return NULL;
}

/* Locate, in the string that starts at head, the first occurrence of any of
 * the characters in accept, starting from pointer and searching backwards. */
char *
mbrevstrpbrk(const char *head, const char *accept, const char *pointer)
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

#if defined(ENABLE_NANORC) && (!defined(NANO_TINY) || defined(ENABLE_JUSTIFY))
/* Return TRUE if the given string contains at least one blank character. */
bool
has_blank_char(const s8 *string)
{
    while (*string != '\0' && !is_blank_char(string))
    {
        string += char_length(string);
    }

    return *string;
}
#endif

/* Return TRUE when the given string is empty or consists of only blanks. */
bool
white_string(const char *string)
{
    while (*string != '\0' && (is_blank_char(string) || *string == '\r'))
    {
        string += char_length(string);
    }

    return !*string;
}

#if defined(ENABLE_SPELLER) || defined(ENABLE_COLOR)
/* Remove leading whitespace from the given string. */
void
strip_leading_blanks_from(char *string)
{
    while (string && (*string == ' ' || *string == '\t'))
    {
        memmove(string, string + 1, strlen(string));
    }
}
#endif
