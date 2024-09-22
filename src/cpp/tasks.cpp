#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <time.h>

/* These are the functions that the sub threads perform. */
struct sub_thread_function
{
    /* This is the task performed by the other thread. */
    static void *search_word_task(void *arg)
    {
        word_search_task_t *task = (word_search_task_t *)arg;
        NLOG("path: '%s'.\n", task->path);
        task->words = words_from_file(task->path, &task->nwords);
        return task;
    }

    /* Use a sub thread to find syntax from a header file and put them all into
     * a format we can use. Here we put them into a list that we return for the
     * main thread to prosses. */
    static void *syntax_from(void *arg)
    {
        clock_t       start_time = clock();
        char         *path       = (char *)arg;
        unsigned long nwords, i;
        char        **words = words_from_file(path, &nwords);
        if (words == NULL)
        {
            free(path);
            return NULL;
        }
        syntax_search_t *result = (syntax_search_t *)nmalloc(sizeof(*result));
        result->functions_head  = NULL;
        result->functions_tail  = NULL;
        for (i = 0; i < nwords; i++)
        {
            if (strncmp(words[i], "void", 4) == 0 || strncmp(words[i], "int", 3) == 0)
            {
                if (words[++i] != NULL)
                {
                    if (strncmp(words[i], "*__restrict", 12) == 0 || strncmp(words[i], "*/", 2) == 0)
                    {
                        if (words[++i] == NULL)
                        {
                            continue;
                        }
                    }
                    unsigned long j = 0;
                    for (; words[i][j]; j++)
                    {
                        if (words[i][j] == ',' || words[i][j] == ')' || words[i][j] == ';')
                        {
                            j = 0;
                            break;
                        }
                        if (words[i][j] == '(')
                        {
                            words[i][j] = '\0';
                            break;
                        }
                    }
                    if (j != 0)
                    {
                        while (*words[i] == '*')
                        {
                            char *p = copy_of(words[i] + 1);
                            free(words[i]);
                            words[i] = p;
                        }
                        syntax_word_t *word = (syntax_word_t *)nmalloc(sizeof(*word));
                        word->str           = copy_of(words[i]);
                        word->return_type   = NULL;
                        word->next          = NULL;
                        if (result->functions_tail == NULL)
                        {
                            result->functions_tail = word;
                            result->functions_head = word;
                        }
                        else
                        {
                            result->functions_tail->next = word;
                            result->functions_tail       = result->functions_tail->next;
                        }
                    }
                }
            }
            free(words[i]);
        }
        double tot_time = CALCULATE_MS_TIME(start_time);
        NLOG("nwords found: %lu, in file: %s\n", nwords, path);
        free(words);
        free(path);
        NLOG("time took: %lf m/s.\n", tot_time);
        if (result->functions_head == NULL)
        {
            free(result);
            return NULL;
        }
        return result;
    }

    static void *make_line_list_from_file(void *arg)
    {
        char *path = (char *)arg;
        if (!is_file_and_exists(path))
        {
            logE("Path: '%s' is not a file or does not exist.", path);
            free(path);
            return NULL;
        }
        FILE *file = fopen(path, "rb");
        if (file == NULL)
        {
            logE("Failed to open file '%s'.", path);
            free(path);
            return NULL;
        }
        static thread_local char *buf = NULL;
        unsigned long             size;
        long                      len;
        linestruct               *head = NULL;
        linestruct               *tail = NULL;
        while ((len = getline(&buf, &size, file)) != EOF)
        {
            if (buf[len - 1] == '\n')
            {

                buf[--len] = '\0';
            }
            linestruct *line = make_new_node(tail);
            line->data       = measured_copy(buf, len);
            if (tail == NULL)
            {
                head = line;
                tail = line;
            }
            else
            {
                tail->next = line;
                tail       = tail->next;
            }
        }
        fclose(file);
        free(path);
        return head;
    }

    static void *functions_from(void *arg)
    {
        char  *path      = (char *)arg;
        char **functions = find_functions_in_file(path);
        free(path);
        return functions;
    }

    static void *glob_vars_from(void *arg)
    {
        char  *path      = (char *)arg;
        char **glob_vars = find_variabels_in_file(path);
        free(path);
        return glob_vars;
    }

