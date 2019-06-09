#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mmap_allocator.h"

void *get_next_alloc_space(void *map, unsigned int size)
{
	data_t *tmp;
	data_t new_node;

	if ((tmp = find_free_node(((mem_t *) map)->data, size)) == NULL)
		return (alloc_new_node(map, size));
	if (tmp->data_size >= size + sizeof(data_t)) {
		new_node.data_size = tmp->data_size - size - sizeof(data_t);
		new_node.data = NULL;
		new_node.next = tmp->next;
		tmp->data_size = size;
		tmp->next = (void *) tmp + size + sizeof(data_t);
		memcpy((void *) tmp + size + sizeof(data_t), &new_node,
		sizeof(data_t));
	}
	tmp->data = (void *) tmp + sizeof(data_t);
	return (tmp->data);
}

void *get_allocable_page(void *map, unsigned int page_size, unsigned int size)
{
	mem_t *mem = (mem_t *) map;
	mem_t *tmp = mem;
	data_t *node = NULL;

	while (tmp != NULL) {
		if (tmp->data == NULL || (node = find_free_node(tmp->data,
		size))	!= NULL)
			return (tmp);
		else if ((void * ) tmp + get_mmap_total_size(tmp) + sizeof
		(data_t) + size <= tmp->end_ptr)
			return (tmp);
		mem = tmp;
		tmp = tmp->next;
	}
	tmp = create_shared_memory((page_size >= sizeof(mem_t) + sizeof
	(data_t) + size) ? page_size : sizeof(mem_t) + sizeof(data_t) + size);
	mem->next = (void *) tmp;
	return (tmp);
}

void *mmap_alloc(void *map, unsigned int size)
{
	void *mem;
	data_t *tmp;
	unsigned int page_size = (unsigned int) getpagesize();

	mem = get_allocable_page(map, page_size, size);
	tmp = ((mem_t *) mem)->data;
	if (mem == NULL)
		return (NULL);
	if (tmp == NULL)
		return (alloc_mmap_start(mem, size));
	return (get_next_alloc_space(mem, size));
}

unsigned int get_new_mmap_size(data_t *data)
{
	unsigned int size = data->data_size;

	data = data->next;
	while (data != NULL && data->data == NULL) {
		size += sizeof(data_t) + data->data_size;
		data = data->next;
	}
	return (size);
}

void *get_new_next_ptr(data_t *data)
{
	data = data->next;
	while (data != NULL) {
		if (data->data != NULL)
			return (data);
		data = data->next;
	}
	return (NULL);
}

void *create_shared_memory(unsigned int size)
{
	int protection = PROT_READ | PROT_WRITE;
	int visibility = MAP_ANONYMOUS | MAP_SHARED;
	void *shmem = mmap(NULL, size, protection, visibility, -1, 0);
	mem_t mem;

	mem.data = NULL;
	mem.end_ptr = shmem + size;
	mem.next = NULL;
	memcpy(shmem, &mem, sizeof(mem_t));
	return (shmem);
}
