#include "../include/definitions.h"
#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <fcntl.h>

void
syntax_check_file(openfilestruct *file)
{
    PROFILE_FUNCTION;
    if (openfile->filetop->next == NULL)
    {
        return;
    }
    const char *fext = get_file_extention();
    if (fext && (strncmp(fext, "cpp", 3) == 0 || strncmp(fext, "c", 1) == 0))
    {
        set_last_c_colortype();
        /* for (linestruct *line = file->filetop; line != NULL; line = line->next)
        {
            check_for_syntax_words(line);
        } */
        openfile_syntax_c();
    }
}

bool
parse_color_opts(const char *color_fg, const char *color_bg, short *fg, short *bg, int *attr)
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

/* Add syntax regex to end of colortype list 'c'.  This need`s to be faster. currently this takes around 0.4 ms. */
void
add_syntax_color(const char *color_fg, const char *color_bg, const char *rgxstr, colortype **c, const char *from_file)
{
    if (*c == NULL)
    {
        return;
    }
    short      fg, bg;
    int        attr;
    regex_t   *start_rgx = NULL;
    colortype *nc        = NULL;
    if (!parse_color_opts(color_fg, color_bg, &fg, &bg, &attr))
    {
        return;
    }
    if (from_file != NULL)
    {
        if (!compile_with_callback(rgxstr, NANO_REG_EXTENDED, &start_rgx, from_file))
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
add_start_end_syntax(const char *color_fg, const char *color_bg, const char *start, const char *end, colortype **c)
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
        add_syntax_word("yellow", NULL, rgx_word((*words)[*i]));
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
            add_syntax_word("yellow", NULL, rgx_word((*words)[*i]));
            return TRUE;
        }
    }
    return FALSE;
}

/* Check a file for syntax, and add relevent syntax. */
void
check_syntax(const char *path)
{
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
                    add_syntax_word("brightgreen", NULL, rgx_word(words[i]), path);
                    add_syntax_struct(words[i]);
                }
            }
            else if (type & CS_CLASS)
            {
                if (*words[i + 1] == '{')
                    ;
                else if (!is_syntax_class(words[++i]))
                {
                    add_syntax_word("brightgreen", NULL, rgx_word(words[i]), path);
                    add_syntax_class(words[i]);
                }
            }
            else if (type & CS_ENUM)
            {
                /* If this is a anonumus enum skip, (will be implemented later). */
                if (*words[i + 1] == '{')
                    ;
                else
                {
                    add_syntax_word("brightgreen", NULL, rgx_word(words[++i]), path);
                }
            }
            else if (type & CS_CHAR || type & CS_VOID || type & CS_INT || type & CS_LONG || type & CS_BOOL ||
                     type & CS_SIZE_T || type & CS_SSIZE_T)
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
            //     /* If the include file is a 'local' file, then base the full path on current path. */
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
            //         const char *full_rpath = concat_path(rpath, extract_include(words[i]));
            //         if (is_file_and_exists(full_rpath))
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
            //         const char *rpath = concat_path("/usr/include/", extract_include(words[i]));
            //         if (is_file_and_exists(rpath))
            //         {
            //             if (!is_in_handled_includes_vec(rpath))
            //             {
            //                 add_to_handled_includes_vec(rpath);
            //                 check_syntax(rpath);
            //             }
            //             continue;
            //         }
            //         rpath = concat_path("/usr/include/c++/v1/", extract_include(words[i]));
            //         if (is_file_and_exists(rpath))
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
        free(words);
    }
    fclose(f);
}

void
check_include_file_syntax(const char *path)
{
    unsigned short type;
    unsigned       i;
    unsigned long  nwords;
    char         **words = words_from_file(path, &nwords);
    if (words == NULL)
    {
        return;
    }
    for (i = 0; i < nwords; i++)
    {
        if (*words[i] == '#')
        {
            unsigned long slen = strlen(words[i]) - 1;
            memmove(words[i], words[i] + 1, slen);
            words[i][slen] = '\0';
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
                    add_syntax_word("brightgreen", NULL, rgx_word(words[i]), path);
                    add_syntax_struct(words[i]);
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
                    add_syntax_word("brightgreen", NULL, rgx_word(words[i]), path);
                    add_syntax_class(words[i]);
                }
            }
        }
        else if (type & CS_ENUM)
        {
            if (words[++i] != NULL)
            {
                if (*words[i] == '{' || *words[i] == '*')
                {
                    continue;
                }
                add_syntax_word("brightgreen", NULL, rgx_word(words[i]));
            }
        }
        else if (type & CS_CHAR || type & CS_VOID || type & CS_INT || type & CS_LONG || type & CS_BOOL ||
                 type & CS_SIZE_T || type & CS_SSIZE_T)
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
                            (words[i] != NULL) ? add_syntax_word("yellow", NULL, rgx_word(words[i]), path) : void();
                        }
                    }
                }
                else if (words[i + 1] && *words[i + 1] == '(')
                {
                    strip_leading_chars_from(words[i], '*');
                    (words[i] != NULL) ? add_syntax_word("yellow", NULL, rgx_word(words[i]), path) : void();
                }
            }
        }
        else if (type & CS_INCLUDE)
        {
            if (words[++i] != NULL)
            {
                if (!is_in_handled_includes_vec(words[i]))
                {
                    NETLOGGER.log("%s\n", words[i]);
                    add_to_handled_includes_vec(words[i]);
                    handle_include(words[i]);
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
        add_syntax_word("brightgreen", NULL, rgx_word(word));
    }
    else if (*type & CS_INT || *type & CS_VOID || *type & CS_LONG || *type & CS_CHAR || *type & CS_BOOL ||
             *type & CS_SIZE_T || *type & CS_SSIZE_T || *type & CS_SHORT)
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
                add_syntax_word(VAR_COLOR, NULL, rgx_word(word));
                return NEXT_WORD_ALSO;
            }
            if (word[i] == ';' || word[i] == ')')
            {
                word[i] = '\0';
                break;
            }
        }
        add_syntax_word(VAR_COLOR, NULL, rgx_word(word));
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
        return;
    }
    if (*str == '<')
    {
        sprintf(buf, "%s%s", "/usr/include/", extract_include(str));
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
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
            check_syntax(buf);
            return;
        }
        memset(buf, '\0', sizeof(buf));
        sprintf(buf, "%s%s%s", "/usr/include/c++/v1/", str, ".h");
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
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
        sprintf(buf, "%s%s%s%s", pwd, "/", (current_file != NULL) ? current_file : "", str);
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
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
    add_syntax_word("bold,blue", NULL, rgx_word(str));
}