    static void *parse_funcs_from(void *arg)
    {
        clock_t       t_start = clock();
        char         *path    = (char *)arg;
        unsigned long nwords, i;
        char        **words = words_from_file(path, &nwords);
        if (words == NULL)
        {
            free(path);
            return NULL;
        }
        char  *start      = NULL;
        char  *end        = NULL;
        int    func_words = 0;
        char   buf[1024];
        int    cap = 10, size = 0;
        char **funcs = (char **)nmalloc(cap * sizeof(char *));
        for (i = 0; i < nwords; i++)
        {
            const unsigned int type = retrieve_c_syntax_type(words[i]);
            if (type & CS_INT || type & CS_VOID)
            {
                func_words = 0;
                while (end == NULL)
                {
                    if (strncmp(words[i + func_words], "__REDIRECT", 11) == 0 ||
                        strncmp(words[i + func_words], "__REDIRECT_NTH", 15) == 0 ||
                        strchr(words[i + func_words - 1], ';') != NULL)
                    {
                        break;
                    }
                    if (start == NULL)
                    {
                        start = strchr(words[i + func_words++], '(');
                        end   = strchr(words[i + func_words - 1], ')');
                    }
                    if (start != NULL && end == NULL)
                    {
                        end = strchr(words[i + func_words++], ')');
                    }
                    if (end != NULL)
                    {
                        buf[0] = '\0';
                        for (int j = 0; j < func_words; j++)
                        {
                            strcat(buf, words[i]);
                            free(words[i++]);
                            strcat(buf, " ");
                        }
                        (cap == size) ? cap *= 2, funcs = (char **)nrealloc(funcs, cap * sizeof(char *)) : 0;
                        funcs[size++] = copy_of(buf);
                        start         = NULL;
                        end           = NULL;
                        break;
                    }
                }
            }
            free(words[i]);
        }
        funcs[size] = NULL;
        free(words);
        NLOG("%s: file: %s, time: %lf m/s.\n", __func__, path, CALCULATE_MS_TIME(t_start));
        free(path);
        return funcs;
    }

    static void *find_file_in_dir(void *arg)
    {
        dir_search_task_t *task = (dir_search_task_t *)arg;
        task->entrys            = dir_entrys_from(task->dir);
        unsigned long i         = 0;
        for (; task->entrys[i]; i++)
        {
            if (strcmp(task->find, task->entrys[i]) == 0)
            {
                task->found = TRUE;
            }
            free(task->entrys[i]);
        }
        free(task->entrys);
        task->entrys = NULL;
        return task;
    }
};

/* Callback`s that the main thread performs. */
struct main_thread_function
{
    /* This is the function that the main thread will perform
     * when it is placed in the callback queue. */
    static void on_search_complete(void *result)
    {
        word_search_task_t *search_result = (word_search_task_t *)result;
        if (search_result->words == NULL)
        {
            free(search_result);
            return;
        }
        unsigned long i;
        /* for (i = 0; i < search_result->nwords; i++)
        {} */
        NETLOGGER.log("words fetched: %lu.\n", search_result->nwords);
        for (i = 0; i < search_result->nwords; i++)
        {
            free(search_result->words[i]);
        }
        free(search_result->words);
        free(search_result->path);
        free(search_result);
    }

    /* Callback function for the main thread to add syntax fetched by
     * subthreads. */
    static void handle_syntax(void *arg)
    {
        if (arg == NULL)
        {
            return;
        }
        syntax_search_t *result = (syntax_search_t *)arg;
        while (result->functions_head != NULL)
        {
            syntax_word_t *node    = result->functions_head;
            result->functions_head = node->next;
            if (!syntax_func(node->str))
            {
                new_syntax_func(node->str);
            }
            free(node->str);
            free(node);
        }
        free(result);
    }

    static void handle_found_functions(void *arg)
    {
        if (arg == NULL)
        {
            return;
        }
        char **functions = (char **)arg;
        for (int i = 0; functions[i]; i++)
        {
            nlog("function found %s\n", functions[i]);
            function_info_t info = parse_local_func(functions[i]);
            if (info.name)
            {
                /* char       *func = copy_of(info.name);
                const auto &it   = color_map.find(func);
                if (it == color_map.end())
                {
                    color_map[func] = {FG_VS_CODE_BRIGHT_YELLOW};
                }
                else
                {
                    free(func);
                } */
            }
            free(functions[i]);
        }
        free(functions);
    }

    static void handle_found_glob_vars(void *arg)
    {
        if (arg == NULL)
        {
            return;
        }
        char **vars = (char **)arg;
        for (int i = 0; vars[i]; i++)
        {
            glob_var_t gv;
            parse_variable(vars[i], &gv.type, &gv.name, &gv.value);
            if (gv.name)
            {
                /* const auto &it = color_map.find(gv.name);
                if (it == color_map.end())
                {
                    color_map[gv.name] = {FG_VS_CODE_BRIGHT_CYAN};
                    glob_vars.push_back(gv);
                }
                else
                {
                    NULL_safe_free(gv.type);
                    NULL_safe_free(gv.name);
                    NULL_safe_free(gv.value);
                } */
            }
            else
            {
                NULL_safe_free(gv.type);
                NULL_safe_free(gv.value);
            }
            free(vars[i]);
        }
        free(vars);
    }

