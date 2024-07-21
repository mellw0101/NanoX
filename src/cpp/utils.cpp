/// @file utils.cpp
#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <Mlib/constexpr.hpp>
#include <cerrno>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

/**
    Return the user's home directory.  We use $HOME, and if that fails,
    we fall back on the home directory of the effective user ID.
 */
void
get_homedir()
{
    if (homedir == nullptr)
    {
        const char *homenv = getenv("HOME");
        //
        //  When HOME isn't set,or when we're root,
        //  get the home directory from the password file instead.
        //
        if (homenv == nullptr || geteuid() == ROOT_UID)
        {
            const passwd *userage = getpwuid(geteuid());
            if (userage != nullptr)
            {
                homenv = userage->pw_dir;
            }
        }
        //
        //  Only set homedir if some home directory could be determined,
        //  otherwise keep homedir nullpre.
        //
        if (homenv != nullptr && *homenv != '\0')
        {
            homedir = copy_of(homenv);
        }
    }
}

//
//  Return the filename part of the given path.
//
const char *
tail(const char *path)
{
    const char *slash = constexpr_strrchr(path, '/');
    if (slash == nullptr)
    {
        return path;
    }
    else
    {
        return slash + 1;
    }
}

//
//  Return a copy of the two given strings, welded together.
//
char *
concatenate(const char *path, const char *name)
{
    size_t pathlen = constexpr_strlen(path);
    char  *joined  = (char *)nmalloc(pathlen + constexpr_strlen(name) + 1);
    constexpr_strcpy(joined, path);
    constexpr_strcpy(joined + pathlen, name);
    return joined;
}

//
//  Return the number of digits that the given integer n takes up.
//
int
digits(const long n)
{
    if (n < 100000)
    {
        if (n < 1000)
        {
            if (n < 100)
            {
                return 2;
            }
            else
            {
                return 3;
            }
        }
        else
        {
            if (n < 10000)
            {
                return 4;
            }
            else
            {
                return 5;
            }
        }
    }
    else
    {
        if (n < 10000000)
        {
            if (n < 1000000)
            {
                return 6;
            }
            else
            {
                return 7;
            }
        }
        else
        {
            if (n < 100000000)
            {
                return 8;
            }
            else
            {
                return 9;
            }
        }
    }
}

//
/// Original code: From GNU nano 8.0.1
/*
    // Read an integer from the given string.  If it parses okay,
    // store it in *result and return TRUE; otherwise, return FALSE.
    bool
    parse_num(const s8 *string, s64 *result)
    {
        ssize_t value;
        s8     *excess;

        // Clear the error number so that we can check it afterward.
        errno = 0;

        value = (ssize_t)strtol(string, &excess, 10);

        if (errno == ERANGE || *string == '\0' || *excess != '\0')
        {
            return false;
        }

        *result = value;

        return true;
    }
*/
bool
parseNum(STRING_VIEW string, long &result)
{
    char *end;
    errno      = 0;
    long value = constexpr_strtoll(&string[0], &end, 10);
    if (errno == ERANGE || *end != '\0' || string[0] == '\0')
    {
        return false;
    }
    result = value;
    return true;
}

//
//  Read one number (or two numbers separated by comma, period, or colon)
//  from the given string and store the number(s) in *line (and *column).
//  Return FALSE on a failed parsing, and TRUE otherwise.
//
bool
parse_line_column(const char *string, long *line, long *column)
{
    PROFILE_FUNCTION;
    const char *comma;
    char       *firstpart;
    bool        retval;
    while (*string == ' ')
    {
        string++;
    }
    comma = constexpr_strpbrk(string, ",.:");
    if (comma == nullptr)
    {
        return parseNum(string, *line);
    }
    retval = parseNum(comma + 1, *column);
    if (comma == string)
    {
        return retval;
    }
    firstpart                 = copy_of(string);
    firstpart[comma - string] = '\0';
    retval                    = parseNum(firstpart, *line) && retval;
    free(firstpart);
    return retval;
}

//
//  In the given string, recode each embedded NUL as a newline.
//
void
recode_NUL_to_LF(char *string, size_t length)
{
    while (length > 0)
    {
        if (*string == '\0')
        {
            *string = '\n';
        }
        length--;
        string++;
    }
}

//
//  In the given string, recode each embedded newline as a NUL,
//  and return the number of bytes in the string.
//
size_t
recode_LF_to_NUL(char *string)
{
    char *beginning = string;
    while (*string != '\0')
    {
        if (*string == '\n')
        {
            *string = '\0';
        }
        string++;
    }
    return (size_t)(string - beginning);
}

//
//  Free the memory of the given array, which should contain len elements.
//
void
free_chararray(char **array, size_t len)
{
    if (array == nullptr)
    {
        return;
    }
    while (len > 0)
    {
        free(array[--len]);
    }
    free(array);
}

