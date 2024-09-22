#include "../include/c_prototypes.h"

/**
 * Stuff that needs to be shared with the rest of indent. Documented in indent.h.
 */
char *in_prog_pos = NULL;  /* Used in output.c        code_io.c indent.c                      */
char *buf_ptr     = NULL;  /* Used in output.c lexi.c code_io.c indent.c comments.c           */
char *buf_end     = NULL;  /* Used in output.c lexi.c code_io.c indent.c comments.c           */
bool  had_eof     = false; /* Used in output.c        code_io.c          comments.c parse.c   */
char *cur_line    = NULL;  /* Used in output.c        code_io.c                               */

extern char *
skip_horiz_space(const char *p)
{
    while ((*p == ' ') || (*p == TAB))
    {
        p++;
    }
    return (char *)p;
}

/******************************************************************************/

extern void
skip_buffered_space(void)
{
    while ((*buf_ptr == ' ') || (*buf_ptr == TAB))
    {
        buf_ptr++;

        if (buf_ptr >= buf_end)
        {
            fill_buffer();
        }
    }
}

static bool
is_comment_start(const char *p)
{
    if ((*p == '/') && ((*(p + 1) == '*') || (*(p + 1) == '/')))
    {
        return true;
    }
    return false;
}

/**
 * Return the column we are at in the input line.
 */
extern int
current_column(void)
{
    char *p;
    int   column;
    /* Use save_com.size here instead of save_com.end, because save_com is already emptied at this point. */
    if ((buf_ptr >= save_com.ptr) && (buf_ptr <= save_com.ptr + save_com.len))
    {
        p      = save_com.ptr;
        column = save_com.start_column;
    }
    else
    {
        p      = cur_line;
        column = 1;
    }
    while (p < buf_ptr)
    {
        switch (*p)
        {
            case EOL :
            case 014 : /* Form feed. */
                column = 1;
                break;

            case TAB :
                column += settings.tabsize - (column - 1) % settings.tabsize;
                break;

            case '\b' : /* Backspace. */
                column--;
                break;

            default :
                column++;
                break;
        }
        p++;
    }
    return column;
}

/**
 * Read file FILENAME into a `fileptr' structure, and return a pointer to that structure.
 */
extern file_buffer_t *
read_file(char *filename, struct stat *file_stats)
{
    static file_buffer_t fileptr = {NULL, 0, NULL};
#if defined(__MSDOS__)
    /**
     * Size is required to be unsigned for 'MSDOS',
     * in order to read files larger than 32767
     * bytes in a 16-bit world...
     */
    unsigned int size = 0, size_to_read = 0;
#else
    ssize_t size         = 0;
    size_t  size_to_read = 0;
#endif
    unsigned int namelen = strlen(filename);
    int          fd      = open(filename, O_RDONLY, 0777);
    if (fd < 0)
    {
        fatal("Can't open input file %s", filename);
    }
    if (fstat(fd, file_stats) < 0)
    {
        fatal("Can't stat input file %s", filename);
    }
    if (file_stats->st_size == 0)
    {
        ERROR("Zero-length file %s", filename, 0);
    }
#if !defined(__DJGPP__)
    /* Old 'MSDOS'. */
    if (sizeof(int) == 2)
    {
        if ((file_stats->st_size < 0) || (file_stats->st_size > (0xffff - 1)))
        {
            fatal("File %s is too big to read", filename);
        }
    }
    else
#endif
    {
        if (file_stats->st_size < 0)
        {
            fatal("System problem reading file %s", filename);
        }
    }
    if ((size_t)file_stats->st_size >= SSIZE_MAX)
    {
        fatal("File %s is too big to read", filename);
    }
    fileptr.size = file_stats->st_size;
    if (fileptr.data != 0)
    {
        /* Add 1 for '\0' and 1 for potential final added newline. */
        fileptr.data = xrealloc(fileptr.data, (unsigned)file_stats->st_size + 2);
    }
    else
    {
        /* Add 1 for '\0' and 1 for potential final added newline. */
        fileptr.data = xmalloc((unsigned)file_stats->st_size + 2);
    }
    size_to_read = fileptr.size;
    while (size_to_read > 0)
    {
        size = read(fd, fileptr.data + fileptr.size - size_to_read, size_to_read);
        if (size ==
#if defined(__MSDOS__)
            (unsigned int)
#endif
            -1)
        {
#if !defined(__MSDOS__)
            if (errno == EINTR)
            {
                continue;
            }
#endif
            xfree(fileptr.data);
            fatal("Error reading input file %s", filename);
        }
        size_to_read -= size;
    }
    if (close(fd) < 0)
    {
        xfree(fileptr.data);
        fatal("Error closing input file %s", filename);
    }
    /**
     * Apparently, the DOS stores files using CR-LF for newlines, but
     * then the DOS `read' changes them into '\n'.  Thus, the size of the
     * file on disc is larger than what is read into memory.  Thanks, Bill.
     */
    if ((size_t)size < fileptr.size)
    {
        fileptr.size = size;
    }
    if (fileptr.name != NULL)
    {
        fileptr.name = xrealloc(fileptr.name, (unsigned)namelen + 1);
    }
    else
    {
        fileptr.name = xmalloc(namelen + 1);
    }
    memcpy(fileptr.name, filename, namelen);
    fileptr.name[namelen] = EOS;
    if ((fileptr.size > 0) && (fileptr.data[fileptr.size - 1] != EOL))
    {
        fileptr.data[fileptr.size] = EOL;
        fileptr.size++;
    }
    fileptr.data[fileptr.size] = EOS;
    return &fileptr;
}