    static void handle_parsed_funcs(void *arg)
    {
        if (arg == NULL)
        {
            return;
        }
        char            **funcs = (char **)arg;
        int               cap = 10, size = 0;
        function_info_t **info_array = (function_info_t **)nmalloc(cap * sizeof(**info_array));
        for (int i = 0; funcs[i]; i++)
        {
            function_info_t *info = parse_func(funcs[i]);
            if (info != NULL)
            {
                (cap == size)                                                                             ? cap *= 2,
                    info_array     = (function_info_t **)nrealloc(info_array, cap * sizeof(**info_array)) : 0;
                info_array[size++] = info;
                free(funcs[i]);
            }
        }
        free(funcs);
        info_array[size] = NULL;
        /* if (func_info == NULL)
        {
            func_info = info_array;
        }
        else
        {
            for (int i = 0;; i++)
            {
                if (func_info[i] == NULL)
                {
                    func_info =
                        (function_info_t **)nrealloc(func_info, (i + size + 1) *
        sizeof(function_info_t *)); int k = 0; for (; info_array[k]; k++)
                    {
                        func_info[i + k] = info_array[k];
                    }
                    func_info[i + k] = NULL;
                    break;
                }
            }
        } */
    }

    static void get_line_list(void *arg)
    {
        if (arg == NULL)
        {
            return;
        }
        linestruct *head = (linestruct *)arg;
        for (linestruct *line = head; line; line = line->next)
        {
            const char *class_p = word_strstr(line->data, "class");
            if (class_p && strstr(line->data, "/*") == NULL)
            {
                // const char *start = NULL;
                const char *end = class_p + 5;
                nlog("%s\n", line->data);
                if (*end == ' ' || *end == '\t')
                {
                    adv_ptr_to_next_word(end);
                    if (*end)
                    {
                        // start = end;
                        adv_ptr_past_word(end);
                        parse_class_data(line);
                    }
                }
            }
        }
        while (head)
        {
            linestruct *line = head;
            head             = line->next;
            free(line->data);
            free(line);
        }
    }

    static void on_find_file_in_dir(void *arg)
    {
        PROFILE_FUNCTION;
        dir_search_task_t *result = (dir_search_task_t *)arg;
        if (result->found == TRUE)
        {
            pause_sub_threads_guard_t pause_guard;
            LOUT_logI("Found file: '%s' in dir: '%s'.", result->find, result->dir);
        }
        free(result->find);
        free(result->dir);
        free(result);
    }
};

/* Helpers to simplyfy task ptr creation. */
struct task_creator
{
    static word_search_task_t *create_word_search_task(const char *str)
    {
        word_search_task_t *task = (word_search_task_t *)nmalloc(sizeof(word_search_task_t));
        task->path               = copy_of(str);
        return task;
    }

    static dir_search_task_t *create_dir_search_task(const char *find, const char *in_dir)
    {
        dir_search_task_t *task = (dir_search_task_t *)nmalloc(sizeof(dir_search_task_t));
        task->find              = copy_of(find);
        task->dir               = copy_of(in_dir);
        return task;
    }
};

/* Calleble functions begin here.  Above are staic functions. */

void submit_search_task(const char *path)
{
    word_search_task_t *task = task_creator::create_word_search_task(path);
    submit_task(sub_thread_function::search_word_task, task, NULL, main_thread_function::on_search_complete);
}

void submit_find_in_dir(const char *find, const char *in_dir)
{
    dir_search_task_t *task = task_creator::create_dir_search_task(find, in_dir);
    submit_task(sub_thread_function::find_file_in_dir, task, NULL, main_thread_function::on_find_file_in_dir);
}

void sub_thread_find_syntax(const char *path)
{
    submit_task(sub_thread_function::syntax_from, copy_of(path), NULL, main_thread_function::handle_syntax);
}

void sub_thread_parse_funcs(const char *path)
{
    submit_task(sub_thread_function::parse_funcs_from, copy_of(path), NULL, main_thread_function::handle_parsed_funcs);
}

void find_functions_task(const char *path)
{
    char *alloced_path = copy_of(path);
    submit_task(sub_thread_function::functions_from, alloced_path, NULL, main_thread_function::handle_found_functions);
}

void find_glob_vars_task(const char *path)
{
    char *alloced_path = copy_of(path);
    submit_task(sub_thread_function::glob_vars_from, alloced_path, NULL, main_thread_function::handle_found_glob_vars);
}

void get_line_list_task(const char *path)
{
    char *arg = copy_of(path);
    submit_task(sub_thread_function::make_line_list_from_file, arg, NULL, main_thread_function::get_line_list);
}
