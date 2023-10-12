#include "cmemory_pool.h"

cmemory_pool& cmemory_pool::GetInstance(){
    static cmemory_pool instance ;
    return instance;
}

mp_pool_t* cmemory_pool::mp_create_pool(size_t size){

    mp_pool_t *p;
	int ret = posix_memalign((void **)&p, MP_ALIGNMENT, size + sizeof(mp_pool_t) + sizeof(mp_node_t));
	if (ret) {
		return NULL;
	}
	
	p->max = (size < MP_MAX_ALLOC_FROM_POOL) ? size : MP_MAX_ALLOC_FROM_POOL;
	p->current = p->head;
	p->large = NULL;

	p->head->last = (unsigned char *)p + sizeof(mp_pool_t) + sizeof(mp_node_t);
	p->head->end = p->head->last + size;

	p->head->failed = 0;

	return p;

}

void cmemory_pool:: mp_destory_pool( mp_pool_t *pool) {

	mp_node_t *h, *n;
	mp_large_t *l;

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	h = pool->head->next;

	while (h) {
		n = h->next;
		free(h);
		h = n;
	}

	free(pool);

    return ;
}

void cmemory_pool::mp_reset_pool(mp_pool_t *pool) {

	 mp_node_t *h;
	 mp_large_t *l;

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	pool->large = NULL;

	for (h = pool->head; h; h = h->next) {
		h->last = (unsigned char *)h + sizeof(mp_node_t);
	}

}

void* cmemory_pool::mp_alloc_block(mp_pool_t *pool, size_t size) {

	unsigned char *m;
    mp_node_t *h = pool->head;
	size_t psize = (size_t)(h->end - (unsigned char *)h);
	
	int ret = posix_memalign((void **)&m, MP_ALIGNMENT, psize);
	if (ret) return NULL;

	mp_node_t *p, *new_node, *current;
	new_node = (mp_node_t*)m;

	new_node->end = m + psize;
	new_node->next = NULL;
	new_node->failed = 0;

	m += sizeof(mp_node_t);
	m = mp_align_ptr(m, MP_ALIGNMENT);
	new_node->last = m + size;

	current = pool->current;

	for (p = current; p->next; p = p->next) {
		if (p->failed++ > 4) { //
			current = p->next;
		}
	}
	p->next = new_node;

	pool->current = current ? current : new_node;

	return m;

}

void * cmemory_pool::mp_alloc_large(mp_pool_t *pool, size_t size) {

	void *p = malloc(size);
	if (p == NULL) return NULL;

	size_t n = 0;
    mp_large_t *large;

	for (large = pool->large; large; large = large->next) {
		if (large->alloc == NULL) {
			large->alloc = p;
			return p;
		}
		if (n ++ > 3) break;
	}

	large = mp_alloc(pool, sizeof(struct mp_large_s));
	if (large == NULL) {
		free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}

void* cmemory_pool::mp_memalign( mp_pool_t *pool, size_t size, size_t alignment) {

	void *p;
	
	int ret = posix_memalign(&p, alignment, size);
	if (ret) {
		return NULL;
	}

    mp_large_t *large = mp_alloc(pool, sizeof( mp_large_t));
	if (large == NULL) {
		free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}

void* cmemory_pool::mp_alloc(mp_pool_t *pool, size_t size) {

	unsigned char *m;
	struct mp_node_s *p;

	if (size <= pool->max) {

		p = pool->current;

		do {
			m = mp_align_ptr(p->last, MP_ALIGNMENT);
			if ((size_t)(p->end - m) >= size) {
				p->last = m + size;
				return m;
			}
			p = p->next;
		} while (p);

		return this->mp_alloc_block(pool, size);
	}

	return this->mp_alloc_large(pool, size);
	
}

void* cmemory_pool::mp_calloc(mp_pool_t *pool, size_t size) {

	void *p = mp_alloc(pool, size);
	if (p) {
		memset(p, 0, size);
	}

	return p;
	
}

void cmemory_pool:: mp_free(mp_pool_t *pool, void *p) {

	mp_large_t *l;
	for (l = pool->large; l; l = l->next) {
		if (p == l->alloc) {
			free(l->alloc);
			l->alloc = NULL;

			return ;
		}
	}
	
}

// int main(int argc, char *argv[]) {

// 	int size = 1 << 12;

// 	mp_pool_t *p = mp_create_pool(size);

// 	int i = 0;
// 	for (i = 0;i < 10;i ++) {

// 		void *mp = cmemory_pool::GetInstance().mp_alloc(p, 512);
// //		mp_free(mp);
// 	}

// 	//printf("mp_create_pool: %ld\n", p->max);
// 	printf("mp_align(123, 32): %d, mp_align(17, 32): %d\n", mp_align(24, 32), mp_align(17, 32));
// 	//printf("mp_align_ptr(p->current, 32): %lx, p->current: %lx, mp_align(p->large, 32): %lx, p->large: %lx\n", mp_align_ptr(p->current, 32), p->current, mp_align_ptr(p->large, 32), p->large);

// 	int j = 0;
// 	for (i = 0;i < 5;i ++) {

// 		char *pp =cmemory_pool::GetInstance().mp_calloc(p, 32);
// 		for (j = 0;j < 32;j ++) 
// 			if (pp[j]) {
// 				printf("calloc wrong\n");
// 			}
// 			printf("calloc success\n");
// 		}
// 	}

// 	//printf("mp_reset_pool\n");

// 	for (i = 0;i < 5;i ++) {
// 		void *l = cmemory_pool::GetInstance().mp_alloc(p, 8192);
// 		cmemory_pool::GetInstance().mp_free(p, l);
// 	}

// 	cmemory_pool::GetInstance().mp_reset_pool(p);

// 	//printf("mp_destory_pool\n");
// 	for (i = 0;i < 58;i ++) {
// 		cmemory_pool::GetInstance().mp_alloc(p, 256);
// 	}

// 	cmemory_pool::GetInstance().mp_destory_pool(p);

// 	return 0;

// }