/**
 * This should come from stdio.h and be some system-optimal number
 */
#ifndef BUFSIZ
#    define BUFSIZ 1024
#endif

/* Suck the standard input into a file_buffer structure, and return a pointer to that structure. */
file_buffer_t *
read_stdin(void)
{
    static file_buffer_t stdinptr = {NULL, 0, NULL};
    unsigned int          size     = 15 * BUFSIZ;
    int                   ch       = EOF;
    char                 *p        = NULL;
    if (stdinptr.data != 0)
    {
        free(stdinptr.data);
    }
    stdinptr.data = xmalloc(size + 1);
    stdinptr.size = 0;
    p             = stdinptr.data;
    do
    {
        while (stdinptr.size < size)
        {
            ch = getc(stdin);
            if (ch == EOF)
            {
                break;
            }
            *p++ = ch;
            stdinptr.size++;
        }
        if (ch != EOF)
        {
            size += (2 * BUFSIZ);
            stdinptr.data = xrealloc(stdinptr.data, (unsigned)size);
            p             = stdinptr.data + stdinptr.size;
        }
    }
    while (ch != EOF);
    stdinptr.name                = "Standard Input";
    stdinptr.data[stdinptr.size] = EOS;
    return &stdinptr;
}

/*
 * Advance `buf_ptr' so that it points to the next line of input.
 *
 * If the next input line contains an indent control comment turning
 * off formatting (a comment, C or C++, beginning with *INDENT-OFF*),
 * disable indenting by calling inhibit_indenting() which will cause
 * `dump_line ()' to simply print out all input lines without formatting
 * until it finds a corresponding comment containing *INDENT-0N* which
 * re-enables formatting.
 *
 * Note that if this is a C comment we do not look for the closing
 * delimiter.  Note also that older versions of this program also
 * skipped lines containing *INDENT** which represented errors
 * generated by indent in some previous formatting.  This version does
 * not recognize such lines.
 */
void
fill_buffer(void)
{
    char   *p               = NULL;
    bool finished_a_line = false;
    /* indent() may be saving the text between "if (...)" and the following
     * statement.  To do so, it uses another buffer (`save_com').  Switch
     * back to the previous buffer here. */
    if (bp_save != 0)
    {
        buf_ptr = bp_save;
        buf_end = be_save;
        bp_save = be_save = 0;
        /* only return if there is really something in this buffer */
        if (buf_ptr < buf_end)
        {
            return;
        }
    }
    if (*in_prog_pos == EOS)
    {
        buf_ptr  = in_prog_pos;
        cur_line = buf_ptr;
        had_eof  = true;
    }
    else
    {
        /* Here if we know there are chars to read.  The file is NULL-terminated,
         * so we can always look one character ahead safely. */
        cur_line        = in_prog_pos;
        p               = cur_line;
        finished_a_line = false;
        do
        {
            p = skip_horiz_space(p);
            /* If we are looking at the beginning of a comment, see
             * if it turns off formatting with off-on directives. */
            if (is_comment_start(p))
            {
                p += 2;
                p = skip_horiz_space(p);
                /* Skip all lines between the indent off and on directives. */
                if (strncmp(p, "*INDENT-OFF*", 12) == 0)
                {
                    inhibit_indenting(true);
                }
            }
            while ((*p != EOS) && *p != EOL)
            {
                p++;
            }
            /* Here for newline -- finish up unless formatting is off */
            if (*p == EOL)
            {
                finished_a_line = true;
                in_prog_pos     = p + 1;
            }
            /* Here for embedded NULLs */
            else if ((unsigned int)(p - current_input->data) < current_input->size)
            {
                fatal("File %s contains NULL-characters: cannot proceed\n", current_input->name);
            }
            /* Here for EOF with no terminating newline char. */
            else
            {
                in_prog_pos     = p;
                finished_a_line = true;
            }
        }
        while (!finished_a_line);
        buf_ptr = cur_line;
        buf_end = in_prog_pos;
    }
}
