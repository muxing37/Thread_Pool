#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "threadpool.h"

#define CAP 10000033

struct Range {
    int *num;
    int left;
    int right;
};

int cmp(const void *a,const void *b) {
    int x=*(const int *)a;
    int y=*(const int *)b;
    if(x<y) return -1;
    if(x>y) return 1;
    return 0;
}

void *sort_task(void *arg) {
    struct Range *p=(struct Range *)arg;
    qsort(p->num+p->left,p->right-p->left,sizeof(int),cmp);
    return NULL;
}

void get_arry(int *num) {
    srand(time(NULL));
    int i;
    for(i=0;i<CAP;i++) {
        num[i]=rand()%100000000;
    }
}

int main()
{
    int i,j;
    int *num=malloc(sizeof(int)*CAP);
    int c=sysconf(_SC_NPROCESSORS_ONLN);
    int NUMBER=2*c;
    int range=CAP/NUMBER+1;
    int n=NUMBER;
    get_arry(num);

    struct Threadpool *pool=threadpool_create(c);

    for(j=0;n>0;j++) {
        for(i=0;i<n;i++) {
            struct Range *ra=calloc(1,sizeof(struct Range));
            ra->left=i*range;
            ra->right=(i+1)*range;
            if(ra->right>CAP) ra->right=CAP;

            ra->num=num;

            Task *task=malloc(sizeof(Task));
            task->arg=ra;
            task->callback=NULL;
            task->function=sort_task;
            
            threadpool_add(pool,task);
        }
        if(n%2==0 || n==1) n=n/2;
        else n=(n+1)/2;
        range=range*2;
        threadpool_wait(pool);
    }
}