static unsigned short last_type = 0;

/* Check a line for syntax words, also index files that are included and add functions as well. */
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
                    add_syntax_word("lagoon", NULL, rgx_word(words[++i]));
                }
            }
        }
        else if (is_syntax_class(words[i]))
        {
            if (words[i + 1] != NULL)
            {
                handle_struct_syntax(&words[i + 1]);
                add_syntax_word("lagoon", NULL, rgx_word(words[++i]));
            }
        }
        const unsigned short type = retrieve_c_syntax_type(words[i]);
        if (last_type != 0)
        {
            if (last_type & CS_VOID || last_type & CS_INT || last_type & CS_CHAR || last_type & CS_LONG ||
                last_type & CS_BOOL || last_type & CS_SIZE_T || last_type & CS_SSIZE_T || last_type & CS_SHORT)
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
                add_syntax_word("yellow", NULL, rgx_word(words[i]));
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
                add_syntax_word("brightgreen", NULL, rgx_word(words[i]));
                add_syntax_struct(words[i]);
            }
        }
        else if (type & CS_CLASS)
        {
            if (!is_syntax_class(words[++i]))
            {
                add_syntax(&type, words[i]);
                add_syntax_class(words[i]);
            }
        }
        else if (type & CS_ENUM)
        {
            add_syntax_word("brightgreen", NULL, rgx_word(words[++i]));
        }
        else if (type & CS_LONG || type & CS_VOID || type & CS_INT || type & CS_CHAR || type & CS_BOOL ||
                 type & CS_SIZE_T || type & CS_SSIZE_T || type & CS_SHORT)
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
    free(words);
}

/* Add some "basic" cpp syntax. */
void
do_cpp_syntax(void)
{
    set_last_c_colortype();
    if (last_c_color == NULL)
    {
        NETLOGGER.log("last_c_color == NULL\n");
        return;
    }
    /* add_syntax_word("gray", NULL, ";"); */
    // add_syntax_word("brightred", NULL, "\\<[A-Z_][0-9A-Z_]*\\>");
    add_syntax_word("sand", NULL, "[0-9]");
    add_syntax_word("bold,blue", NULL, "\\<(NULL|nullptr|FALSE|TRUE)\\>");
    add_syntax_word("brightmagenta", NULL, "^[[:blank:]]*[A-Z_a-z][0-9A-Z_a-z]*:[[:blank:]]*$");
    add_syntax_word("normal", NULL, ":[[:blank:]]*$");
    /* This makes word after green, while typing. */
    add_syntax_word("brightgreen", NULL, "(namespace|enum|struct|class)[[:blank:]]+[A-Za-z_][A-Za-z_0-9]*");
    /* Types and related keywords. */
    add_syntax_word("bold,brightblue", NULL,
                    "\\<(auto|const|bool|char|double|enum|extern|float|inline|int|long|restrict|short|signed|"
                    "sizeof|static|struct|typedef|union|unsigned|void)\\>");
    add_syntax_word("brightgreen", NULL, "\\<([[:lower:]][[:lower:]_]*|(u_?)?int(8|16|32|64))_t\\>");
    add_syntax_word(
        "green", NULL,
        "\\<(_(Alignas|Alignof|Atomic|Bool|Complex|Generic|Imaginary|Noreturn|Static_assert|Thread_local))\\>");
    add_syntax_word("brightblue", NULL,
                    "\\<(class|explicit|friend|mutable|namespace|override|private|protected|public|register|"
                    "template|this|typename|virtual|volatile|false|true)\\>");
    add_syntax_word("brightgreen", NULL, "\\<(std|string|vector)\\>");
    /* Flow control. */
    if (term != NULL)
    {
        if (strcmp(term, "xterm") == 0)
        {
            add_syntax_word(
                "brightmagenta", NULL,
                "\\<(using|break|continue|goto|return|try|throw|catch|operator|new|delete|if|else|for|while|do|"
                "switch|case|default)\\>");
        }
        else
        {
            add_syntax_word(
                CONTROL_COLOR, NULL,
                "\\<(using|break|continue|goto|return|try|throw|catch|operator|new|delete|if|else|for|while|do|"
                "switch|case|default)\\>");
        }
    }
    add_syntax_word("brightmagenta", NULL, "'([^'\\]|\\\\([\"'\abfnrtv]|x[[:xdigit:]]{1,2}|[0-3]?[0-7]{1,2}))'");
    add_syntax_word("cyan", NULL,
                    "__attribute__[[:blank:]]*\\(\\([^)]*\\)\\)|__(aligned|asm|builtin|hidden|inline|packed|"
                    "restrict|section|typeof|weak)__");
    add_syntax_word("brightyellow", NULL, "\"([^\"]|\\\")*\"|#[[:blank:]]*include[[:blank:]]*<[^>]+>");
    add_syntax_word(
        "brightmagenta", NULL, "^[[:blank:]]*#[[:blank:]]*((define|else|endif|include(_next)?|line|undef)\\>|$)");
    add_syntax_word("green", NULL, "//[^\"]*$|(^|[[:blank:]])//.*");
    add_start_end_syntax("green", NULL, "/\\*", "\\*/", &last_c_color);
    add_syntax_word("brightwhite", "yellow", "\\<(FIXME|TODO|XXX)\\>");
    add_syntax_word(NULL, "green", "[[:space:]]+$");
    update_c_syntaxtype();
}

