#ifndef MMAP_ALLOCATOR_H
#define MMAP_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/mman.h>

typedef struct data_s {
        void *data;
        unsigned int data_size;
        struct data_s *next;
} data_t;

typedef struct mem_s {
        struct data_s *data;
        void *end_ptr;
        struct mem_s *next;
} mem_t;

//Allocation system func
void *refill_mmap(data_t *data, unsigned int size);
void *alloc_mmap_start(mem_t *mem, unsigned int size);
void *alloc_new_node(void *map, unsigned int size);
void *get_next_alloc_space(void *map, unsigned int size);
unsigned int get_new_mmap_size(data_t *data);
void *get_new_next_ptr(data_t *data);
void *get_mmap_data_from_ptr(void *map, void *ptr);
data_t *find_free_node(data_t *data, unsigned int size);
unsigned int get_mmap_total_size(void *map);
data_t *get_last_node(void *map);

//User func
void *create_shared_memory(unsigned int size);
void *mmap_alloc(void *map, unsigned int size);
void *mmap_calloc(void *map, unsigned int size);
void mmap_free(void *map, void *ptr);
void dump_mmap_mem(void *map);
void dump_mmap_mem_info(void *map);

#ifdef __cplusplus
}
#endif

#endif
