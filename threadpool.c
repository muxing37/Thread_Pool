#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct Task {
    void (*function)(void *arg);
    void *arg;
};

struct Node {
    struct Task *task;
    struct Node *next;
};

struct Queue {
    struct Node *front;
    struct Node *rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct Threadpool {
    pthread_t *threads;
    int thread_count;
    struct Queue queue;
    int shutdown;
};

typedef struct threadpool {
    pthread_t *threads;
    int thread_count;
    struct Queue queue;
    int shutdown;
} threadpool;

void init_queue(struct Queue *q) {
    q->front=NULL;
    q->rear=NULL;
}

void enqueue(struct Queue *q,struct Task *task) {
    struct Node *node=calloc(1,sizeof(struct Node));
    node->task=task;
    if(q->rear==NULL) {
        q->front=node;
        q->rear=node;
    } else {
        q->rear->next=node;
        q->rear=node;
    }
}

struct Task* dequeue(struct Queue *q) {
    if(q->front==NULL) return NULL;
    struct Node *temp=q->front;
    struct Task *task=temp->task;
    q->front=temp->next;
    if(q->front==NULL) q->rear=NULL;
    free(temp);
    return task;
}

void threadpool_init(threadpool *pool,int n) {
    int i;
    pool->thread_count=n;
    pool->shutdown=0;
    pool->threads=malloc(n*sizeof(pthread_t));
    for(i=0;i<n;i++) {
        //pthread_create(&pool->threads[i],NULL,worker,pool);
    }
}

//-------------------------------------------
//以下用于测试

void test_task(void *arg) {
    int num = *(int*)arg;

    printf("Thread %lu is processing task %d\n", pthread_self(), num);

    // 模拟耗时任务
    sleep(1);

    printf("Thread %lu finished task %d\n", pthread_self(), num);

    //free(arg);  // 释放参数
}

int main() {
    struct Queue q;
    init_queue(&q);

    struct Task *t = malloc(sizeof(struct Task));

    t->function = test_task;
    int x = 10;
    t->arg = &x;

    enqueue(&q,t);

    struct Task *t2 = dequeue(&q);
    t2->function(t2->arg);
    free(t);
}

// int main() {
//     struct Threadpool pool;

//     //初始化线程池（4个线程）
//     threadpool_init(&pool, 4);

//     //提交10个任务
//     for (int i = 0; i < 10; i++) {
//         struct Task *task = malloc(sizeof(struct Task));

//         int *arg = malloc(sizeof(int));
//         *arg = i;

//         task->function = test_task;
//         task->arg = arg;

//         threadpool_add(&pool, task);
//     }

//     //等待任务执行（简单方式）
//     sleep(5);

//     //销毁线程池
//     threadpool_destroy(&pool);

//     return 0;
// }