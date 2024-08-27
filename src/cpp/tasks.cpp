#include "../include/prototypes.h"

#include <Mlib/Profile.h>

/* These are the functions that the sub threads perform. */
struct sub_thread_function
{
    /* This is the task performed by the other thread. */
    static void *
    search_word_task(void *arg)
    {
        word_search_task_t *task = (word_search_task_t *)arg;
        NETLOGGER.log("path: '%s'.\n", task->path);
        task->words = words_from_file(task->path, &task->nwords);
        return task;
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
        delete_c_syntax_task_t *task = (delete_c_syntax_task_t *)arg;
        colortype              *c;
        for (c = task->syntax_type->color; c->next != NULL && !str_equal_to_rgx(task->word, c->next->start);
             c = c->next, task->iter++)
            ;
        if (c->next != NULL)
        {
            pthread_mutex_guard_t sub_guard(&task_queue->mutex);
            colortype            *tc = c->next->next;
            (c->next->end) ? regfree(c->next->end) : void();
            regfree(c->next->start);
            free(c->next);
            c->next = tc;
        }
        return task;
    }

    /* TODO: (add_c_syntax) - Finish this. */
    static void *
    add_c_syntax(void *arg)
    {
        PROFILE_FUNCTION;
        add_c_syntax_task_t *task = (add_c_syntax_task_t *)arg;
        if (c_syntaxtype == NULL)
        {
            return task;
        }
        short    fg, bg;
        int      attr;
        regex_t *start = NULL;
        if (!parse_color_opts(task->color_fg, task->color_bg, &fg, &bg, &attr))
        {
            LOUT_logE("Could not parse color opts.");
            return task;
        }
        if (!compile(task->rgxstr, NANO_REG_EXTENDED, &start))
        {
            LOUT_logE("Could not compile rgxstr: '%s'.", task->rgxstr);
            return task;
        }
        if (task->color_type == NULL)
        {
            colortype            *c = NULL;
            pthread_mutex_guard_t guard(&task_queue->mutex);
            for (c = c_syntaxtype->color; c->next != NULL; c = c->next)
                ;
            if (c == NULL)
            {
                LOUT_logE("Could not retrieve the last item in c colortype linked list.");
                return task;
            }
            (*task->color_type) = c;
        }
        colortype *c  = (colortype *)nmalloc(sizeof(colortype));
        c->start      = start;
        c->end        = NULL;
        c->fg         = fg;
        c->bg         = bg;
        c->attributes = attr;
        if ((*task->color_type)->next != NULL)
        {
            colortype            *c;
            pthread_mutex_guard_t guard(&task_queue->mutex);
            for (c = c_syntaxtype->color; c->next != NULL; c = c->next)
                ;
            (*task->color_type) = c;
        }
        c->pairnum                = (*task->color_type)->pairnum + 1;
        c->next                   = NULL;
        (*task->color_type)->next = c;
        (*task->color_type)       = (*task->color_type)->next;
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
    check_add_c_syntax(void *arg)
    {
        add_c_syntax_task_t *task = (add_c_syntax_task_t *)arg;
        (task->color_fg) ? free(task->color_fg) : void();
        (task->color_bg) ? free(task->color_bg) : void();
        free(task->rgxstr);
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

    static add_c_syntax_task_t *
    create_add_c_syntax_task(const char *color_fg, const char *color_bg, const char *rgxstr, colortype **color_type)
    {
        add_c_syntax_task_t *task = (add_c_syntax_task_t *)nmalloc(sizeof(add_c_syntax_task_t));
        task->color_fg            = (color_fg) ? copy_of(color_fg) : NULL;
        task->color_bg            = (color_bg) ? copy_of(color_bg) : NULL;
        task->rgxstr              = copy_of(rgxstr);
        task->color_type          = color_type;
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
sub_thread_add_c_syntax(const char *color_fg, const char *color_bg, const char *rgxstr, colortype **color_type)
{
    add_c_syntax_task_t *task = task_creator::create_add_c_syntax_task(color_fg, color_bg, rgxstr, color_type);
    submit_task(sub_thread_function::add_c_syntax, task, NULL, main_thread_function::check_add_c_syntax);
}
