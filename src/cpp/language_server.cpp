#include "../include/prototypes.h"

language_server_t *language_server_t::instance   = NULL;
pthread_mutex_t    language_server_t::init_mutex = PTHREAD_MUTEX_INITIALIZER;

int language_server_t::find_endif(linestruct *from)
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

void language_server_t::fetch_compiler_defines(string compiler)
{
    if ((compiler != "clang" && compiler != "clang++") &&
        (compiler != "gcc" && compiler != "g++"))
    {
        logE("'%s' is an invalid compiler.", compiler.c_str());
        return;
    }
    compiler += " -dM -E - < /dev/null";
    unsigned int n_lines;
    char       **lines = retrieve_exec_output(compiler.c_str(), &n_lines);
    if (lines)
    {
        for (unsigned int i = 0; i < n_lines; i++)
        {
            const char *start = lines[i];
            const char *end   = NULL;
            ADV_PAST_WORD(start);
            ADV_TO_NEXT_WORD(start);
            if (!*start)
            {
                free(lines[i]);
                continue;
            }
            end = start;
            ADV_PAST_WORD(end);
            string name(start, (end - start));
            ADV_TO_NEXT_WORD(end);
            start = end;
            for (; *end; end++);
            if (start == end)
            {
                free(lines[i]);
                continue;
            }
            string value(start, (end - start));
            add_define({name, value});
            free(lines[i]);
        }
        free(lines);
    }
}

/* This is the only way to access the language_server.
 * There also exist`s a shorthand for this function
 * call named 'LSP'. */
language_server_t *language_server_t::Instance(void)
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
    fetch_compiler_defines("clang++");
    fetch_compiler_defines("clang");
    fetch_compiler_defines("g++");
    fetch_compiler_defines("gcc");
#ifdef __cplusplus
    ADD_BASE_DEF(__cplusplus);
#endif
    ADD_BASE_DEF(_GNU_SOURCE);
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
int language_server_t::is_defined(const string &name)
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

bool language_server_t::has_been_included(const string &name)
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
string language_server_t::define_value(const string &name)
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
void language_server_t::add_define(const define_entry_t &entry)
{
    if (is_defined(entry.name) == -1)
    {
        /* NLOG("define added: %s\n", entry.name.c_str()); */
        _defines.push_back(entry);
    }
}

void language_server_t::define(linestruct *line, const char **ptr)
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
    // NLOG("define: %s\n", define.c_str());
    start = end;
    ADV_TO_NEXT_WORD(start);
    end = start;
    ADV_PAST_WORD(end);
    string value(start, (end - start));
    // NLOG("value: %s\n", value.c_str());
}

void language_server_t::ifndef(const string &define, linestruct *current_line)
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

void language_server_t::ifdef(const string &define, linestruct *current_line)
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

void language_server_t::undef(const string &define)
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

static int define_is_equl_or_greater(const string &statement)
{
    NLOG("statement:('%s')\n", statement.c_str());
    const char *start = &statement[0];
    const char *end   = start;
    ADV_TO_NEXT_WORD(start);
    end = start;
    ADV_PAST_WORD(end);
    if (end == start)
    {
        return -1;
    }
    string def(start, (end - start));
    string def_value = LSP->define_value(def);
    if (def_value.empty())
    {
        return -1;
    }
    start = strstr(end, ">=");
    if (!start)
    {
        return -1;
    }
    start += 2;
    ADV_TO_NEXT_WORD(start);
    end = start;
    ADV_PAST_WORD(end);
    if (end == start)
    {
        return -1;
    }
    string compare(start, (end - start));
    if (atoi(def_value.c_str()) >= atoi(compare.c_str()))
    {
        return TRUE;
    }
    return FALSE;
}

vector<string> language_server_t::split_if_statement(const string &str)
{
    /* vector<string> result;
    const auto    *data  = str.data();
    const char    *found = NULL;
    unsigned int   index;
    do {
        found = string_strstr_array(
            data, {"defined", "&&", "!defined", "(", ")"}, &index);
        if (found)
        {
            result.push_back(str.substr((data - str.data()), (found - data)));
            data = found;
            NLOG("found: %s\n", found);
            if (index == 0)
            {
                data += 7;
            }
            else if (index == 1)

            {
                data += 2;
            }
            else if (index == 2)
            {
                data += 8;
            }
            else if (index == 3 || index == 4)
            {
                data += 1;
            }
        }
    }
    while (found);
    for (const auto &s : result)
    {
        NLOG("%s ", s.c_str());
    }
    NLOG("\n"); */
    return {};
}

