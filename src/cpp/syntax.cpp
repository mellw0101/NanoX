#include "../include/definitions.h"
#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <fcntl.h>
#include <stdio.h>

/* Function to check syntax for a open buffer. */
void
syntax_check_file(openfilestruct *file)
{
    PROFILE_FUNCTION;
    if (openfile->filetop->next == NULL)
    {
        return;
    }
    if (!ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX))
    {
        const char *fext = get_file_extention();
        if (fext &&
            (strncmp(fext, "cpp", 3) == 0 || strncmp(fext, "c", 1) == 0))
        {
            for (linestruct *line = file->filetop; line != NULL;
                 line             = line->next)
            {
                check_for_syntax_words(line);
            }
            prosses_callback_queue();
        }
    }
    /* else
    {
        check_include_file_syntax("/usr/include/Mlib/Packy.h");
        check_include_file_syntax("/usr/include/stdio.h");
        check_include_file_syntax("/usr/include/stdlib.h");
    } */
    char *type;
    char *name;
    char *value;
    parse_variable("        text++", &type, &name, &value);
    if (type)
    {
        nlog("type: %s\n", type);
        free(type);
    }
    if (name)
    {
        nlog("name: %s\n", name);
        free(name);
    }
    if (value)
    {
        nlog("value: %s\n", value);
        free(value);
    }
}

bool
parse_color_opts(const char *color_fg, const char *color_bg, short *fg,
                 short *bg, int *attr)
{
    bool vivid, thick;
    *attr = A_NORMAL;
    if (color_fg != NULL)
    {
        if (strncmp(color_fg, "bold", 4) == 0)
        {
            *attr |= A_BOLD, color_fg += 5;
        }
        if (strncmp(color_fg, "italic", 6) == 0)
        {
            *attr |= A_ITALIC, color_fg += 7;
        }
        *fg = color_to_short(color_fg, vivid, thick);
        if (*fg == BAD_COLOR)
        {
            return FALSE;
        }
        if (vivid && !thick && COLORS > 8)
        {
            fg += 8;
        }
        else if (vivid)
        {
            *attr |= A_BOLD;
        }
    }
    else
    {
        *fg = THE_DEFAULT;
    }
    if (color_bg != NULL)
    {
        if (strncmp(color_bg, "bold", 4) == 0)
        {
            *attr |= A_BOLD;
            color_bg += 5;
        }
        if (strncmp(color_bg, "italic", 6) == 0)
        {
            *attr |= A_ITALIC;
            color_bg += 7;
        }
        *bg = color_to_short(color_bg, vivid, thick);
        if (*bg == BAD_COLOR)
        {
            return FALSE;
        }
        if (vivid && COLORS > 8)
        {
            bg += 8;
        }
    }
    else
    {
        *bg = THE_DEFAULT;
    }
    return TRUE;
}

/* Add syntax regex to end of colortype list 'c'.  This need`s
 * to be faster.  Currently this takes around 0.4 ms. */
void
add_syntax_color(const char *color_fg, const char *color_bg, const char *rgxstr,
                 colortype **c, const char *from_file)
{
    PROFILE_FUNCTION;
    if (*c == NULL)
    {
        LOUT_logE("c == NULL.");
        return;
    }
    short      fg, bg;
    int        attr;
    regex_t   *start_rgx = NULL;
    colortype *nc        = NULL;
    if (!parse_color_opts(color_fg, color_bg, &fg, &bg, &attr))
    {
        logE("'parse_color_opts' Failed.");
        return;
    }
    if (from_file != NULL)
    {
        if (!compile_with_callback(
                rgxstr, NANO_REG_EXTENDED, &start_rgx, from_file))
        {
            return;
        }
    }
    else
    {
        if (!compile(rgxstr, NANO_REG_EXTENDED, &start_rgx))
        {
            return;
        }
    }
    nc             = (colortype *)nmalloc(sizeof(colortype));
    nc->start      = start_rgx;
    nc->end        = NULL;
    nc->fg         = fg;
    nc->bg         = bg;
    nc->attributes = attr;
    nc->pairnum    = (*c)->pairnum + 1;
    nc->next       = NULL;
    (*c)->next     = nc;
    (*c)           = (*c)->next;
}