//
//  Is the word starting at the given position in 'text' and of the given
//  length a separate word?  That is: is it not part of a longer word?
//
bool
is_separate_word(size_t position, size_t length, const char *text)
{
    const char *before = text + step_left(text, position);
    const char *after  = text + position + length;
    //
    //  If the word starts at the beginning of the line OR the character before
    //  the word isn't a letter, and if the word ends at the end of the line OR
    //  the character after the word isn't a letter, we have a whole word.
    //
    return ((position == 0 || !is_alpha_char(before)) && (*after == '\0' || !is_alpha_char(after)));
}

//
//  constexpr auto ERROR_MSG_OUT_OF_MEMORY = "Out of memory";
//
//  Return the position of the needle in the haystack, or NULL if not found.
//  When searching backwards, we will find the last match that starts no later
//  than the given start; otherwise, we find the first match starting no earlier
//  than start.  If we are doing a regexp search, and we find a match, we fill
//  in the global variable regmatches with at most 9 subexpression matches.
//
const char *
strstrwrapper(const char *const haystack, const char *const needle, const char *const start)
{
    PROFILE_FUNCTION;
    if ISSET (USE_REGEXP)
    {
        if ISSET (BACKWARDS_SEARCH)
        {
            size_t last_find, ceiling, far_end, floor, next_rung;
            //
            //  The start of the search range,
            //  and the next start.
            //
            floor = 0, next_rung = 0;
            if (regexec(&search_regexp, haystack, 1, regmatches, 0))
            {
                return nullptr;
            }
            far_end   = constexpr_strlen(haystack);
            ceiling   = start - haystack;
            last_find = regmatches[0].rm_so;
            //
            //  A result beyond the search range also means: no match.
            //
            if (last_find > ceiling)
            {
                return nullptr;
            }
            //
            //  Move the start-of-range forward until there is no more match;
            //  then the last match found is the first match backwards.
            //
            while (regmatches[0].rm_so <= ceiling)
            {
                floor     = next_rung;
                last_find = regmatches[0].rm_so;
                //
                //  If this is the last possible match, don't try to advance.
                //
                if (last_find == ceiling)
                {
                    break;
                }
                next_rung           = step_right(haystack, last_find);
                regmatches[0].rm_so = next_rung;
                regmatches[0].rm_eo = far_end;
                if (regexec(&search_regexp, haystack, 1, regmatches, REG_STARTEND))
                {
                    break;
                }
            }
            //
            //  Find the last match again, to get possible submatches.
            //
            regmatches[0].rm_so = floor;
            regmatches[0].rm_eo = far_end;
            if (regexec(&search_regexp, haystack, 10, regmatches, REG_STARTEND))
            {
                return nullptr;
            }
            return haystack + regmatches[0].rm_so;
        }
        //
        //  Do a forward regex search from the starting point.
        //
        regmatches[0].rm_so = start - haystack;
        regmatches[0].rm_eo = constexpr_strlen(haystack);
        if (regexec(&search_regexp, haystack, 10, regmatches, REG_STARTEND))
        {
            return nullptr;
        }
        else
        {
            return haystack + regmatches[0].rm_so;
        }
    }
    if ISSET (CASE_SENSITIVE)
    {
        if ISSET (BACKWARDS_SEARCH)
        {
            return revstrstr(haystack, needle, start);
        }
        else
        {
            return constexpr_strstr(start, needle);
        }
    }
    if ISSET (BACKWARDS_SEARCH)
    {
        return mbrevstrcasestr(haystack, needle, start);
    }
    else
    {
        return mbstrcasestr(start, needle);
    }
}

//
//  Allocate the given amount of memory and return a pointer to it.
//
void *
nmalloc(const size_t howmuch)
{
    void *section = malloc(howmuch);
    if (section == nullptr)
    {
        die(_(ERROR_MSG_OUT_OF_MEMORY));
    }
    return section;
}

//
//  Reallocate the given section of memory to have the given size.
//
void *
nrealloc(void *section, const size_t howmuch)
{
    section = realloc(section, howmuch);
    if (section == nullptr)
    {
        die(_(ERROR_MSG_OUT_OF_MEMORY));
    }
    return section;
}

//
//  Return an appropriately reallocated dest string holding a copy of src.
//  Usage: "dest = mallocstrcpy(dest, src);".
//
char *
mallocstrcpy(char *dest, const char *src)
{
    const size_t count = constexpr_strlen(src) + 1;
    dest               = (char *)nrealloc(dest, count);
    constexpr_strncpy(dest, src, count);
    return dest;
}

//
//  Return an allocated copy of the first count characters
//  of the given string, and NUL-terminate the copy.
//
char *
measured_copy(const char *string, const size_t count)
{
    char *thecopy = (char *)nmalloc(count + 1);
    memcpy(thecopy, string, count);
    thecopy[count] = '\0';
    return thecopy;
}

//
//  Return an allocated copy of the given string.
//
s8 *
copy_of(C_s8 *string)
{
    return measured_copy(string, constexpr_strlen(string));
}

//
//  Free the string at dest and return the string at src.
//
s8 *
free_and_assign(s8 *dest, s8 *src)
{
    std::free(dest);
    return src;
}

