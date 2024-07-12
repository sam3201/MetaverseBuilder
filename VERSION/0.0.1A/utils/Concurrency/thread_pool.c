#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

static void *threadPoolWorker(void *arg);

ThreadPool *threadPoolCreate(int threadCount, int taskQueueSize) {
    ThreadPool *pool;
    int i;

    if (threadCount <= 0 || taskQueueSize <= 0) {
        return NULL;
    }

    pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (!pool) {
        perror("Failed to allocate thread pool");
        return NULL;
    }

    pool->threadCount = threadCount;
    pool->taskQueueSize = taskQueueSize;
    pool->taskQueueHead = 0;
    pool->taskQueueTail = 0;
    pool->taskCount = 0;
    pool->shutdown = false;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * threadCount);
    pool->taskQueue = (ThreadTask *)malloc(sizeof(ThreadTask) * taskQueueSize);

    if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
        pthread_cond_init(&(pool->notify), NULL) != 0 ||
        pool->threads == NULL || pool->taskQueue == NULL) {
        perror("Failed to initialize thread pool");
        free(pool->threads);
        free(pool->taskQueue);
        free(pool);
        return NULL;
    }

    for (i = 0; i < threadCount; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadPoolWorker, (void *)pool) != 0) {
            perror("Failed to create thread");
            threadPoolDestroy(pool);
            return NULL;
        }
    }

    return pool;
}

bool threadPoolAddTask(ThreadPool *pool, void (*function)(void *), void *argument) {
    int next;

    if (pool == NULL || function == NULL) {
        return false;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return false;
    }

    next = (pool->taskQueueTail + 1) % pool->taskQueueSize;
    if (pool->taskCount == pool->taskQueueSize) {
        pthread_mutex_unlock(&(pool->lock));
        return false;
    }

    pool->taskQueue[pool->taskQueueTail].function = function;
    pool->taskQueue[pool->taskQueueTail].argument = argument;
    pool->taskQueueTail = next;
    pool->taskCount += 1;

    if (pthread_cond_signal(&(pool->notify)) != 0) {
        pthread_mutex_unlock(&(pool->lock));
        return false;
    }

    if (pthread_mutex_unlock(&(pool->lock)) != 0) {
        return false;
    }

    return true;
}

void threadPoolDestroy(ThreadPool *pool) {
    int i;

    if (pool == NULL) {
        return;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return;
    }

    pool->shutdown = true;

    if (pthread_cond_broadcast(&(pool->notify)) != 0 || pthread_mutex_unlock(&(pool->lock)) != 0) {
        return;
    }

    for (i = 0; i < pool->threadCount; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            return;
        }
    }

    if (pthread_mutex_destroy(&(pool->lock)) != 0 || pthread_cond_destroy(&(pool->notify)) != 0) {
        return;
    }

    free(pool->threads);
    free(pool->taskQueue);
    free(pool);
}

static void *threadPoolWorker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    ThreadTask task;

    while (true) {
        if (pthread_mutex_lock(&(pool->lock)) != 0) {
            break;
        }

        while (pool->taskCount == 0 && !pool->shutdown) {
            if (pthread_cond_wait(&(pool->notify), &(pool->lock)) != 0) {
                pthread_mutex_unlock(&(pool->lock));
                return NULL;
            }
        }

        if (pool->shutdown && pool->taskCount == 0) {
            pthread_mutex_unlock(&(pool->lock));
            break;
        }

        task.function = pool->taskQueue[pool->taskQueueHead].function;
        task.argument = pool->taskQueue[pool->taskQueueHead].argument;
        pool->taskQueueHead = (pool->taskQueueHead + 1) % pool->taskQueueSize;
        pool->taskCount -= 1;

        pthread_mutex_unlock(&(pool->lock));

        (*(task.function))(task.argument);
    }

    pthread_exit(NULL);
    return NULL;
}