void
add_start_end_syntax(const char *color_fg, const char *color_bg,
                     const char *start, const char *end, colortype **c)
{
    if (*c == NULL)
    {
        return;
    }
    short      fg, bg;
    int        attr;
    regex_t   *start_rgx = NULL, *end_rgx = NULL;
    colortype *nc;
    if (!parse_color_opts(color_fg, color_bg, &fg, &bg, &attr))
    {
        return;
    }
    if (!compile(start, NANO_REG_EXTENDED, &start_rgx))
    {
        return;
    }
    if (!compile(end, NANO_REG_EXTENDED, &end_rgx))
    {
        regfree(start_rgx);
        free(start_rgx);
        return;
    }
    nc             = (colortype *)nmalloc(sizeof(colortype));
    nc->start      = start_rgx;
    nc->end        = end_rgx;
    nc->fg         = fg;
    nc->bg         = bg;
    nc->attributes = attr;
    nc->pairnum    = (*c)->pairnum + 1;
    nc->next       = NULL;
    (*c)->next     = nc;
    (*c)           = (*c)->next;
}

bool
check_func_syntax(char ***words, unsigned long *i)
{
    unsigned long  at   = 0;
    unsigned short type = 0;
    if ((*words)[++(*i)] == NULL)
    {
        return FALSE;
    }
    /* Remove all if any preceding '*' char`s. */
    while (*((*words)[*i]) == '*')
    {
        (*words)[*i] += 1;
    }
    if (is_word_func((*words)[*i], &at))
    {
        if (!syntax_func((*words)[*i]))
        {
            new_syntax_func((*words)[*i]);
            // add_syntax_word(FUNC_COLOR, NULL, rgx_word((*words)[*i]));
            // sub_thread_compile_add_rgx(FUNC_COLOR, NULL,
            // rgx_word((*words)[*i]), &last_c_color);
        }
        (*words)[*i] += at + 1;
        type = retrieve_c_syntax_type((*words)[*i]);
        if (type)
        {
            if ((*words)[++(*i)] != NULL)
            {
                add_syntax(&type, (*words)[*i]);
            }
        }
        return TRUE;
    }
    if ((*words)[(*i) + 1] != NULL)
    {
        if (*((*words)[(*i) + 1]) == '(')
        {
            if (!syntax_func((*words)[*i]))
            {
                new_syntax_func((*words)[*i]);
                // add_syntax_word(FUNC_COLOR, NULL, rgx_word((*words)[*i]));
                // sub_thread_compile_add_rgx(FUNC_COLOR, NULL,
                // rgx_word((*words)[*i]), &last_c_color);
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* Check a file for syntax, and add relevent syntax. */
void
check_syntax(const char *path)
{
    if (!is_file_and_exists(path))
    {
        return;
    }
    char         *buf = NULL, **words;
    unsigned long size, len, i;
    FILE         *f = fopen(path, "rb");
    if (f == NULL)
    {
        return;
    }
    while ((len = getline(&buf, &size, f)) != EOF)
    {
        if (buf[len - 1] == '\n')
        {
            buf[--len] = '\0';
        }
        words = words_in_str(buf);
        if (*words == NULL)
        {
            continue;
        }
        for (i = 0; words[i] != NULL; i++)
        {
            const unsigned short type = retrieve_c_syntax_type(words[i]);
            if (words[i + 1] == NULL)
            {
                break;
            }
            if (!type)
            {
                continue;
            }
            else if (type & CS_STRUCT)
            {
                if (*words[i + 1] == '{' || *words[i + 1] == '*')
                    ;
                else if (!is_syntax_struct(words[++i]))
                {
                    // sub_thread_compile_add_rgx("brightgreen", NULL,
                    // rgx_word(words[i]), &last_c_color);
                }
            }
            else if (type & CS_CLASS)
            {
                if (*words[i + 1] == '{')
                    ;
                else if (!is_syntax_class(words[++i]))
                {
                    // sub_thread_compile_add_rgx("brightgreen", NULL,
                    // rgx_word(words[i]), &last_c_color);
                }
            }
            else if (type & CS_ENUM)
            {
                /* If this is a anonumus enum skip, (will be implemented later).
                 */
                if (*words[i + 1] == '{')
                    ;
                else
                {
                    // sub_thread_compile_add_rgx("brightgreen", NULL,
                    // rgx_word(words[++i]), &last_c_color);
                }
            }
            else if (type & CS_CHAR || type & CS_VOID || type & CS_INT ||
                     type & CS_LONG || type & CS_BOOL || type & CS_SIZE_T ||
                     type & CS_SSIZE_T)
            {
                if (check_func_syntax(&words, &i))
                {
                    continue;
                }
            }
            else if (type & CS_DEFINE)
            {
                handle_define(words[++i]);
            }
            /* TODO: Fix check_multis before. */
            // else if (type & CS_INCLUDE)
            // {
            //     /* If the include file is a 'local' file, then base
            //      * the full path on current path. */
            //     if (*words[++i] == '"')
            //     {
            //         char         *rpath = strdup(path);
            //         unsigned long j = 0, pos = 0;
            //         for (; rpath[j]; j++)
            //         {
            //             if (rpath[j] == '/')
            //             {
            //                 pos = j;
            //             }
            //         }
            //         if (pos != 0)
            //         {
            //             rpath[pos] = '\0';
            //         }
            //         const char *full_rpath = concat_path(rpath,
            //         extract_include(words[i])); if
            //         (is_file_and_exists(full_rpath))
            //         {
            //             if (!is_in_handled_includes_vec(full_rpath))
            //             {
            //                 add_to_handled_includes_vec(full_rpath);
            //                 check_syntax(full_rpath);
            //             }
            //         }
            //         free(rpath);
            //     }
            //     else if (*words[i] == '<')
            //     {
            //         const char *rpath = concat_path("/usr/include/",
            //         extract_include(words[i])); if
            //         (is_file_and_exists(rpath))
            //         {
            //             if (!is_in_handled_includes_vec(rpath))
            //             {
            //                 add_to_handled_includes_vec(rpath);
            //                 check_syntax(rpath);
            //             }
            //             continue;
            //         }
            //         rpath = concat_path("/usr/include/c++/v1/",
            //         extract_include(words[i])); if
            //         (is_file_and_exists(rpath))
            //         {
            //             if (!is_in_handled_includes_vec(rpath))
            //             {
            //                 add_to_handled_includes_vec(rpath);
            //                 check_syntax(rpath);
            //             }
            //         }
            //     }
            // }
        }
        prosses_callback_queue();
        free(words);
    }
    fclose(f);
}

void
check_include_file_syntax(const char *path)
{
    if (!is_file_and_exists(path))
    {
        return;
    }
    PROFILE_FUNCTION;
    unsigned int  type;
    unsigned      i;
    unsigned long nwords;
    char        **words = words_from_file(path, &nwords);
    if (words == NULL)
    {
        return;
    }
    bool in_comment = FALSE;
    for (i = 0; i < nwords; i++)
    {
        if (*words[i] == '#')
        {
            if (words[i][1] == '\0')
            {
                ++i;
            }
            else
            {
                unsigned long slen = strlen(words[i]) - 1;
                memmove(words[i], words[i] + 1, slen);
                words[i][slen] = '\0';
            }
        }
        if (strcmp(words[i], "/*") == 0)
        {
            in_comment = TRUE;
        }
        else if (strcmp(words[i], "*/") == 0)
        {
            in_comment = FALSE;
        }
        if (in_comment)
        {
            continue;
        }
        type = retrieve_c_syntax_type(words[i]);
        if (!type)
        {
            continue;
        }
        else if (type & CS_STRUCT)
        {
            if (words[++i] != NULL)
            {
                if (*words[i] == '{' || *words[i] == '*')
                {
                    continue;
                }
                else if (!is_syntax_struct(words[i]))
                {
                    char *struct_str      = copy_of(words[i]);
                    color_map[struct_str] = FG_VS_CODE_BRIGHT_GREEN;
                    structs.push_back(struct_str);
                }
            }
        }
        else if (type & CS_CLASS)
        {
            if (words[++i] != NULL)
            {
                if (*words[i] == '{' || *words[i] == '*')
                {
                    continue;
                }
                else if (!is_syntax_class(words[i]))
                {
                    char *class_str      = copy_of(words[i]);
                    color_map[class_str] = FG_VS_CODE_BRIGHT_GREEN;
                    classes.push_back(class_str);
                }
            }
        }
        /* else if (type & CS_ENUM)
        {
            if (words[++i] != NULL)
            {
                if (*words[i] == '{' || *words[i] == '*')
                {
                    continue;
                }
                add_syntax_word(STRUCT_COLOR, NULL, rgx_word(words[i]));
            }
        } */
        else if (type & CS_CHAR || type & CS_VOID || type & CS_INT ||
                 type & CS_LONG || type & CS_BOOL || type & CS_SIZE_T ||
                 type & CS_SSIZE_T)
        {
            if (words[++i] != NULL)
            {
                unsigned long at;
                if (char_is_in_word(words[i], '(', &at))
                {
                    if (at != 0)
                    {
                        words[i][at] = '\0';
                        if (!char_is_in_word(words[i], '=', &at))
                        {
                            strip_leading_chars_from(words[i], '*');
                            if (words[i] != NULL)
                            {
                                if (!syntax_func(words[i]))
                                {
                                    char *func_str = copy_of(words[i]);
                                    color_map[func_str] =
                                        FG_VS_CODE_BRIGHT_YELLOW;
                                    funcs.push_back(func_str);
                                }
                            }
                        }
                    }
                }
                else if (words[i + 1] && *words[i + 1] == '(')
                {
                    strip_leading_chars_from(words[i], '*');
                    if (!syntax_func(words[i]))
                    {
                        char *func_str      = copy_of(words[i]);
                        color_map[func_str] = FG_VS_CODE_BRIGHT_YELLOW;
                        funcs.push_back(func_str);
                    }
                }
            }
        }
        else if (type & CS_DEFINE)
        {
            if (words[++i] != NULL)
            {
                handle_define(words[i]);
            }
        }
    }
    for (i = 0; i < nwords; i++)
    {
        free(words[i]);
    }
    free(words);
}

int
add_syntax(const unsigned short *type, char *word)
{
    if (*type & CS_STRUCT || *type & CS_ENUM || *type & CS_CLASS)
    {
        // sub_thread_compile_add_rgx("brightgreen", NULL, rgx_word(word),
        // &last_c_color);
    }
    else if (*type & CS_INT || *type & CS_VOID || *type & CS_LONG ||
             *type & CS_CHAR || *type & CS_BOOL || *type & CS_SIZE_T ||
             *type & CS_SSIZE_T || *type & CS_SHORT)
    {
        /* Remove all if any '*' char`s */
        while (word[0] == '*')
        {
            word += 1;
        }
        unsigned int i = 0;
        for (i = 0; word[i]; i++)
        {
            if (word[i] == ',')
            {
                word[i] = '\0';
                if (!syntax_var(word))
                {
                    new_syntax_var(word);
                    // sub_thread_compile_add_rgx(VAR_COLOR, NULL,
                    // rgx_word(word), &last_c_color);
                }
                return NEXT_WORD_ALSO;
            }
            if (word[i] == ';' || word[i] == ')')
            {
                word[i] = '\0';
                break;
            }
        }
        if (!syntax_var(word))
        {
            new_syntax_var(word);
            // sub_thread_compile_add_rgx(VAR_COLOR, NULL, rgx_word(word),
            // &last_c_color);
        }
    }
    return 0;
}

void
handle_include(char *str)
{
    static char        buf[PATH_MAX];
    static const char *pwd          = NULL;
    char              *current_file = NULL;
    unsigned long      i, pos;
    if (str == NULL)
    {
        LOUT_logE("str == NULL.");
        return;
    }
    if (*str == '<')
    {
        sprintf(buf, "%s%s", "/usr/include/", extract_include(str));
        if (is_file_and_exists(buf))
        {
            if (!is_in_handled_includes_vec(buf))
            {
                add_to_handled_includes_vec(buf);
                check_syntax(buf);
            }
            return;
        }
        str += 1;
        if (*str == 'c')
        {
            str += 1;
        }
        memset(buf, '\0', sizeof(buf));
        sprintf(buf, "%s%s", "/usr/include/c++/v1/", str);
        if (is_file_and_exists(str))
        {
            if (!is_in_handled_includes_vec(buf))
            {
                add_to_handled_includes_vec(buf);
                check_syntax(buf);
            }
            return;
        }
        memset(buf, '\0', sizeof(buf));
        sprintf(buf, "%s%s%s", "/usr/include/c++/v1/", str, ".h");
        if (is_file_and_exists(buf))
        {
            if (!is_in_handled_includes_vec(buf))
            {
                add_to_handled_includes_vec(buf);
                check_syntax(buf);
            }
            return;
        }
    }
    else if (*str == '"')
    {
        pwd = getenv("PWD");
        if (pwd == NULL)
        {
            return;
        }
        current_file = strdup(openfile->filename);
        for (i = 0, pos = 0; current_file[i]; i++)
        {
            if (current_file[i] == '/')
            {
                pos = i;
            }
        }
        if (pos != 0)
        {
            current_file[pos + 1] = '\0';
        }
        unsigned long slen = strlen(str) - 2;
        memmove(str, str + 1, slen);
        str[slen] = '\0';
        sprintf(buf, "%s%s%s%s", pwd, "/",
                (current_file != NULL) ? current_file : "", str);
        if (is_file_and_exists(buf))
        {
            if (!is_in_handled_includes_vec(buf))
            {
                add_to_handled_includes_vec(buf);
                check_syntax(buf);
            }
        }
    }
}

/* Add a '#define' to syntax. */
void
handle_define(char *str)
{
    unsigned int i;
    if (*str == '\\')
    {
        return;
    }
    for (i = 0; str[i]; i++)
    {
        if (str[i] == '(')
        {
            str[i] = '\0';
            break;
        }
    }
    if (!define_exists(str))
    {
        char *define_str      = copy_of(str);
        color_map[define_str] = FG_VS_CODE_BLUE;
        funcs.push_back(define_str);
        defines.push_back(define_str);
    }
}

/* This keeps track of the last type when a type is descovered and there is no
 * next word. */
static unsigned short last_type = 0;
/* Check a line for syntax words, also index files that are included and add
 * functions as well. */
void
check_for_syntax_words(linestruct *line)
{
    unsigned long i;
    char        **words;
    if (is_empty_line(line))
    {
        return;
    }
    words = words_in_str(line->data);
    if (words == NULL)
    {
        return;
    }
    for (i = 0; words[i] != NULL; i++)
    {
        if (is_syntax_struct(words[i]))
        {
            if (words[i + 1] != NULL)
            {
                if (*words[i + 1] == '(')
                    ;
                else
                {
                    handle_struct_syntax(&words[i + 1]);
                    if (!syntax_var(words[++i]))
                    {
                        new_syntax_var(words[i]);
                        // sub_thread_compile_add_rgx(VAR_COLOR, NULL,
                        // rgx_word(words[i]), &last_c_color);
                    }
                }
            }
        }
        else if (is_syntax_class(words[i]))
        {
            if (words[i + 1] != NULL)
            {
                handle_struct_syntax(&words[i + 1]);
                if (!syntax_var(words[i]))
                {
                    new_syntax_var(words[i]);
                    // sub_thread_compile_add_rgx(VAR_COLOR, NULL,
                    // rgx_word(words[++i]), &last_c_color);
                }
            }
        }
        const unsigned short type = retrieve_c_syntax_type(words[i]);
        if (last_type != 0)
        {
            if (last_type & CS_VOID || last_type & CS_INT ||
                last_type & CS_CHAR || last_type & CS_LONG ||
                last_type & CS_BOOL || last_type & CS_SIZE_T ||
                last_type & CS_SSIZE_T || last_type & CS_SHORT)
            {
                unsigned int j;
                for (j = 0; (words[i])[j]; j++)
                {
                    if ((words[i])[j] == '(')
                    {
                        (words[i])[j] = '\0';
                        break;
                    }
                }
                if (!syntax_func(words[i]))
                {
                    new_syntax_func(words[i]);
                    // sub_thread_compile_add_rgx(FUNC_COLOR, NULL,
                    // rgx_word(words[i]), &last_c_color);
                }
                words[i] += j + 1;
                --i;
                last_type = 0;
                continue;
            }
        }
        if (words[i + 1] == NULL)
        {
            last_type = type;
            break;
        }
        if (!type)
        {
            continue;
        }
        else if (type & CS_STRUCT)
        {
            if (!is_syntax_struct(words[++i]))
            {
                // sub_thread_compile_add_rgx(STRUCT_COLOR, NULL,
                // rgx_word(words[i]), &last_c_color);
            }
        }
        else if (type & CS_CLASS)
        {
            if (!is_syntax_class(words[++i]))
            {
                add_syntax(&type, words[i]);
                // sub_thread_compile_add_rgx(STRUCT_COLOR, NULL,
                // rgx_word(words[i]), &last_c_color);
            }
        }
        else if (type & CS_ENUM)
        {}
        else if (type & CS_LONG || type & CS_VOID || type & CS_INT ||
                 type & CS_CHAR || type & CS_BOOL || type & CS_SIZE_T ||
                 type & CS_SSIZE_T || type & CS_SHORT)
        {
            if (check_func_syntax(&words, &i))
            {
                continue;
            }
            if (add_syntax(&type, words[i]) == NEXT_WORD_ALSO)
            {
                while (TRUE)
                {
                    if (words[++i] == NULL)
                    {
                        break;
                    }
                    if (add_syntax(&type, words[i]) != NEXT_WORD_ALSO)
                    {
                        break;
                    }
                }
                continue;
            }
        }
        else if (type & CS_INCLUDE)
        {
            handle_include(words[++i]);
        }
        else if (type & CS_DEFINE)
        {
            handle_define(words[++i]);
        }
    }
    prosses_callback_queue();
    free(words);
}

/* Add some "basic" cpp syntax. */
void
do_cpp_syntax(void)
{
    PROFILE_FUNCTION;
    flag_all_brackets();
    flag_all_block_comments();
}

/* Return`s 'TRUE' if 'str' is in the 'syntax_structs' vector. */
bool
is_syntax_struct(std::string_view str)
{
    for (int i = 0; i < structs.get_size(); i++)
    {
        if (strcmp(&str[0], structs[i]) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool
is_syntax_class(std::string_view str)
{
    for (int i = 0; i < classes.get_size(); i++)
    {
        if (strcmp(&str[0], classes[i]) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool
define_exists(const char *str)
{
    for (int i = 0; i < defines.get_size(); i++)
    {
        if (strcmp(str, defines[i]) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void
handle_struct_syntax(char **word)
{
    unsigned long i;
    while (*(*word) == '*')
    {
        *word += 1;
    }
    for (i = 0; (*word)[i]; i++)
    {
        if ((*word)[i] == ';')
        {
            (*word)[i] = '\0';
            break;
        }
        if ((*word)[i] == ')')
        {
            (*word)[i] = '\0';
            break;
        }
    }
}

void
handle(char *word, const unsigned short type)
{
    if (word == NULL)
    {
        return;
    }
    strip_leading_chars_from(word, '*');
    unsigned long i = 0;
    for (; word[i]; i++)
    {
        if (word[i] == ';' || word[i] == ',' || word[i] == ')')
        {
            word[i] = '\0';
            break;
        }
    }
    if (type == 0)
    {
        if (!syntax_var(word))
        {
            new_syntax_var(word);
        }
    }
    else if (type & CS_DEFINE)
    {}
    else if (type & CS_STRUCT)
    {
        if (!is_syntax_struct(word))
        {}
    }
    else if (type & CS_CLASS)
    {
        if (!is_syntax_class(word))
        {}
    }
}

void
openfile_syntax_c(void)
{
    unsigned long i, nwords, at;
    char        **words = words_from_current_file(&nwords);
    if (words == NULL)
    {
        return;
    }
    /* Main loop to parse the syntax. */
    for (i = 0; i < nwords; i++)
    {
        if (*words[i] == '#')
        {
            if (words[i][1] == '\0')
            {
                ++i;
            }
            else
            {
                strip_leading_chars_from(words[i], '#');
            }
        }
        else if (is_syntax_struct(words[i]))
        {
            handle(words[++i], 0);
        }
        else if (is_syntax_class(words[i]))
        {
            handle(words[++i], 0);
        }
        const unsigned short type = retrieve_c_syntax_type(words[i]);
        if (type == 0)
        {
            continue;
        }
        else if (type & CS_DEFINE)
        {
            handle(words[++i], type);
        }
        else if (type & CS_STRUCT)
        {
            handle(words[++i], type);
        }
        else if (type & CS_CLASS)
        {
            handle(words[++i], type);
        }
        else if (type & CS_CHAR || type & CS_VOID || type & CS_INT ||
                 type & CS_LONG || type & CS_SHORT || type & CS_SIZE_T ||
                 type & CS_SSIZE_T)
        {
            if (words[++i])
            {
                if (char_is_in_word(words[i], '(', &at))
                {
                    if (at)
                    {
                        char *ptr    = copy_of(words[i]);
                        words[i][at] = '\0';
                        free(words[i]);
                        words[i--] =
                            measured_copy(ptr + at + 1, strlen(ptr) - at - 1);
                        free(ptr);
                    }
                }
                else if (words[i + 1] && *words[i + 1] == '(')
                {}
                else
                {
                    handle(words[i], 0);
                }
            }
        }
    }
    /* Loop to free all words, then we free the array. */
    for (i = 0; i < nwords; i++)
    {
        free(words[i]);
    }
    free(words);
}

void
find_block_comments(int from, int end)
{
    PROFILE_FUNCTION;
    linestruct *line = line_from_number(from);
    for (; line && line->lineno != end; line = line->next)
    {
        const char *found_start = strstr(line->data, "/*");
        const char *found_end   = strstr(line->data, "*/");
        if (found_start && found_end)
            ;
        else if (found_start && !found_end)
        {
            LINE_SET(line, BLOCK_COMMENT_START);
            continue;
        }
        else if (!found_start && found_end)
        {
            LINE_SET(line, BLOCK_COMMENT_END);
            continue;
        }
        else if (!found_start && !found_end)
        {
            if (line->prev != NULL &&
                (LINE_ISSET(line->prev, BLOCK_COMMENT_START) ||
                 LINE_ISSET(line->prev, IN_BLOCK_COMMENT)))
            {
                LINE_SET(line, IN_BLOCK_COMMENT);
            }
        }
    }
}

char **
find_functions_in_file(char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    long          len;
    unsigned long size;
    static char  *line;
    unsigned long acap = 10, asize = 0;
    char        **func_str_array = (char **)nmalloc(acap * sizeof(char *));
    while ((len = getline(&line, &size, file)) != EOF)
    {
        if (line[len - 1] == '\n')
        {
            line[--len] = '\0';
        }
        const char *start        = line;
        const char *end          = line;
        const char *parent_start = strchr(line, '(');
        if (parent_start)
        {
            start = parent_start;
            for (; start > line && *start != ' ' && *start != '\t' &&
                   *start != '*' && *start != '&';
                 start--);
            if (start > line)
            {
                start += 1;
                if (strstr(line, "__nonnull") != NULL ||
                    line[(start - line) + 1] == '*')
                {
                    continue;
                }
                end = strchr(line, ';');
                if (end)
                {
                    char *func_str =
                        measured_memmove_copy(start, (end - start));
                    char *return_type = copy_of("void ");
                    char *full_func =
                        alloc_str_free_substrs(return_type, func_str);
                    if (acap == asize)
                    {
                        acap *= 2;
                        func_str_array = (char **)nrealloc(
                            func_str_array, acap * sizeof(char *));
                    }
                    func_str_array[asize++] = full_func;
                }
                /* Here we can check for edge cases. */
            }
        }
    }
    fclose(file);
    func_str_array[asize] = NULL;
    return func_str_array;
}

char **
find_variabels_in_file(char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    long          len;
    unsigned long size;
    static char  *line;
    unsigned long acap = 10, asize = 0;
    char        **var_str_array = (char **)nmalloc(acap * sizeof(char *));
    while ((len = getline(&line, &size, file)) != EOF)
    {
        if (line[len - 1] == '\n')
        {
            line[--len] = '\0';
        }
        const char *parent_start = strchr(line, '(');
        const char *parent_end   = strchr(line, ')');
        const char *assign       = strchr(line, ';');
        if (parent_start || parent_end)
        {
            continue;
        }
        if (assign)
        {
            char *str = measured_memmove_copy(line, (assign - line) + 1);
            if (acap == asize)
            {
                acap *= 2;
                var_str_array =
                    (char **)nrealloc(var_str_array, acap * sizeof(char *));
            }
            var_str_array[asize++] = str;
        }
    }
    fclose(file);
    var_str_array[asize] = NULL;
    return var_str_array;
}