//
//  When not softwrapping, nano scrolls the current line horizontally by
//  chunks ("pages").  Return the column number of the first character
//  displayed in the edit window when the cursor is at the given column.
//
u64
get_page_start(C_u64 column)
{
    if (column == 0 || column + 2 < editwincols || ISSET(SOFTWRAP))
    {
        return 0;
    }
    else if (editwincols > 8)
    {
        return column - 6 - (column - 6) % (editwincols - 8);
    }
    else
    {
        return column - (editwincols - 2);
    }
}

//
//  Return the placewewant associated with current_x,
//  i.e. the zero-based column position of the cursor.
//
u64
xplustabs()
{
    return wideness(openfile->current->data, openfile->current_x);
}

//
//  Return the index in text of the character that (when displayed) will
//  not overshoot the given column.
//
u64
actual_x(C_s8 *text, u64 column)
{
    //
    //  From where we start walking through the text.
    //
    const s8 *start = text;
    //
    //  The current accumulated span, in columns.
    //
    u64 width = 0;

    while (*text != '\0')
    {
        s32 charlen = advance_over(text, width);
        if (width > column)
        {
            break;
        }
        text += charlen;
    }

    return static_cast<u64>(text - start);
}

//
//  A strnlen() with tabs and multicolumn characters factored in:
//  how many columns wide are the first maxlen bytes of text?
//
u64
wideness(C_s8 *text, u64 maxlen)
{
    if (maxlen == 0)
    {
        return 0;
    }

    u64 width = 0;
    while (*text != '\0')
    {
        u64 charlen = static_cast<u64>(advance_over(text, width));
        if (maxlen <= charlen)
        {
            break;
        }
        maxlen -= charlen;
        text += charlen;
    }
    return width;
}

//
//  Return the number of columns that the given text occupies.
//
u64
breadth(C_s8 *text)
{
    u64 span = 0;
    while (*text != '\0')
    {
        text += advance_over(text, span);
    }
    return span;
}

//
//  Append a new magic line to the end of the buffer.
//
void
new_magicline()
{
    openfile->filebot->next       = make_new_node(openfile->filebot);
    openfile->filebot->next->data = copy_of("");
    openfile->filebot             = openfile->filebot->next;
    openfile->totsize++;
}

//
//  Remove the magic line from the end of the buffer, if there is one and
//  it isn't the only line in the file.
//
void
remove_magicline()
{
    if (openfile->filebot->data[0] == '\0' && openfile->filebot != openfile->filetop)
    {
        if (openfile->current == openfile->filebot)
        {
            openfile->current = openfile->current->prev;
        }
        openfile->filebot = openfile->filebot->prev;
        delete_node(openfile->filebot->next);
        openfile->filebot->next = nullptr;
        openfile->totsize--;
    }
}

//
//  Return 'true' when the mark is before or at the cursor, and false otherwise.
//
bool
mark_is_before_cursor()
{
    return (openfile->mark->lineno < openfile->current->lineno ||
            (openfile->mark == openfile->current && openfile->mark_x <= openfile->current_x));
}

//
//  Return in (top, top_x) and (bot, bot_x) the start and end "coordinates"
//  of the marked region.
//
void
get_region(linestruct **top, unsigned long *top_x, linestruct **bot, unsigned long *bot_x)
{
    if (mark_is_before_cursor())
    {
        *top   = openfile->mark;
        *top_x = openfile->mark_x;
        *bot   = openfile->current;
        *bot_x = openfile->current_x;
    }
    else
    {
        *bot   = openfile->mark;
        *bot_x = openfile->mark_x;
        *top   = openfile->current;
        *top_x = openfile->current_x;
    }
}

//
//  Get the set of lines to work on -- either just the current line, or the
//  first to last lines of the marked region.  When the cursor (or mark) is
//  at the start of the last line of the region, exclude that line.
//
void
get_range(linestruct **top, linestruct **bot)
{
    if (!openfile->mark)
    {
        *top = openfile->current;
        *bot = openfile->current;
    }
    else
    {
        unsigned long top_x, bot_x;
        get_region(top, &top_x, bot, &bot_x);
        if (bot_x == 0 && bot != top && !also_the_last)
        {
            *bot = (*bot)->prev;
        }
        else
        {
            also_the_last = true;
        }
    }
}

//
//  Return a pointer to the line that has the given line number.
//
linestruct *
line_from_number(long number)
{
    linestruct *line = openfile->current;
    if (line->lineno > number)
    {
        while (line->lineno != number)
        {
            line = line->prev;
        }
    }
    else
    {
        while (line->lineno != number)
        {
            line = line->next;
        }
    }
    return line;
}

//
//  Count the number of characters from begin to end, and return it.
//
unsigned long
number_of_characters_in(const linestruct *begin, const linestruct *end)
{
    const linestruct *line;
    unsigned long     count = 0;
    //
    //  Sum the number of characters (plus a newline) in each line.
    //
    for (line = begin; line != end->next; line = line->next)
    {
        count += mbstrlen(line->data) + 1;
    }
    //
    //  Do not count the final newline.
    //
    return (count - 1);
}
