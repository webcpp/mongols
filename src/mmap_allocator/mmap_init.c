#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mmap_allocator.h"

void *alloc_mmap_start(mem_t *mem, unsigned int size)
{
	data_t new_node;

	new_node.data = (void *) mem + sizeof(mem_t) + sizeof(data_t);
	new_node.data_size = size;
	new_node.next = NULL;
	mem->data = (void *) mem + sizeof(mem_t);
	memcpy((void *) mem + sizeof(mem_t), &new_node, sizeof(data_t));
	return (new_node.data);
}

void *alloc_new_node(void *map, unsigned int size)
{
	int count;
	data_t new_node;

	count = get_mmap_total_size(map);
	new_node.data_size = size;
	new_node.data = map + count + sizeof(data_t);
	new_node.next = NULL;
	memcpy(map + count, &new_node, sizeof(data_t));
	get_last_node(map)->next = map + count;
	return (new_node.data);
}

