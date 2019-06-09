#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mmap_allocator.h"

void *refill_mmap(data_t *data, unsigned int size)
{
	void *ptr = (void *) data + (unsigned int) sizeof(data_t);
	data_t new_node;

	if (data->data_size >= size + (unsigned int) sizeof(data_t)) {
		new_node.data_size = data->data_size - sizeof(data_t) - size;
		new_node.data = NULL;
		new_node.next = data->next;
		memcpy((void *) data + (unsigned int) sizeof(data_t) + size,
		       &new_node,
		       sizeof(data_t));
		data->next = (void *) data + (unsigned int) sizeof(data_t) +
			     size;
		data->data_size = size;
	}
	data->data = ptr;
	return (ptr);
}

void remap_mmap_free(void *map)
{
	mem_t *mem = (mem_t *) map;
	data_t *tmp;

	for (tmp = mem->data; tmp != NULL; tmp = tmp->next) {
		if (tmp->data == NULL) {
			tmp->data_size = get_new_mmap_size(tmp);
			tmp->next = get_new_next_ptr(tmp);
		}
	}
}

void mmap_free(void *map, void *ptr)
{
	mem_t *mem = (mem_t *) map;
	data_t *data;

	while (mem != NULL) {
		if ((data = get_mmap_data_from_ptr((void *) mem, ptr)) != NULL)
			break;
		mem = mem->next;
	}
	if (data == NULL)
		return;
	data->data = NULL;
	remap_mmap_free((void *) mem);
}

void *get_mmap_data_from_ptr(void *map, void *ptr)
{
	mem_t *mem = (mem_t *) map;
	data_t *tmp;

	for (tmp = mem->data; tmp != NULL; tmp = tmp->next)
		if (tmp->data == ptr)
			return (tmp);
	return (NULL);
}
