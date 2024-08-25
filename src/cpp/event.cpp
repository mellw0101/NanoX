#include "../include/prototypes.h"

#include <pthread.h>

callback_queue_t *callback_queue = NULL;

template <typename Callback>
static inline void
under_callback_mutex(Callback &&callback)
{
    if (pthread_mutex_lock(&callback_queue->mutex) != 0)
    {
        LOUT_logE("Failed to lock callback mutex.");
        return;
    }
    callback();
    if (pthread_mutex_unlock(&callback_queue->mutex) != 0)
    {
        LOUT_logE("Failed to unlock callback mutex.");
    }
}

/* Init the 'callback_queue'. */
void
init_callback_queue(void)
{
    callback_queue = (callback_queue_t *)malloc(sizeof(callback_queue_t));
    if (!callback_queue)
    {
        LOUT_logE("Failed to allocate mem for callback queue.");
        return;
    }
    callback_queue->head = NULL;
    callback_queue->tail = NULL;
    pthread_mutex_init(&callback_queue->mutex, NULL);
}

void
init_event_handler(void)
{
    init_callback_queue();
}

void
enqueue_callback(void (*callback)(void *), void *result)
{
    callback_node_t *new_node = (callback_node_t *)nmalloc(sizeof(callback_node_t));
    new_node->callback        = callback;
    new_node->result          = result;
    new_node->next            = NULL;
    /* The next part is done under the protection of the callback mutex. */
    under_callback_mutex(
        [new_node]
        {
            /* If 'tail' is not empty, link 'new_node' to 'tail->next'.  */
            if (callback_queue->tail)
            {
                callback_queue->tail->next = new_node;
            }
            /* Otherwise if the queue is empty, set 'new_node' as the 'head'. */
            else
            {
                callback_queue->head = new_node;
            }
            /* Now we set the 'tail' ptr to 'new_node'. */
            callback_queue->tail = new_node;
        });
}

void
prosses_callback_queue(void)
{
    /* Lock the callback mutex while we retrieve a callback. */
    pthread_mutex_lock(&callback_queue->mutex);
    /* Here we fetch all callbacks until there are no more left. */
    while (callback_queue->head != NULL)
    {
        /* Set 'node' to 'head'. */
        callback_node_t *node = callback_queue->head;
        /* Then we set the new 'head' to 'node->next'. */
        callback_queue->head = node->next;
        if (callback_queue->head == NULL)
        {
            callback_queue->tail = NULL;
        }
        /* We unlock the mutex when done fetching. */
        pthread_mutex_unlock(&callback_queue->mutex);
        /* Now we execute, then delete the 'node'. */
        node->callback(node->result);
        free(node);
        /* Now we lock the mutex for the next iter. */
        pthread_mutex_lock(&callback_queue->mutex);
    }
    pthread_mutex_unlock(&callback_queue->mutex);
}

/* Clean up and destory the callback queue. */
void
cleanup_callback_queue(void)
{
    pthread_mutex_destroy(&callback_queue->mutex);
    free(callback_queue);
}

void
cleanup_event_handler(void)
{
    cleanup_callback_queue();
}
