#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    void (*function)(void *);
    void *argument;
} ThreadTask;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    ThreadTask *taskQueue;
    int threadCount;
    unsigned int taskQueueSize;
    int taskQueueHead;
    int taskQueueTail;
    int taskCount;
    bool shutdown;
} ThreadPool;

ThreadPool *threadPoolCreate(int threadCount, int taskQueueSize);
bool threadPoolAddTask(ThreadPool *pool, void (*function)(void *), void *argument);
void threadPoolDestroy(ThreadPool *pool);

#endif 
