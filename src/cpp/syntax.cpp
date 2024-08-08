#include "../include/definitions.h"
#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <fcntl.h>

void
syntax_check_file(openfilestruct *file)
{
    if (openfile->filetop->next == NULL)
    {
        return;
    }
    const char *fext = get_file_extention();
    if (strcmp(fext, "cpp") == 0 || strcmp(fext, "c") == 0)
    {
        linestruct *line;
        for (line = file->filetop; line != NULL; line = line->next)
        {
            check_for_syntax_words(line);
        }
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
            *attr |= A_BOLD;
            color_fg += 5;
        }
        if (strncmp(color_fg, "italic", 6) == 0)
        {
            *attr |= A_ITALIC;
            color_fg += 7;
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

/* Add syntax regex to end of colortype list 'c'. */
void
add_syntax_color(const char *color_fg, const char *color_bg, const char *rgxstr, colortype **c)
{
    if (c == NULL)
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
    if (!compile(rgxstr, NANO_REG_EXTENDED, &start_rgx))
    {
        return;
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
    short      fg, bg;
    int        attr;
    regex_t   *start_rgx = NULL, *end_rgx = NULL;
    colortype *nc;
    if (c == NULL)
    {
        return;
    }
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
check_func_syntax(char ***words, unsigned int *i)
{
    unsigned long  at   = 0;
    unsigned short type = 0;
    if ((*words)[++(*i)] == NULL)
    {
        return false;
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
            remove_leading_parent(&(*words)[*i]);
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
    unsigned int  i;
    unsigned long size, len;
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
            if (type & CS_STRUCT)
            {
                if (!is_syntax_struct(words[++i]))
                {
                    add_syntax_word("brightgreen", NULL, rgx_word(words[i]));
                    add_syntax_struct(words[i]);
                }
            }
            if (type & CS_ENUM || type & CS_CLASS)
            {
                add_syntax_word("brightgreen", NULL, rgx_word(words[++i]));
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
        }
        free(words);
    }
    fclose(f);
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
                add_syntax_word("lagoon", NULL, rgx_word(word));
                return NEXT_WORD_ALSO;
            }
            if (word[i] == ';' || word[i] == ')')
            {
                word[i] = '\0';
                break;
            }
        }
        add_syntax_word("lagoon", NULL, rgx_word(word));
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
        sprintf(buf, "%s%s%s%s", pwd, "/", (current_file != NULL) ? current_file : "", extract_include(str));
        if (is_file_and_exists(buf))
        {
            check_syntax(buf);
        }
        free(current_file);
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
    unsigned i;
    char   **words;
    if (is_empty_line(line))
    {
        return;
    }
    words = words_in_str(line->data);
    if (*words == NULL)
    {
        free(words);
        return;
    }
    for (i = 0; words[i] != NULL; i++)
    {
        if (is_syntax_struct(words[i]))
        {
            if (words[i + 1] != NULL)
            {
                handle_struct_syntax(&words[i + 1]);
                add_syntax_word("lagoon", NULL, words[++i]);
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
        else if (type & CS_CLASS || type & CS_ENUM)
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

/* Add some basic cpp syntax. */
void
do_cpp_syntax(void)
{
    if (last_c_color == NULL)
    {
        return;
    }
    add_syntax_word("gray", NULL, ";");
    add_syntax_word("brightred", NULL, "\\<[A-Z_][0-9A-Z_]*\\>");
    add_syntax_word("sand", NULL, "\\<[0-9]\\>");
    add_syntax_word("blue", NULL, "\\<(NULL|nullptr|FALSE|TRUE)\\>");
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
    add_syntax_word("brightyellow", NULL, "\\<(if|else|for|while|do|switch|case|default)\\>");
    add_syntax_word("brightyellow", NULL, "\\<(try|throw|catch|operator|new|delete)\\>");
    add_syntax_word("brightmagenta", NULL, "\\<(using|break|continue|goto|return)\\>");
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

colortype *
get_last_c_colortype(void)
{
    if (c_syntaxtype == NULL)
    {
        c_syntaxtype = get_c_syntaxtype();
        if (c_syntaxtype == NULL)
        {
            return NULL;
        }
    }
    colortype *c = NULL;
    /* Find end of colortype list in syntax 'c' */
    for (c = c_syntaxtype->color; c->next != NULL; c = c->next)
        ;
    /* Return if 'c' == NULL */
    if (c == NULL)
    {
        return NULL;
    }
    return c;
}

syntaxtype *
get_c_syntaxtype(void)
{
    syntaxtype *s = NULL;
    for (s = syntaxes; s != NULL && strcmp(s->name, "c"); s = s->next)
        ;
    if (s == NULL)
    {
        return NULL;
    }
    return s;
}

void
update_c_syntaxtype(void)
{
    if (c_syntaxtype == NULL)
    {
        c_syntaxtype = get_c_syntaxtype();
        if (c_syntaxtype == NULL)
        {
            return;
        }
    }
    set_syntax_colorpairs(c_syntaxtype);
}

/* Add a word and the color of that word to the syntax list. */
void
add_syntax_word(const char *color_fg, const char *color_bg, const char *word)
{
    if (last_c_color == NULL)
    {
        last_c_color = get_last_c_colortype();
    }
    /* Add the syntax. */
    add_syntax_color(color_fg, color_bg, word, &last_c_color);
}

void
add_syntax_struct(const char *name)
{
    syntax_structs.push_back(name);
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
