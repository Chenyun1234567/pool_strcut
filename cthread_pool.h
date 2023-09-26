//该线程池设置12个线程，一个任务发布者

#ifndef CTHREAD_POLL_H
#define CTHREAD_POLL_H

#include <pthread.h>
#include <cstdint>

typedef void(*handler_ptr)(void*);

typedef struct task_s{
    handler_ptr task_func; //队列处理函数
    void* arg;    //函数的参数，可以封装一个结构体，以实现多个参数传递
}task_t;

typedef struct task_queue_s{
    uint32_t head;    //头指针
    uint32_t tail;    //尾指针
    uint32_t count;   //当前任务的数量
    task_t *queue;    //储存任务的队列
}task_queue_t;

 
typedef struct thread_pool_s{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_t *threads;//线程数组
    task_queue_t task_queue;//任务队列
 
    bool thread_poll_status ;//false为关闭，true为开启
     
    int thrd_count; //线程的数量
    int queue_size;  //任务的大小
    unsigned char threading_count=0;
    
}thread_pool_t;

thread_pool_t* thread_pool_create(unsigned char thread_count, unsigned char task_queue_count);
int thread_pool_post(thread_pool_t *pool, handler_ptr func, void *arg) ;
void * thread_worker(void *thrd_pool) ;
int thread_pool_destroy(thread_pool_t *pool);

#endif