#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <time.h>

#define NEW_SYSTEM FALSE

/* These are the functions that the sub threads perform. */
struct sub_thread_function
{
    /* This is the task performed by the other thread. */
    static void *
    search_word_task(void *arg)
    {
        word_search_task_t *task = (word_search_task_t *)arg;
        NLOG("path: '%s'.\n", task->path);
        task->words = words_from_file(task->path, &task->nwords);
        return task;
    }

    /* Use a sub thread to find syntax from a header file and put them all into a format we can use.
     * Here we put them into a list that we return for the main thread to prosses. */
    static void *
    syntax_from(void *arg)
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

    static void *
    find_file_in_dir(void *arg)
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

    /* Delete`s one syntax form the c syntax linked list. */
    static void *
    delete_one_c_syntax(void *arg)
    {
        PROFILE_FUNCTION;
        pthread_mutex_guard_t   guard(&task_queue->mutex);
        delete_c_syntax_task_t *task = (delete_c_syntax_task_t *)arg;
        colortype              *c;
        for (c = task->syntax_type->color; c->next != NULL && !str_equal_to_rgx(task->word, c->next->start);
             c = c->next, task->iter++)
            ;
        if (c->next != NULL)
        {
            colortype *tc = c->next->next;
            (c->next->end != NULL) ? regfree(c->next->end) : void();
            (c->next->start != NULL) ? regfree(c->next->start) : void();
            free(c->next);
            c->next = tc;
        }
        return task;
    }

    static void *
    compile_rgx(void *arg)
    {
        compile_rgx_task_t *task                           = (compile_rgx_task_t *)arg;
        static void (*cleanup)(compile_rgx_task_t *, bool) = [](compile_rgx_task_t *task, bool error)
        {
            if (task)
            {
                (task->color_fg) ? free(task->color_fg) : void();
                (task->color_bg) ? free(task->color_bg) : void();
                (task->rgxstr) ? free(task->rgxstr) : void();
                if (error)
                {
                    (task->rgx) ? free(task->rgx) : void();
                    free(task);
                }
            }
        };
        if (!parse_color_opts(task->color_fg, task->color_bg, &task->fg, &task->bg, &task->attr))
        {
            LOUT_logE("Failed to parse color opts.");
            cleanup(task, TRUE);
            return NULL;
        }
        if (!compile(task->rgxstr, NANO_REG_EXTENDED, &task->rgx))
        {
            LOUT_logE("Could not compile rgxstr: '%s'.", task->rgxstr);
            cleanup(task, TRUE);
            return NULL;
        }
        cleanup(task, false);
        return task;
    }
};

/* Callback`s that the main thread performs. */
struct main_thread_function
{
    /* This is the function that the main thread will perform
     * when it is placed in the callback queue. */
    static void
    on_search_complete(void *result)
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

    /* Callback function for the main thread to add syntax fetched by subthreads. */
    static void
    handle_syntax(void *arg)
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

    static void
    on_find_file_in_dir(void *arg)
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

    /* Check wether or not the thread successfully removed the syntax.
     * Also free the struct used to hold the data. */
    static void
    check_delete_one_c_syntax(void *arg)
    {
        delete_c_syntax_task_t *result = (delete_c_syntax_task_t *)arg;
        NETLOGGER.log("%lu\n", result->iter);
        free(result->word);
        free(result);
    }

    static void
    add_rgx_to_list(void *arg)
    {
        PROFILE_FUNCTION;
        if (arg == NULL || c_syntaxtype == NULL)
        {
            return;
        }
        compile_rgx_task_t *task = (compile_rgx_task_t *)arg;
        if (task->last_c == NULL)
        {
            colortype *c;
            for (c = c_syntaxtype->color; c->next != NULL; c = c->next)
                ;
            (*task->last_c) = c;
        }
        colortype *c  = (colortype *)nmalloc(sizeof(colortype));
        c->start      = task->rgx;
        c->end        = NULL;
        c->fg         = task->fg;
        c->bg         = task->bg;
        c->attributes = task->attr;
        if ((*task->last_c)->next != NULL)
        {
            colortype *c;
            for (c = (*task->last_c); c->next; c = c->next)
                ;
            (*task->last_c) = c;
        }
        c->pairnum            = (*task->last_c)->pairnum + 1;
        c->next               = NULL;
        (*task->last_c)->next = c;
        (*task->last_c)       = (*task->last_c)->next;
        refresh_needed        = TRUE;
        perturbed             = TRUE;
        free(task);
    }
};

