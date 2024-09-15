#include "../include/prototypes.h"

#include <Mlib/Profile.h>
#include <pthread.h>

callback_queue_t *callback_queue = NULL;

bool is_main_thread(void)
{
    return (pthread_equal(main_thread->thread, pthread_self()));
}

/* Helper function to perform actions protected by the
 * 'callback_queue' mutex. */
template <typename Callback>
static inline void under_callback_mutex(Callback &&callback)
{
    pthread_mutex_guard_t guard(&callback_queue->mutex);
    callback();
}

/* Helper func to lock the callback queue`s mutex. */
static inline void lock_callback_mutex(bool lock)
{
    lock_pthread_mutex(&callback_queue->mutex, lock);
}

/* Init the 'callback_queue'. */
void init_callback_queue(void)
{
    callback_queue = (callback_queue_t *)nmalloc(sizeof(callback_queue_t));
    callback_queue->head = NULL;
    callback_queue->tail = NULL;
    pthread_mutex_init(&callback_queue->mutex, NULL);
}

/* This is the main init function for the event handler.
 * This is used to init all subfunctions of the event handler. */
void init_event_handler(void)
{
    init_callback_queue();
    init_main_thread();
}

/* Place a callback at the end of the queue. */
void enqueue_callback(callback_functionptr_t callback, void *result)
{
    callback_node_t *node = (callback_node_t *)nmalloc(sizeof(callback_node_t));
    node->callback        = callback;
    node->result          = result;
    node->next            = NULL;
    /* The next part is done under the protection of the callback mutex. */
    under_callback_mutex(
        [node]
        {
            /* If 'tail' is not empty, link 'node' to 'tail->next'.
             * Otherwise, we set 'node' as the 'head'. */
            (callback_queue->tail) ? (callback_queue->tail->next = node) :
                                     (callback_queue->head = node);
            /* Now we set the 'tail' ptr to 'new_node'. */
            callback_queue->tail = node;
        });
}

/* This function is used by the main thread to perform
 * all callbacks in the queue. */
void prosses_callback_queue(void)
{
    /* Lock the callback mutex while we retrieve a callback. */
    lock_callback_mutex(TRUE);
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
        lock_callback_mutex(FALSE);
        /* Now we execute, then delete the 'node'. */
        node->callback(node->result);
        free(node);
        /* Now we lock the mutex for the next iter. */
        lock_callback_mutex(TRUE);
    }
    lock_callback_mutex(FALSE);
}

/* Clean up and destory the callback queue. */
void cleanup_callback_queue(void)
{
    pthread_mutex_destroy(&callback_queue->mutex);
    free(callback_queue);
}

/* This is the main cleanup function for the event handler, this is used to
 * clean up all subfunctions of the event handler, this way we can ensure
 * everything gets cleaned in the correct order. */
void cleanup_event_handler(void)
{
    cleanup_callback_queue();
}
