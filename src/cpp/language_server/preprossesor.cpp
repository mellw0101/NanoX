#include "../../include/language_server/language_server.h"
#include "../../include/prototypes.h"

inline namespace utils
{
    const char *
    get_preprosses_type(linestruct *line, char **word)
    {
        const char *found = strchr(line->data, '#');
        if (found)
        {
            const char *start = found + 1;
            ADV_TO_NEXT_WORD(start);
            if (!*start)
            {
                return nullptr;
            }
            const char *end = start;
            ADV_PAST_WORD(end);
            *word = measured_copy(start, (end - start));
            return end;
        }
        return nullptr;
    }

    char *
    get_next_word(const char **ptr)
    {
        const char *start = *ptr;
        const char *end   = *ptr;
        ADV_TO_NEXT_WORD(start);
        if (!*start)
        {
            return nullptr;
        }
        end = start;
        ADV_PAST_WORD(end);
        *ptr = end;
        return measured_copy(start, (end - start));
    }

    static int
    define_is_equl_or_greater(const string &statement)
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
        string def_value = LSP.define_value(def);
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

    char *
    get_file_dir(const char *file)
    {
        const char *tail = strrchr(file, '/');
        if (tail)
        {
            return measured_copy(file, (tail - file) + 1);
        }
        else
        {
            return copy_of("");
        }
    }

    int
    get_include_path(const char *current_file, const char **ptr, char **path, bool *local)
    {
        const char *start = *ptr;
        const char *end   = nullptr;
        ADV_PTR(start, (*start != '"' && *start != '<'));
        if (*start)
        {
            if (*start == '<')
            {
                ++start;
                end = start;
                ADV_PTR(end, (*end != '>'));
                if (!*end)
                {
                    return -2;
                }
                *path  = measured_copy(start, (end - start));
                *local = false;
                return 0;
            }
            else if (*start == '"')
            {
                ++start;
                end = start;
                ADV_PTR(end, (*end != '"'));
                if (!*end)
                {
                    return -2;
                }
                char *p      = measured_copy(start, (end - start));
                char *op_dir = get_file_dir(current_file);
                *path        = alloc_str_free_substrs(op_dir, p);
                *local       = true;
                return 0;
            }
        }
        return -1;
    }
}

void
do_include(linestruct *line, const char *current_file, const char **ptr)
{
    char *path;
    bool  local;
    int   result = get_include_path(current_file, ptr, &path, &local);
    if (result < 0)
    {
        logE("get_include_path: Error: %d", result);
        return;
    }
    if (local)
    {
        if (!LSP.has_been_included(path))
        {
            if (is_file_and_exists(path))
            {
                NLOG("%s\n", path);
                IndexFile idfile;
                idfile.file = copy_of(path);
                idfile.head = retrieve_file_as_lines(path);
                LSP.index.include.push_back(idfile);
                if (idfile.head)
                {
                    LSP.check(idfile.head, path);
                }
            }
        }
    }
    else
    {
        /** FIXME: This needs refactoring. */
        MVector<string> check_files = {
            "/usr/include/c++/v1/" + string(path),
            "/usr/lib/clang/18/include/" + string(path),
            "/usr/include/" + string(path),
            "/usr/local/include/" + string(path)
        };
        string check_file = string(path);
        if (strchr(check_file.c_str(), '.') == nullptr)
        {
            return;
        }
        for (const auto &s : check_files)
        {
            if (is_file_and_exists(s.c_str()))
            {
                check_file = s;
                break;
            }
        }
        if (!is_file_and_exists(check_file.c_str()))
        {
            NLOG("[FATAL] file: '%s' could not be found.\n", string(path).c_str());
        }
        else if (!LSP.has_been_included(check_file.c_str()))
        {
            IndexFile idfile;
            idfile.file = measured_copy(check_file.c_str(), check_file.length());
            idfile.head = retrieve_file_as_lines(check_file);
            LSP.index.include.push_back(idfile);
            NLOG("%s\n", check_file.c_str());
            if (idfile.head)
            {
                LSP.check(idfile.head, check_file);
            }
        }
    }
    free(path);
}

void
do_if(linestruct *line, const char **ptr)
{
    string full_delc = LSP.parse_full_pp_delc(line, ptr);
    // NLOG("%s\n", full_delc.c_str());
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
                should_be_defined = (found != start && start[(found - start) - 1] == '!')
                                      ? FALSE
                                      :       /* This def should not be defined. */
                                        TRUE; /* This def should be defined. */
                start             = found + 7;
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
                    if (LSP.is_defined(def) == -1)
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
                        should_be_defined = (next_rule != start && start[(next_rule - start) - 1] == '!')
                                              ? FALSE
                                              :       /* This def should not be defined. */
                                                TRUE; /* This def should be defined. */
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
                            if (LSP.is_defined(def) != -1)
                            {
                                was_correct = TRUE;
                            }
                        }
                        /* Should not be defined. */
                        else
                        {
                            if (LSP.is_defined(def) == -1)
                            {
                                was_correct = TRUE;
                            }
                        }
                        NLOG("was_correct: %s\n", BOOL_STR(was_correct));
                        if (!was_correct)
                        {
                            int endif = LSP.find_endif(line->next);
                            for (linestruct *l = line->next; l && l->lineno != endif; l = l->next)
                            {
                                l->flags.set(DONT_PREPROSSES_LINE);
                            }
                        }
                    }
                    else if (index == 2)
                    {
                        if (!define_is_equl_or_greater(string(start, (end - start))))
                        {
                            int endif = LSP.find_endif(line->next);
                            for (linestruct *l = line->next; l && l->lineno != endif; l = l->next)
                            {
                                l->flags.set(DONT_PREPROSSES_LINE);
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
            if (LSP.is_defined(def) != -1)
            {
                was_correct = TRUE;
            }
        }
        /* Should not be defined. */
        else
        {
            if (LSP.is_defined(def) == -1)
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
                    int endif = LSP.find_endif(line->next);
                    for (linestruct *l = line->next; l && l->lineno != endif; l = l->next)
                    {
                        l->flags.set(DONT_PREPROSSES_LINE);
                    }
                }
            }
        }
    }
    while (found);
}

/* This is the main handler of the language server for preprossesing. */
void
do_preprossesor(linestruct *line, const char *current_file)
{
    char       *word;
    const char *found = get_preprosses_type(line, &word);
    if (found)
    {
        line->flags.set<PP_LINE>();
        if (strcmp(word, "include") == 0)
        {
            ++found;
            do_include(line, current_file, &found);
        }
        else if (strcmp(word, "ifndef") == 0)
        {
            free(word);
            word = get_next_word(&found);
            LSP.ifndef(word, line);
        }
        else if (strcmp(word, "ifdef") == 0)
        {
            free(word);
            word = get_next_word(&found);
            LSP.ifdef(word, line);
        }
        else if (strcmp(word, "define") == 0)
        {
            LSP.define(line, &found);
        }
        else if (strcmp(word, "if") == 0)
        {
            ++found;
            do_if(line, &found);
        }
        free(word);
    }
}
