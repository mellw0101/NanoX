#include "../include/prototypes.h"

void *
search_word_task(void *arg)
{
    word_search_task_t *task   = (word_search_task_t *)arg;
    word_search_task_t *result = (word_search_task_t *)nmalloc(sizeof(word_search_task_t));
    result->words              = words_from_file(task->path, &task->nwords);
    result->nwords             = task->nwords;
    return result;
}

void
on_search_complete(void *result)
{
    word_search_task_t *search_result = (word_search_task_t *)result;
    if (search_result->words == NULL)
    {
        free(search_result);
        return;
    }
    unsigned long i;
    for (i = 0; i < search_result->nwords; i++)
    {
        NETLOGGER.log("%s\n", search_result->words[i]);
    }
    NETLOGGER.log("words fetched: %lu.\n", search_result->nwords);
    for (i = 0; i < search_result->nwords; i++)
    {
        free(search_result->words[i]);
    }
    free(search_result->words);
    if (search_result->path == NULL)
    {
        free(search_result->path);
    }
    free(search_result);
}

void
submit_search_task(const char *path)
{
    word_search_task_t *new_search_task = (word_search_task_t *)nmalloc(sizeof(word_search_task_t));
    new_search_task->path               = copy_of(path);
    submit_task(search_word_task, new_search_task, NULL, on_search_complete);
}
