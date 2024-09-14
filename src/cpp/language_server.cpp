#include "../include/prototypes.h"

language_server_t *language_server_t::instance   = NULL;
pthread_mutex_t    language_server_t::init_mutex = PTHREAD_MUTEX_INITIALIZER;

int
language_server_t::find_endif(linestruct *from)
{
    int         lvl   = 0;
    const char *found = NULL;
    const char *start = NULL;
    const char *end   = NULL;
    for (linestruct *line = from; line != NULL; line = line->next)
    {
        if (!LINE_ISSET(line, DONT_PREPROSSES_LINE))
        {
            found = strchr(line->data, '#');
            if (found)
            {
                start = found + 1;
                ADV_TO_NEXT_WORD(start);
                if (!*start)
                {
                    return -1;
                }
                end = start;
                ADV_PAST_WORD(end);
                string word(start, (end - start));
                if (word == "ifndef" || word == "ifdef" || word == "if")
                {
                    lvl += 1;
                }
                else if (word == "endif")
                {
                    if (lvl == 0)
                    {
                        return line->lineno;
                    }
                    lvl -= 1;
                }
            }
        }
    }
    return 0;
}

/* This is the only way to access the language_server.
 * There also exist`s a shorthand for this function
 * call named 'LSP'. */
language_server_t *
language_server_t::Instance(void)
{
    if (!instance)
    {
        pthread_mutex_guard_t guard(&init_mutex);
        if (!instance)
        {
            instance = new language_server_t();
        }
    }
    return instance;
}

language_server_t::language_server_t(void)
{
    pthread_mutex_init(&_mutex, NULL);
#ifdef __GNUC__
    ADD_BASE_DEF(__GNUC__);
#endif
#ifdef __GNUC_MINOR__
    ADD_BASE_DEF(__GNUC_MINOR__);
#endif
#ifdef __clang_major__
    ADD_BASE_DEF(__clang_major__);
#endif
#ifdef __clang_minor__
    ADD_BASE_DEF(__clang_minor__);
#endif
#ifdef _GNU_SOURCE
    ADD_BASE_DEF(_GNU_SOURCE);
#endif
#ifdef __cplusplus
    ADD_BASE_DEF(__cplusplus);
#endif
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    ADD_BASE_DEF(__GXX_EXPERIMENTAL_CXX0X__);
#endif
}

language_server_t::~language_server_t(void)
{
    pthread_mutex_destroy(&_mutex);
    if (instance)
    {
        free(instance);
    }
}

/* Return`s index of entry if found otherwise '-1'. */
int
language_server_t::is_defined(const string &name)
{
    const auto *data = _defines.data();
    for (int i = 0; i < _defines.size(); i++)
    {
        if (data[i].name == name)
        {
            return i;
        }
    }
    return -1;
}

