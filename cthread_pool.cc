#include "cthread_pool.h"

#include <stdlib.h>
#include <string.h>

thread_pool_t* thread_pool_create(unsigned char thread_count, unsigned char task_queue_count){

    if(thread_count==0 || task_queue_count==0){
        //todo::接入日志，记录函数错误调用
        return nullptr;
    }

    thread_pool_t* pool=(thread_pool_t*)malloc(sizeof(thread_pool_t));
    if(pool==nullptr){
        //todo::接入日志，记录线程池内存分配失败
        return nullptr;
    }
    memset(pool,0,sizeof(thread_pool_t));
    pool->task_queue.head=pool->task_queue.tail==pool->task_queue.count=0;
    pool->thrd_count=thread_count;
    pool->queue_size=task_queue_count;

    pool->task_queue.queue=(task_t*)malloc(sizeof(task_t)*pool->queue_size);
    if (pool->task_queue.queue == nullptr) {
        //todo::接入日志库
         free(pool);
        return nullptr;
    }
    memset(pool->task_queue.queue,0,sizeof(task_t)*pool->queue_size);

    pool->threads=(pthread_t*)malloc(sizeof(pthread_t)*pool->thrd_count);
    if(pool->threads==nullptr){
        //todo::接入日志
        free(pool);
        free(pool->task_queue.queue);
        return nullptr;
    }

     for (int i=0; i < pool->thrd_count; i++) {
     if (pthread_create(&(pool->threads[i]), NULL, thread_worker, void* p) != 0) {
        //todo::接入日志
        free(poll);
        free(pool->task_queue.queue);
        return nullptr;
        }
 
    pool->threading_count++;
  }
 
  pool->thread_poll_status=true;
  return pool; 
}

int thread_pool_post(thread_pool_t *pool, handler_ptr func, void *arg) {
 
   if(pool==nullptr || func==nullptr || arg==nullptr){
     //接入日志
     return -1;   //这里可以加错误处理函数
   }
  
   task_queue_t *task_queue = &(pool->task_queue);
   if(task_queue==nullptr){
    //std::cout<<" pool task_queue wrong"<<std::endl;
    return -2;   //这里可以加错误处理函数
   }
 
   int ret=0;
 
  //给队列加锁，使用自旋锁会更优
 
   ret=pthread_mutex_lock(&(pool->mutex));
   if(ret!=0){
    //std::cout<<" mutex lock wrong"<<std::endl;
    return -3;   //加锁失败
   }
   
   if (pool->thread_poll_status ==false) {
        pthread_mutex_unlock(&(pool->mutex));
        std::cout<<" thread_poll status wrong"<<std::endl;
        return -3;//线程池未开启
    }
   
     if (task_queue->count == pool->queue_size) {
        pthread_mutex_unlock(&(pool->mutex));
        std::cout<<" pool queue is full"<<std::endl;
        return -4; //任务队列已满
    }
   
    task_queue->queue[task_queue->tail].func = func;
    task_queue->queue[task_queue->tail].arg = arg;
    task_queue->tail =( task_queue->tail+1) % pool->queue_size;//这里是取余操作
    task_queue->count++;
 
    ret=pthread_cond_signal(&(pool->condition));
    if(ret!=0){
        pthread_mutex_unlock(&(pool->mutex));
        std::cout<<" cond_signal wrong"<<std::endl;
        return -5;
    }
 
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}

void * thread_worker(void *thrd_pool) {
 
    thread_pool_t *pool = (thread_pool_t*)thrd_pool;//线程池入口函数
    task_queue_t *que;
    task_t task;
 
    while(1) {
 
        pthread_mutex_lock(&(pool->mutex)); //加锁
        que = &pool->task_queue; //取出任务
 
        while (que->count == 0 && pool->thread_pool_status ==false) { 
            pthread_cond_wait(&(pool->condition), &(pool->mutex));//阻塞线程
           //加while()循环是防止系统本身虚假唤醒线程，或者业务逻辑不严谨
        }
        
      //使用条件变量进行线程调度
        if (pool->thread_poll_status==false) break;
 
        task = que->queue[que->head];
        que->head = (que->head + 1) % pool->queue_size; //循环队列
        que->count--;
        pthread_mutex_unlock(&(pool->mutex));//解锁
 
        (*(task.func))(task.arg); //执行任务
    }
 
    pthread_mutex_unlock(&(pool->mutex));//解锁
    pthread_exit(NULL); //退出线程
}


int wait_all_done(thread_pool_t *pool) {
    int i, ret=0;
    for (i=0; i < pool->thrd_count; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            ret=1;
        }
    }   
    return ret; //成功返回1，失败返回0
}

 void thread_pool_free(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }
 
    if (pool->threads) {
        free(pool->threads);
        pool->threads = NULL;
 
        pthread_mutex_lock(&(pool->mutex));
        pthread_mutex_destroy(&pool->mutex);
        pthread_cond_destroy(&pool->condition);
    }
 
    if (pool->task_queue.queue) {
        free(pool->task_queue.queue);
        pool->task_queue.queue = NULL;
    }
    free(pool);
}

 
int thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) {
        return -1;
    }
 
    if (pthread_mutex_lock(&(pool->mutex)) != 0) {
        return -2;
    }
 
    if (pool->thread_pool_status==false) {
        thread_pool_free(pool);
        return -3;
    }
 
   //通过广播信号唤醒所有等待在条件变量pool->condition上的线程，并让它们重新竞争
    if (pthread_cond_broadcast(&(pool->condition)) != 0 ||  pthread_mutex_unlock(&(pool->mutex)) != 0) {
        thread_pool_free(pool);
        return -4;
    }
 
    wait_all_done(pool);
 
    thread_pool_free(pool);
    pool->thread_pool_status=false;
    return 0;
}