/* Helpers to simplyfy task ptr creation. */
struct task_creator
{
    static word_search_task_t *
    create_word_search_task(const char *str)
    {
        word_search_task_t *task = (word_search_task_t *)nmalloc(sizeof(word_search_task_t));
        task->path               = copy_of(str);
        return task;
    }

    static dir_search_task_t *
    create_dir_search_task(const char *find, const char *in_dir)
    {
        dir_search_task_t *task = (dir_search_task_t *)nmalloc(sizeof(dir_search_task_t));
        task->find              = copy_of(find);
        task->dir               = copy_of(in_dir);
        return task;
    }

    static delete_c_syntax_task_t *
    create_delete_c_syntax_task(const char *word)
    {
        if (c_syntaxtype == NULL)
        {
            LOUT_logE("'c_syntaxtype' == NULL.");
            return NULL;
        }
        delete_c_syntax_task_t *task = (delete_c_syntax_task_t *)nmalloc(sizeof(delete_c_syntax_task_t));
        task->syntax_type            = c_syntaxtype;
        task->word                   = copy_of(word);
        task->iter                   = 0;
        return task;
    }

    static compile_rgx_task_t *
    create_compile_rgx_task(const char *color_fg, const char *color_bg, const char *rgxstr,
                            colortype **last_c)
    {
        compile_rgx_task_t *task = (compile_rgx_task_t *)nmalloc(sizeof(compile_rgx_task_t));
        task->color_fg           = (color_fg) ? copy_of(color_fg) : NULL;
        task->color_bg           = (color_bg) ? copy_of(color_bg) : NULL;
        task->rgxstr             = copy_of(rgxstr);
        task->last_c             = last_c;
        task->rgx                = NULL;
        return task;
    }
};

/* Calleble functions begin here.  Above are staic functions. */

void
submit_search_task(const char *path)
{
    word_search_task_t *task = task_creator::create_word_search_task(path);
    submit_task(sub_thread_function::search_word_task, task, NULL, main_thread_function::on_search_complete);
}

void
submit_find_in_dir(const char *find, const char *in_dir)
{
    dir_search_task_t *task = task_creator::create_dir_search_task(find, in_dir);
    submit_task(sub_thread_function::find_file_in_dir, task, NULL, main_thread_function::on_find_file_in_dir);
}

/* Call on a sub thread to delete a 'c' syntax obj.  We will have to see if we
 * need to copy the syntax then restore it later or if we should mutex lock it. */
void
sub_thread_delete_c_syntax(char *word)
{
    delete_c_syntax_task_t *task = task_creator::create_delete_c_syntax_task(word);
    free(word);
    (task) ? submit_task(sub_thread_function::delete_one_c_syntax, task, NULL,
                         main_thread_function::check_delete_one_c_syntax) :
             void();
}

void
sub_thread_compile_add_rgx(const char *color_fg, const char *color_bg, const char *rgxstr, colortype **last_c)
{
    compile_rgx_task_t *task = task_creator::create_compile_rgx_task(color_fg, color_bg, rgxstr, last_c);
    submit_task(sub_thread_function::compile_rgx, task, NULL, main_thread_function::add_rgx_to_list);
}

void
sub_thread_find_syntax(const char *path)
{
    submit_task(sub_thread_function::syntax_from, copy_of(path), NULL, main_thread_function::handle_syntax);
}