void language_server_t::handle_if(linestruct *line, const char **ptr)
{
    string full_delc = parse_full_pp_delc(line, ptr);
    NLOG("%s\n", full_delc.c_str());
    // split_if_statement(full_delc);
    const char *start = &full_delc[0];
    const char *end   = &full_delc[0];
    ADV_TO_NEXT_WORD(start);
    if (!*start)
    {
        return;
    }
    const char        *found     = NULL;
    string             def       = "";
    string             found_str = "";
    unsigned int       index;
    const char        *next_rule         = NULL;
    const unsigned int rule_count        = 5;
    const char        *rules[rule_count] = {"&&", "||", ">=", "defined", "?"};
    bool               should_be_defined = FALSE;
    bool               was_correct       = FALSE;
    do {
        should_be_defined = FALSE;
        was_correct       = FALSE;
        /* If no more rules are found we break. */
        found = strstr_array(start, rules, rule_count, &index);
        if (!found)
        {
            break;
        }
        switch (index)
        {
            case 0 : /* && */
            {
            }
            case 1 : /* || */
            {
            }
            case 2 : /* >= */
            {
                found += 2;
                start = found;
                break;
            }
            case 3 : /* defined */
            {
                should_be_defined =
                    (found != start && start[(found - start) - 1] == '!') ?
                        FALSE : /* This def should not be defined. */
                        TRUE;   /* This def should be defined. */
                start = found + 7;
                ADV_TO_NEXT_WORD(start);
                if (!*start)
                {
                    return;
                }
                end = start;
                ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != ')'));
                def = string(start, (end - start));
                found += 7;
                start     = end;
                next_rule = strstr_array(start, rules, rule_count, &index);
                if (!next_rule)
                {
                    return;
                }
                if (index == 4)
                {
                    NLOG("full_decl: %s\n", full_delc.c_str());
                    start = next_rule + 1;
                    end   = start;
                    ADV_PTR(end, (*end != ':'));
                    if (!*end)
                    {
                        return;
                    }
                    if (is_defined(def) == -1)
                    {
                        end += 1;
                        start = end;
                        for (; *end; end++);
                        if (start == end)
                        {
                            return;
                        }
                    }
                    ADV_TO_NEXT_WORD(start);
                    next_rule = strstr_array(start, rules, rule_count, &index);
                    if (index == 3)
                    {
                        should_be_defined =
                            (next_rule != start &&
                             start[(next_rule - start) - 1] == '!') ?
                                FALSE : /* This def should not be defined. */
                                TRUE;   /* This def should be defined. */
                        NLOG("%s\n", BOOL_STR(should_be_defined));
                        start += 7;
                        ADV_TO_NEXT_WORD(start);
                        end = start;
                        ADV_PAST_WORD(end);
                        if (end == start)
                        {
                            return;
                        }
                        def = string(start, (end - start));
                        /* This def should be defined. */
                        if (should_be_defined)
                        {
                            if (is_defined(def) != -1)
                            {
                                was_correct = TRUE;
                            }
                        }
                        /* Should not be defined. */
                        else
                        {
                            if (is_defined(def) == -1)
                            {
                                was_correct = TRUE;
                            }
                        }
                        NLOG("was_correct: %s\n", BOOL_STR(was_correct));
                        if (!was_correct)
                        {
                            int endif = find_endif(line->next);
                            for (linestruct *l              = line->next;
                                 l && l->lineno != endif; l = l->next)
                            {
                                LINE_SET(l, DONT_PREPROSSES_LINE);
                            }
                        }
                    }
                    else if (index == 2)
                    {
                        if (!define_is_equl_or_greater(
                                string(start, (end - start))))
                        {
                            int endif = find_endif(line->next);
                            for (linestruct *l              = line->next;
                                 l && l->lineno != endif; l = l->next)
                            {
                                LINE_SET(l, DONT_PREPROSSES_LINE);
                            }
                            return;
                        }
                        /* ADV_TO_NEXT_WORD(start);
                        end = start;
                        ADV_PAST_WORD(end);
                        if (start == end)
                        {
                            return;
                        }
                        def           = string(start, (end - start));
                        int def_index = is_defined(def);
                        if (def_index == -1)
                        {
                            return;
                        }
                        string def_value = define_value(def);
                        if (def_value == "")
                        {
                            return;
                        }
                        start = next_rule + 2;
                        ADV_TO_NEXT_WORD(start);
                        end = start;
                        ADV_PAST_WORD(end);
                        if (start == end)
                        {
                            return;
                        }
                        string compare(start, (end - start));
                        if (!(atoi(def_value.c_str()) >= atoi(compare.c_str())))
                        {
                            int endif = find_endif(line->next);
                            for (linestruct *l              = line->next;
                                 l && l->lineno != endif; l = l->next)
                            {
                                LINE_SET(l, DONT_PREPROSSES_LINE);
                            }
                        } */
                    }
                }
                break;
            }
            case 4 : /* ? */
            {

                break;
            }
        }
        /* This def should be defined. */
        if (should_be_defined)
        {
            if (is_defined(def) != -1)
            {
                was_correct = TRUE;
            }
        }
        /* Should not be defined. */
        else
        {
            if (is_defined(def) == -1)
            {
                was_correct = TRUE;
            }
        }
        if (!was_correct)
        {
            if (next_rule)
            {
                string nrule(next_rule, 2);
                if (nrule == "&&")
                {
                    int endif = find_endif(line->next);
                    for (linestruct *l = line->next; l && l->lineno != endif;
                         l             = l->next)
                    {
                        LINE_SET(l, DONT_PREPROSSES_LINE);
                    }
                }
            }
        }
    }
    while (found);
}

const char *get_preprosses_type(linestruct *line, string &word)
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

string get_next_word(const char **ptr)
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

void language_server_t::check(linestruct *from, string file)
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
void language_server_t::add_defs_to_color_map(void)
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

/* Parses a full preprossesor decl so that '\' are placed on the same line. */
string language_server_t::parse_full_pp_delc(linestruct *line, const char **ptr,
                                             int *end_lineno)
{
    PROFILE_FUNCTION;
    string ret = "";
    ret.reserve(100);
    const char *start = *ptr;
    const char *end   = *ptr;
    do {
        ADV_TO_NEXT_WORD(start);
        if (!*start)
        {
            break;
        }
        end = start;
        ADV_PTR(end, (*end != ' ' && *end != '\t' && *end != '\\'));
        if (*end == '\\')
        {
            line  = line->next;
            start = line->data;
            end   = start;
        }
        else
        {
            if (!ret.empty())
            {
                ret += " ";
            }
            ret += string(start, (end - start));
        }
        start = end;
    }
    while (*end);
    end_lineno ? *end_lineno = (int)line->lineno : 0;
    return ret;
}

vector<define_entry_t> language_server_t::retrieve_defines(void)
{
    return _defines;
}
