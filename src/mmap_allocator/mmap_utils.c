#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mmap_allocator.h"

void dump_mmap_mem(void *map)
{
	(void) map;
	//write(1, map, ((mem_t *) map)->actual_mem_used);
}

void dump_mmap_mem_info(void *map)
{
	mem_t *mem = (mem_t *) map;
	data_t *tmp;

	while (mem != NULL) {
		tmp = mem->data;
		while (tmp != NULL) {
			printf("page %p, node %p, size %d, data %p, next %p\n",
			mem, tmp, tmp->data_size, tmp->data, mem->next);
			tmp = tmp->next;
		}
		mem = mem->next;
	}
}

unsigned int get_mmap_total_size(void *map)
{
	unsigned int total_size = sizeof(mem_t);
	data_t *tmp = ((mem_t *) map)->data;

	while (tmp != NULL) {
		total_size += sizeof(data_t) + tmp->data_size;
		tmp = tmp->next;
	}
	return (total_size);
}

data_t *get_last_node(void *map)
{
	data_t *tmp = ((mem_t *) map)->data;

	while (tmp->next != NULL)
		tmp = tmp->next;
	return (tmp);
}

data_t *find_free_node(data_t *data, unsigned int size)
{
	data_t *tmp = data;

	while (tmp != NULL) {
		if (tmp->data == NULL && tmp->data_size >= size)
			return (tmp);
		tmp = tmp->next;
	}
	return (NULL);
}