void
update_c_syntaxtype(void)
{
    PROFILE_FUNCTION;
    if (c_syntaxtype == NULL)
    {
        NETLOGGER.log("c_syntaxtype == NULL\n");
        return;
    }
    set_syntax_colorpairs(c_syntaxtype);
}

/* Add a word and the color of that word to the syntax list. */
void
add_syntax_word(const char *color_fg, const char *color_bg, const char *word, const char *from_file)
{
    if (last_c_color == NULL)
    {
        NETLOGGER.log("last_c_color == NULL.\n");
        return;
    }
    /* Add the syntax. */
    add_syntax_color(color_fg, color_bg, word, &last_c_color, from_file);
}

void
set_last_c_colortype(void)
{
    if (c_syntaxtype != NULL && last_c_color != NULL)
    {
        return;
    }
    syntaxtype *s;
    for (s = syntaxes; s && (strncmp(s->name, "c", 1)); s = s->next)
        ;
    if (s == NULL)
    {
        NETLOGGER.log("s == NULL\n");
        return;
    }
    colortype *c;
    for (c = s->color; c->next; c = c->next)
        ;
    if (c == NULL)
    {
        NETLOGGER.log("c == NULL\n");
        return;
    }
    c_syntaxtype = s;
    last_c_color = c;
}

void
add_syntax_struct(const char *name)
{
    syntax_structs.push_back(name);
}

void
add_syntax_class(const char *name)
{
    syntax_classes.push_back(name);
}

/* Return`s 'TRUE' if 'str' is in the 'syntax_structs' vector. */
bool
is_syntax_struct(std::string_view str)
{
    for (const auto &s : syntax_structs)
    {
        if (s == str)
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool
is_syntax_class(std::string_view str)
{
    for (const auto &c : syntax_classes)
    {
        if (c == str)
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
            add_syntax_word(VAR_COLOR, NULL, rgx_word(word));
        }
    }
    else if (type & CS_DEFINE)
    {
        add_syntax_word(DEFINE_COLOR, NULL, rgx_word(word));
    }
    else if (type & CS_STRUCT)
    {
        if (!is_syntax_struct(word))
        {
            add_syntax_word(STRUCT_COLOR, NULL, rgx_word(word));
            add_syntax_struct(word);
        }
    }
    else if (type & CS_CLASS)
    {
        if (!is_syntax_class(word))
        {
            add_syntax_word(STRUCT_COLOR, NULL, rgx_word(word));
            add_syntax_class(word);
        }
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
    NETLOGGER.log("words: %lu\n", nwords);
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
        else if (type & CS_CHAR || type & CS_VOID || type & CS_INT || type & CS_LONG || type & CS_SHORT ||
                 type & CS_SIZE_T || type & CS_SSIZE_T)
        {
            if (words[++i])
            {
                if (char_is_in_word(words[i], '(', &at))
                {
                    if (at)
                    {
                        char *ptr    = copy_of(words[i]);
                        words[i][at] = '\0';
                        add_syntax_word(FUNC_COLOR, NULL, rgx_word(words[i]));
                        free(words[i]);
                        words[i--] = measured_copy(ptr + at + 1, strlen(ptr) - at - 1);
                        free(ptr);
                    }
                }
                else if (words[i + 1] && *words[i + 1] == '(')
                {
                    add_syntax_word(FUNC_COLOR, NULL, rgx_word(words[i]));
                }
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
