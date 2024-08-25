#include "../include/prototypes.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

task_queue_t          *task_queue        = NULL;
pthread_t             *threads           = NULL;
volatile sig_atomic_t *stop_thread_flags = NULL;

void *
terminate_work(void *)
{
    return NULL;
}

template <typename Callback>
static inline int
under_mutex_lock(Callback &&callback)
{
    if (pthread_mutex_lock(&task_queue->mutex) != 0)
    {
        return -1;
    }
    callback();
    if (pthread_mutex_unlock(&task_queue->mutex) != 0)
    {
        return -1;
    }
    return 0;
}

void *
worker_thread(void *arg)
{
    /* Assign 'thread_id' with the id passed via 'arg'. */
    unsigned char thread_id = (unsigned char)(uintptr_t)arg;
    while (1)
    {
        pthread_mutex_lock(&task_queue->mutex);
        /* Wait for a task to become avaliable. */
        while (task_queue->count == 0 && !stop_thread_flags[thread_id])
        {
            pthread_cond_wait(&task_queue->cond, &task_queue->mutex);
        }
        /* Check if stop flag is set for this thread. */
        if (stop_thread_flags[thread_id])
        {
            pthread_mutex_unlock(&task_queue->mutex);
        }
        /* Get a task from the queue. */
        task_t task       = task_queue->tasks[task_queue->front];
        task_queue->front = (task_queue->front + 1) % QUEUE_SIZE;
        task_queue->count--;
        pthread_mutex_unlock(&task_queue->mutex);
        /* Check if 'function' reciveved were the 'terminate_work' task. */
        if (task.function == terminate_work)
        {
            break;
        }
        /* Execute the task. */
        void *result = task.function(task.arg);
        if (task.result != NULL)
        {
            *(void **)task.result = result;
        }
        if (task.callback != NULL)
        {
            enqueue_callback(task.callback, result);
        }
    }
    return NULL;
}

void
init_queue_task(void)
{
    /* Init the queue. */
    if (task_queue == NULL)
    {
        task_queue        = (task_queue_t *)nmalloc(sizeof(task_queue_t));
        task_queue->front = 0;
        task_queue->rear  = 0;
        task_queue->count = 0;
        pthread_mutex_init(&task_queue->mutex, NULL);
        pthread_cond_init(&task_queue->cond, NULL);
    }
    /* Allocate memory for the 'threads'. */
    threads = (pthread_t *)nmalloc(MAX_THREADS * sizeof(pthread_t));
    /* As well as for the stop flags for the 'threads'. */
    stop_thread_flags = (volatile sig_atomic_t *)nmalloc(MAX_THREADS * sizeof(sig_atomic_t));
    /* Create the threads, and start them running working_thread. */
    for (unsigned char i = 0; i < MAX_THREADS; i++)
    {
        stop_thread_flags[i] = 0;
        if (pthread_create(&threads[i], NULL, worker_thread, (void *)(uintptr_t)i) != 0)
        {
            LOUT_logE("Failed to create thread: '%u'.", i);
        }
    }
}

void
shutdown_queue(void)
{
    for (unsigned char i = 0; i < MAX_THREADS; i++)
    {
        stop_thread(i);
        pthread_join(threads[i], NULL);
    }
    free(threads);
    free(task_queue);
    free((void *)stop_thread_flags);
}

void
submit_task(void *(*function)(void *), void *arg, void **result, void (*callback)(void *))
{
    under_mutex_lock(
        [function, arg, result, callback]
        {
            if (task_queue->count < QUEUE_SIZE)
            {
                task_queue->tasks[task_queue->rear].function = function;
                task_queue->tasks[task_queue->rear].arg      = arg;
                task_queue->tasks[task_queue->rear].result   = result;
                task_queue->tasks[task_queue->rear].callback = callback;
                task_queue->rear                             = (task_queue->rear + 1) % QUEUE_SIZE;
                task_queue->count++;
                /* Send a signal that there is a new task. */
                pthread_cond_signal(&task_queue->cond);
            }
        });
}

void
stop_thread(unsigned char thread_id)
{
    if (thread_id < MAX_THREADS)
    {
        under_mutex_lock(
            [thread_id]
            {
                stop_thread_flags[thread_id] = TRUE;
                pthread_cond_broadcast(&task_queue->cond);
            });
    }
}