bool
language_server_t::has_been_included(const string &name)
{
    const auto *data = _includes.data();
    for (int i = 0; i < _includes.size(); i++)
    {
        if (data[i] == name)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* If define is in vector then return its value.
 * Note that the caller should check is define
 * exists before using this function.
 * As it returns "" if not found as well as if
 * the define has no value. */
string
language_server_t::define_value(const string &name)
{
    int index = is_defined(name);
    if (index != -1)
    {
        const auto *data = _defines.data();
        return data[index].value;
    }
    return "";
}

/* If define is not already in vector then add it. */
void
language_server_t::add_define(const define_entry_t &entry)
{
    if (is_defined(entry.name) == -1)
    {
        NLOG("define added: %s\n", entry.name.c_str());
        _defines.push_back(entry);
    }
}

void
language_server_t::define(linestruct *line, const char **ptr)
{
    const char *start = *ptr;
    const char *end   = *ptr;
    ADV_TO_NEXT_WORD(start);
    if (!*start)
    {
        return;
    }
    end = start;
    ADV_PTR(end, (*end != '(' && *end != ' ' && *end != '\t'));
    if (*end == '(')
    {
        ADV_PTR(end, (*end != ')'));
        if (!*end)
        {
            return;
        }
        end += 1;
    }
    string define(start, (end - start));
    NLOG("define: %s\n", define.c_str());
    start = end;
    ADV_TO_NEXT_WORD(start);
    end = start;
    ADV_PAST_WORD(end);
    string value(start, (end - start));
    NLOG("value: %s\n", value.c_str());
}

void
language_server_t::ifndef(const string &define, linestruct *current_line)
{
    if (is_defined(define) == -1)
    {
        // NLOG("define: '%s' was not defined.\n", define.c_str());
        return;
    }
    int endif = find_endif(current_line->next);
    for (linestruct *line                              = current_line->next;
         line != NULL && (line->lineno != endif); line = line->next)
    {
        LINE_SET(line, DONT_PREPROSSES_LINE);
    }
}

void
language_server_t::ifdef(const string &define, linestruct *current_line)
{
    if (is_defined(define) != -1)
    {
        // NLOG("define: '%s' was defined\n", define.c_str());
        return;
    }
    int endif = find_endif(current_line->next);
    for (linestruct *line                              = current_line->next;
         line != NULL && (line->lineno != endif); line = line->next)
    {
        LINE_SET(line, DONT_PREPROSSES_LINE);
    }
}

void
language_server_t::undef(const string &define)
{
    auto it = _defines.begin();
    while (it != _defines.end())
    {
        if (it->name == define)
        {
            it = _defines.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
language_server_t::handle_if(linestruct *line, const char **ptr)
{
    const char *start = *ptr;
    const char *end   = *ptr;
    ADV_TO_NEXT_WORD(start);
    if (!*start)
    {
        return;
    }
    const char *found             = NULL;
    const char *and_found         = NULL;
    const char *or_found          = NULL;
    bool        should_be_defined = FALSE;
    bool        was_correct       = FALSE;
    do {
        found = strstr(start, "defined");
        if (!found)
        {
            break;
        }
        should_be_defined = FALSE;
        was_correct       = FALSE;
        NLOG("start: %s\n", start);
        NLOG("found: %s\n", found);
        /* This def should not be defined. */
        if (found != start && start[(found - start) - 1] == '!')
        {
            NLOG("%c\n", start[(found - start) - 1]);
            should_be_defined = FALSE;
        }
        /* This def should be defined to continue. */
        else
        {
            should_be_defined = TRUE;
        }
        start = found + 7;
        ADV_TO_NEXT_WORD(start);
        if (!*start)
        {
            return;
        }
        end = start;
        ADV_PAST_WORD(end);
        string def(start, (end - start));
        NLOG("def: %s\n", def.c_str());
        and_found = strstr(end, "&&");
        or_found  = strstr(end, "||");
        /* If this def should be defined. */
        if (should_be_defined)
        {
            if (is_defined(def) == -1)
            {
                NLOG("def correct: %s\n", def.c_str());
                was_correct = FALSE;
            }
            else
            {
                was_correct = TRUE;
            }
        }
        /* If this def should not be defined. */
        else
        {
            if (is_defined(def) == -1)
            {
                NLOG("def correct: %s\n", def.c_str());
                was_correct = TRUE;
            }
            else
            {
                was_correct = FALSE;
            }
        }
        if (!was_correct)
        {
            NLOG("%s\n", line->data);
            if (((and_found && or_found) && or_found < and_found) ||
                and_found || (!and_found && !or_found))
            {
                int endif = find_endif(line->next);
                for (linestruct *l = line->next; l && l->lineno != endif;
                     l             = l->next)
                {
                    LINE_SET(l, DONT_PREPROSSES_LINE);
                }
            }
        }

        found += 7;
    }
    while (found);
}

const char *
get_preprosses_type(linestruct *line, string &word)
{
    const char *found = strchr(line->data, '#');
    const char *start = NULL;
    const char *end   = NULL;
    if (found)
    {
        start = found + 1;
        ADV_TO_NEXT_WORD(start);
        if (!*start)
        {
            return NULL;
        }
        end = start;
        ADV_PAST_WORD(end);
        word = string(start, (end - start));
        return end;
    }
    return NULL;
}

string
get_next_word(const char **ptr)
{
    const char *start = *ptr;
    const char *end   = *ptr;
    ADV_TO_NEXT_WORD(start);
    if (!*start)
    {
        return "";
    }
    end = start;
    ADV_PAST_WORD(end);
    *ptr = end;
    return string(start, (end - start));
}

void
language_server_t::check(linestruct *from, string file)
{
    if (!from)
    {
        from = openfile->filetop;
        file = current_file_dir() + string(tail(openfile->filename));
        // NLOG("current file: '%s'.\n", file.c_str());
    }
    const char *found = NULL;
    for (linestruct *line = from; line != NULL; line = line->next)
    {
        if (!LINE_ISSET(line, DONT_PREPROSSES_LINE))
        {
            string word = "";
            found       = get_preprosses_type(line, word);
            if (found)
            {
                if (word == "ifndef")
                {
                    word = get_next_word(&found);
                    ifndef(word, line);
                }
                else if (word == "ifdef")
                {
                    word = get_next_word(&found);
                    ifdef(word, line);
                }
                else if (word == "define")
                {
                    define(line, &found);
                }
                else if (word == "if")
                {
                    handle_if(line, &found);
                }
            }
            /* found = strchr(line->data, '#');
            if (found)
            {
                start = found + 1;
                ADV_TO_NEXT_WORD(start);
                if (!*start)
                {
                    return;
                }
                end = start;
                ADV_PAST_WORD(end);
                string key_word(start, (end - start));
                start = end;
                ADV_TO_NEXT_WORD(start);
                if (*start)
                {
                    end = start;
                    ADV_PAST_WORD(end);
                    string word = string(start, (end - start));
                    if (key_word == "ifndef")
                    {
                        ifndef(word, line);
                    }
                    else if (key_word == "ifdef")
                    {
                        ifdef(word, line);
                    }
                    else if (key_word == "define")
                    {
                        start = end;
                        ADV_TO_NEXT_WORD(start);
                        string value = "";
                        if (*start)
                        {
                            end = start;
                            ADV_PAST_WORD(end);
                            value = string(start, (end - start));
                        }
                        define({word, value});
                    }
                    else if (key_word == "include")
                    {
                        start = found + 1;
                        ADV_PTR(start, (*start != '"' && *start != '<'));
                        if (*start)
                        {
                            if (*start == '<')
                            {
                                start += 1;
                                end = start;
                                ADV_PTR(end, (*end != '>'));
                                if (*end)
                                {
                                    string check_file =
                                        "/usr/include/" +
                                        string(start, (end - start));
                                    NLOG("%s\n", check_file.c_str());
                                    if (!has_been_included(check_file))
                                    {
                                        linestruct *head =
                                            retrieve_file_as_lines(check_file);
                                        if (head)
                                        {
                                            _includes.push_back(check_file);
                                            check(head, check_file);
                                            while (head)
                                            {
                                                linestruct *node = head;
                                                head             = head->next;
                                                free(node->data);
                                                free(node);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } */
        }
    }
}

/* Add the current defs to color map. */
void
language_server_t::add_defs_to_color_map(void)
{
    const auto *data = _defines.data();
    for (int i = 0; i < _defines.size(); i++)
    {
        test_map[data[i].name] = {
            FG_VS_CODE_BLUE,
            -1,
            -1,
            DEFINE_SYNTAX,
        };
    }
}

vector<define_entry_t>
language_server_t::retrieve_defines(void)
{
    return _defines;
}
