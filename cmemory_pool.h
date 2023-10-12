#ifndef CMEMORY_POOL_H
#define CMEMORY_POOL_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

#define MP_ALIGNMENT      32
#define MP_PAGE_SIZE      4096
#define MP_MAX_ALLOC_FROM_POOL (MP_PAGE_SIZE-1)

#define mp_align(n,alignment) (((n)+(alignment-1)) & ~(alignment-1))
#define mp_align_ptr(p, alignment) (void *)((((size_t)p)+(alignment-1)) & ~(alignment-1))


typedef mp_large_s {
  struct mp_large_s* next;
  void* alloc;
}mp_large_t;


typedef struct mp_node_s { 
   unsigned char* last;
   unsigned char*  end;
  
  struct mp_node_s* next;

  size_t failed;

}mp_node_t;

 typedef struct mp_pool_s{

    size_t max;

    mp_node_t* current;
    mp_large_t* large;

    struct mp_node_s head[0];

 }mp_pool_t;

class cmemory_pool{
    
public: 

    static cmemory_pool& GetInstance();//获得内存池的单例

    mp_pool_t* mp_create_pool(size_t size);//创建指定大小的内存池

    void* mp_destroy_pool(mp_pool_t* pool);//销毁内存池
    void mp_reset_pool(mp_pool_t *pool); //重置内存池

    void* mp_memalign( mp_pool_t *pool, size_t size, size_t alignment);//

    void* mp_alloc(mp_pool_t* pool,size_t size);
    void* mp_calloc(mp_pool_t* pool,size_t size);

    void mp_free(mp_pool_t* pool,void* p);

 private:
     
     cmemory_pool();
     cmemory_pool(const sys_info &)=delete;
     cmemory_pool& operator=(const cmemory_pool &)=delete;
     cmemory_pool(cmemory_pool &&)=delete;
     cmemory_pool& operator=(cmemory_pool &&)= delete;
     

    void* mp_alloc_block(mp_pool_t* pool,size_t size);
    void* mp_alloc_large(mp_pool_t* pool,size_t size);

    ~cmemory_pool();
};
 
#endif



