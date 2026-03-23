#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct Task {
    void *(*function)(void *arg);
    void *arg;
    void (*callback)(void *result);
};

struct Node {
    struct Task *task;
    struct Node *next;
};

struct Queue {
    struct Node *head;
    struct Node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct Threadpool {
    pthread_t *threads;
    int thread_count;
    struct Queue queue;
    int shutdown;

    int count_task;
    pthread_mutex_t count_mutex;
    pthread_cond_t count_cond;
};

void enqueue(struct Queue *q,struct Task *task) {
    pthread_mutex_lock(&q->mutex);
    struct Node *node=calloc(1,sizeof(struct Node));
    node->task=task;
    if(q->tail==NULL) {
        q->head=node;
        q->tail=node;
    } else {
        q->tail->next=node;
        q->tail=node;
    }
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

struct Task* dequeue(struct Queue *q,int *shutdown) {
    pthread_mutex_lock(&q->mutex);

    while(q->head==NULL && !(*shutdown)) {
        pthread_cond_wait(&q->cond,&q->mutex);
    }

    if(q->head==NULL && *shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }
    struct Node *temp=q->head;
    struct Task *task=temp->task;
    q->head=temp->next;
    if(q->head==NULL) q->tail=NULL;
    pthread_mutex_unlock(&q->mutex);
    free(temp);
    return task;
}

void* worker(void *arg) {
    struct Threadpool *pool=(struct Threadpool*)arg;
    while(1) {
        struct Task *t=dequeue(&pool->queue,&pool->shutdown);
        if(t==NULL) {
            break;
        }
        void *result=t->function(t->arg);
        if(t->callback) {
            t->callback(result);
        }
        free(t->arg);
        free(t);
        pthread_mutex_lock(&pool->count_mutex);
        pool->count_task--;
        if(pool->count_task==0) {
            pthread_cond_signal(&pool->count_cond);
        }
        pthread_mutex_unlock(&pool->count_mutex);
    }
    return NULL;
}

int threadpool_add(struct Threadpool *pool,struct Task *task) {
    pthread_mutex_lock(&pool->count_mutex);
    if(pool->shutdown) {
        pthread_mutex_unlock(&pool->count_mutex);
        return -1;
    }
    pool->count_task++;
    pthread_mutex_unlock(&pool->count_mutex);

    enqueue(&pool->queue,task);
    return 0;
}

void threadpool_init(struct Threadpool *pool,int n) {
    int i;
    pool->thread_count=n;
    pool->shutdown=0;
    pool->threads=malloc(n*sizeof(pthread_t));
    pool->count_task=0;
    pthread_mutex_init(&pool->count_mutex,NULL);
    pthread_cond_init(&pool->count_cond,NULL);
    pool->queue.head=NULL;
    pool->queue.tail=NULL;
    pthread_mutex_init(&pool->queue.mutex,NULL);
    pthread_cond_init(&pool->queue.cond,NULL);
}

struct Threadpool* threadpool_create(int n) {
    int i;
    struct Threadpool *pool=malloc(sizeof(struct Threadpool));
    threadpool_init(pool,n);
    for(i=0;i<n;i++) {
        pthread_create(&pool->threads[i],NULL,worker,pool);
    }
    return pool;
}

void threadpool_destroy(struct Threadpool *pool) {
    int i;
    pthread_mutex_lock(&pool->queue.mutex);
    pool->shutdown=1;
    pthread_cond_broadcast(&pool->queue.cond);
    pthread_mutex_unlock(&pool->queue.mutex);

    for(i=0;i<pool->thread_count;i++) {
        pthread_join(pool->threads[i],NULL);
    }

    struct Node *cur=pool->queue.head;
    while(cur) {
        struct Node *tmp=cur;
        free(cur->task->arg);
        free(cur->task);
        cur=cur->next;
        free(tmp);
    }
    pthread_mutex_destroy(&pool->queue.mutex);
    pthread_cond_destroy(&pool->queue.cond);
    pthread_mutex_destroy(&pool->count_mutex);
    pthread_cond_destroy(&pool->count_cond);
    free(pool->threads);
    free(pool);
}

void threadpool_wait_end(struct Threadpool *pool) {
    pthread_mutex_lock(&pool->count_mutex);
    while(pool->count_task>0) {
        pthread_cond_wait(&pool->count_cond,&pool->count_mutex);
    }
    pthread_mutex_unlock(&pool->count_mutex);
    threadpool_destroy(pool);
}