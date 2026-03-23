#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct Task {
    void *(*function)(void *arg);
    void *arg;
    void (*callback)(void *result);
} Task;

typedef struct Threadpool Threadpool;

struct Threadpool* threadpool_create(int thread_count);
int threadpool_add(struct Threadpool *pool, struct Task *task);
void threadpool_destroy(struct Threadpool *pool);
void threadpool_wait_end(struct Threadpool *pool);

#endif